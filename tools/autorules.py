#!/usr/bin/env python3
"""Mechanically apply the cookbook's *local* matching rules and keep what helps.

Most cookbook rules are RECOGNITION rules: you must read the asm diff to know
which one applies and where (loop shape, switch-vs-ladder, union-offset casts,
two-regs-same-value...). A blind sweep can't place those. But a handful are
purely MECHANICAL — a token/expression rewrite at an enumerable site whose only
open question is "does it move the bytes the right way?". Nobody (agent or human)
should hand-apply those every time.

This tool enumerates every applicable site of each mechanical rule, tries it,
rebuilds, and rescores with the authoritative whole-image byte diff — then
GREEDILY keeps the single best improving edit and repeats until nothing helps
(or it MATCHES). Greedy re-sweeping handles independent wins. Guided mode adds
a bounded beam, because a pure-C CFG or loop-note edit can be byte-neutral by
itself but enable the next edit.

It is the deterministic, explainable first pass that complements decomp-permuter
(the stochastic search over register-allocation ties): autorules reports exactly
which rule closed how many instructions ("widen `n` to s32: -16"), leaving a
smaller residual for a permuter run or an agent's judgment.

Safe at the acceptance boundary: byte-identical to the original IS correctness,
so a full MATCH is always valid; any non-zero result is advisory and gets
reviewed. Individual rules also reject locally provable semantic regressions;
an improving partial score is not, by itself, proof that a rewrite is sound.

  tools/autorules.py <Name>          sweep + greedy-compose; write the best .c
  tools/autorules.py <Name> --dry    report only, leave the file untouched
  tools/autorules.py <Name> --once   a single pass (no greedy re-sweep)
  tools/autorules.py <Name> --list   list the registered mechanical rules
  tools/autorules.py <Name> --guided RTL/source-line-guided advanced search
  tools/autorules.py <Name> --rules loop-fence,cmp-polarity --beam 4 --depth 2

Run inside the nix devShell. Each candidate costs one ./Build (incremental), so
a run is a few dozen builds — a minute or two, unattended.

Candidate output is line-buffered for monitors. Matching tools share one
per-worktree lock. Candidates compile from a private staged source, so even an
ungraceful exit cannot strand a speculative rewrite in the checked-in file.
SIGTERM/SIGHUP also tear down the active build process group, and on Linux a
parent-death signal prevents a command frontend from orphaning autorules.
"""
import argparse, glob, hashlib, itertools, os, re, signal, subprocess, sys, tempfile
import proclife
from contextlib import contextmanager

from matchlock import MatchToolBusy, matching_tool_lock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

SRC = "src/main.exe"
ASMDIFF = "tools/asmdiff.py"
MATCHDIFF = "tools/matchdiff.py"
INVALID = 10**9  # candidate did not compile
SOURCE_OVERRIDE_PREFIX = "TENCHU_MATCH_SOURCE_"

# rtlguide fills this for the expensive/codegen-specific rule family.  A small
# tolerance accounts for instructions attributed to an adjacent statement by
# sched/reorg.  Ordinary mechanical rules remain global: they are cheap and
# were safe before guided mode existed.
GUIDED_LINES = set()
# Names attached by rtlguide to hard registers that differ from the target.
# These are deliberately separate from GUIDED_LINES: a register-allocation
# repair may need to add a reference at an earlier donor site, not at the
# instruction where the final hard-register swap becomes visible.
GUIDED_VARIABLES = set()
_TARGET_GP_CACHE = {}


def _guided_site(line):
    return not GUIDED_LINES or any(abs(line - n) <= 2 for n in GUIDED_LINES)


def _target_gp_symbols(name):
    """Symbols the target accesses with `%gp_rel`, cached per function.

    `extern-array` is an absolute-address materialization lever. Trying it on a
    known gp-relative target can improve a partial alignment score while making
    the relevant access categorically worse.
    """
    if name in _TARGET_GP_CACHE:
        return _TARGET_GP_CACHE[name]
    directory = f".shake/gen/main.exe/asm/nonmatchings/{name}"
    files = sorted(glob.glob(f"{directory}/*.s")) or glob.glob(
        f".shake/gen/main.exe/asm/nonmatchings/*/{name}.s"
    )
    symbols = set()
    for path in files:
        with open(path, errors="replace") as stream:
            symbols.update(re.findall(r"%gp_rel\(([A-Za-z_]\w*)\)", stream.read()))
    _TARGET_GP_CACHE[name] = symbols
    return symbols


# ---------------------------------------------------------------------------
# Source handling: locate the editable region and the function body within it.
# A NON_MATCHING partial has two bodies (INCLUDE_ASM stub XOR the draft behind
# `#else`); we only ever edit the draft, and build it with NON_MATCHING set.
# ---------------------------------------------------------------------------

def load(name):
    path = os.path.join(SRC, name + ".c")
    if not os.path.exists(path):
        sys.exit(f"autorules: {path} not found")
    text = open(path).read()
    partial = "ifndef NON_MATCHING" in text
    return path, text, partial


def region_span(text, partial):
    """(start, end) character span of the part we may edit.

    For a partial that's the draft between `#else` and its closing `#endif`;
    otherwise the whole file. We further restrict edits to inside the target
    function's body (see body_lines) so signatures/prototypes stay untouched.
    """
    if not partial:
        return 0, len(text)
    m = re.search(r"^#else\b.*$", text, re.M)
    if not m:
        return 0, len(text)
    start = m.end()
    ends = [e.start() for e in re.finditer(r"^#endif\b.*$", text, re.M)]
    end = next((e for e in ends if e > start), len(text))
    return start, end


def body_line_ranges(text, span, name):
    """Line-index ranges (inclusive start, exclusive end) that lie inside the
    body of function `name` within `span` — i.e. after its opening `{`. Edits are
    confined here so return types, params and prototypes are never retyped.
    Falls back to the whole span if the signature can't be located."""
    lines = text.split("\n")
    # map char span -> line indices
    starts = [0]
    for ln in lines:
        starts.append(starts[-1] + len(ln) + 1)
    lo = 0
    while lo < len(lines) and starts[lo + 1] <= span[0]:
        lo += 1
    hi = len(lines)
    while hi > lo and starts[hi - 1] >= span[1]:
        hi -= 1
    # Per-line "starts inside a /* */ block (or is an obvious comment line)"
    # mask, so the signature/brace scan never trips on the header comment
    # (which mentions `Name(`).
    mask = [False] * len(lines)
    in_block = False
    for i, ln in enumerate(lines):
        mask[i] = in_block or ln.lstrip().startswith(("*", "//", "/*"))
        j = 0
        while j < len(ln):
            two = ln[j:j + 2]
            if not in_block and two == "/*":
                in_block = True; j += 2; continue
            if in_block and two == "*/":
                in_block = False; j += 2; continue
            j += 1
    # find the DEFINITION line (`name(` not in a comment, not a prototype/call)
    # then brace-match its body.
    sig = re.compile(rf"\b{re.escape(name)}\s*\(")
    ranges = []
    i = lo
    while i < hi:
        line = lines[i]
        if (not mask[i] and sig.search(line)
                and not line.lstrip().startswith(("extern", "static", "return"))):
            j = i
            while j < hi and "{" not in lines[j]:
                if ";" in lines[j]:  # prototype / call statement, not a def
                    j = -1
                    break
                j += 1
            if j == -1 or j >= hi:
                i += 1
                continue
            depth, body_start, k = 0, j + 1, j
            while k < hi:
                depth += lines[k].count("{") - lines[k].count("}")
                if depth <= 0:
                    ranges.append((body_start, k))
                    break
                k += 1
            i = k + 1
            continue
        i += 1
    if not ranges:
        return lines, [(lo, hi)]
    return lines, ranges


def uncommented(lines, rng):
    """Yield (line_index, line_text) for live (non-comment) lines in the range,
    tracking /* */ block state and skipping // lines."""
    in_block = False
    for i in range(*rng):
        s = lines[i]
        t = s
        if in_block:
            if "*/" in t:
                in_block = False
                t = t.split("*/", 1)[1]
            else:
                continue
        # strip trailing line comments / opening block comments for the test
        code = re.sub(r"//.*$", "", t)
        if "/*" in code:
            before, _after = code.split("/*", 1)
            if "*/" not in _after:
                in_block = True
            code = before
        yield i, s, code


# ---------------------------------------------------------------------------
# The rule registry. A rule is gen(text, name, span) -> iterator of
# (label, new_text): each yielded candidate is the FULL file text with ONE site
# rewritten. The driver scores them all and greedily keeps the best. Simple
# token rewrites (type-width) are line/regex based; structural rewrites (&&-nest,
# temp-inline, call-result-split) parse with tree-sitter (get_parser("c")) and
# splice byte spans —
# tolerant of the SDK headers / INCLUDE_ASM / preprocessor without fake headers.
# Add mechanical rules here as the cookbook grows.
# ---------------------------------------------------------------------------

# Integer types we treat as interchangeable, by (width, signed).
TYPES = {
    "s8": (8, True), "u8": (8, False), "char": (8, True),
    "s16": (16, True), "u16": (16, False), "short": (16, True),
    "s32": (32, True), "u32": (32, False),
    "int": (32, True), "long": (32, True), "unsigned": (32, False),
}
# Canonical representative to emit for each (width, signed).
CANON = {(8, True): "s8", (8, False): "u8",
         (16, True): "s16", (16, False): "u16",
         (32, True): "s32", (32, False): "u32"}
WIDTH_ORDER = [8, 16, 32]


def flip_targets(w, s):
    """The (width, signed) flips worth trying for a local of type (w, s): an
    adjacent width at the same signedness (the s16↔s32 index-width lever, 8↔16)
    and the same width at the opposite signedness (the lhu/lh, terminator-sign
    levers). Two-step width changes compose over greedy passes, so we don't
    enumerate them (keeps builds/pass down)."""
    idx = WIDTH_ORDER.index(w)
    out = []
    for d in (-1, 1):
        k = idx + d
        if 0 <= k < len(WIDTH_ORDER):
            out.append((WIDTH_ORDER[k], s))
    out.append((w, not s))
    return out
DECL = re.compile(
    r"^(\s*)(const\s+|volatile\s+)*(" + "|".join(sorted(TYPES, key=len, reverse=True))
    + r")(\s+)(\**\s*)(\w+)\s*(?=[;,=\[])")


def _has_explicit_signed_use(lines, rng, variable):
    """Whether changing a signed local to unsigned would visibly change C.

    A partial byte score can improve after an unsigned rewrite even while it
    deletes a required negative correction or turns an arithmetic ``sra`` into
    ``srl``. Those are semantic facts, not allocation levers. Keep this test
    deliberately local and conservative: explicit comparisons against zero or
    a negative literal, and direct right shifts of the variable, lock its
    signedness. Width changes that preserve signedness remain available.
    """
    code = "\n".join(piece for _i, _raw, piece in uncommented(lines, rng))
    ident = re.escape(variable)
    signed_literal = r"(?:0(?:[lL]*)|-\s*(?:0[xX][0-9a-fA-F]+|\d+)(?:[lL]*))"
    comparisons = (
        rf"\b{ident}\b\s*(?:<|<=|>|>=)\s*{signed_literal}\b",
        rf"{signed_literal}\s*(?:<|<=|>|>=)\s*\b{ident}\b",
    )
    if any(re.search(pattern, code) for pattern in comparisons):
        return True
    # Covers ``x >> n`` and parenthesised ``(x) >> n``.
    return re.search(rf"\b{ident}\b\s*\)*\s*>>", code) is not None


def rule_type_width(text, name, span):
    """Flip a local variable's integer type across width/signedness.

    THE most-cited mechanical lever (cookbook Expressions): a short-returning
    call result that indexes an array wants s32 not s16; a u16 field read
    signed; a terminator's signedness; short-vs-int call results (PauseProc);
    an HImode switch guard that must not reuse an SImode case constant
    (ActATTACK).
    For each provably single, plain scalar local declaration we try each
    *distinct* (width, signedness) canonical type.  Pointer, array,
    multi-declarator, and multi-line declarations are excluded: changing a
    shared base type can change another declarator's pointee access width or
    object layout, not just the scalar local's machine mode. A signed local
    with an explicit sign comparison or arithmetic right shift is never
    flipped to unsigned: an improving partial score cannot justify deleting
    that semantics. Line-based (robust without the AST); confined to the body.
    """
    lines, ranges = body_line_ranges(text, span, name)
    for rng in ranges:
        for i, raw, code in uncommented(lines, rng):
            m = DECL.match(code)
            if not m:
                continue
            # Group 5 is the optional pointer stars; the match ends immediately
            # after the first identifier. Require that this whole declaration
            # ends on the same line and has no comma, because in
            # `s16 scalar, *view` retyping the apparently scalar first
            # declarator also changes `view`'s pointee width.
            suffix = code[m.end():]
            declaration_tail = suffix.split(";", 1)[0]
            if ("*" in m.group(5) or
                    suffix.lstrip().startswith("[") or
                    ";" not in suffix or "," in declaration_tail):
                continue
            cur = m.group(3)
            w, s = TYPES[cur]
            signed_use = s and _has_explicit_signed_use(lines, rng, m.group(6))
            for (w2, s2) in flip_targets(w, s):
                if signed_use and not s2:
                    continue
                rep = CANON[(w2, s2)]
                # rebuild THIS declaration on the real (uncommented-source) line
                new_line = _swap_decl_type(raw, m, rep)
                if new_line is None or new_line == raw:
                    continue
                nl = lines[:]
                nl[i] = new_line
                yield (f"{m.group(6)}: {cur}→{rep}", "\n".join(nl))


POINTER_DECL = re.compile(
    r"^(\s*)((?:(?:const|volatile)\s+)*)(" +
    "|".join(sorted(TYPES, key=len, reverse=True)) +
    r")(\s+\*+\s*([A-Za-z_]\w*))"
)


def rule_pointee_volatile(text, name, span):
    """Toggle prefix ``volatile`` on a local integer pointer declaration.

    ``volatile u16 *p`` forces each ``*p`` access to remain a real load/store;
    without it cse can reuse an earlier signed/unsigned view or reconstruct the
    address from a different base.  The qualifier applies to the pointee, not
    the pointer object.  Keep this guided/opt-in and restrict it to one plain
    local pointer declarator on one line; authoritative scoring decides whether
    the target demonstrates the volatile accesses.
    """
    lines, ranges = body_line_ranges(text, span, name)
    for rng in ranges:
        for index, raw, code in uncommented(lines, rng):
            match = POINTER_DECL.match(code)
            if not match or not _guided_site(index + 1):
                continue
            tail = code[match.end():]
            if "," in tail.split(";", 1)[0]:
                continue
            qualifiers = match.group(2).split()
            if "volatile" in qualifiers:
                qualifiers.remove("volatile")
                tag = "remove"
            else:
                qualifiers.append("volatile")
                tag = "add"
            qualifier_text = ((" ".join(qualifiers) + " ")
                              if qualifiers else "")
            replacement = (match.group(1) + qualifier_text + match.group(3) +
                           match.group(4))
            changed = replacement + raw[match.end():]
            if changed == raw:
                continue
            updated = lines[:]
            updated[index] = changed
            yield (f"pointee-volatile {tag} {match.group(5)} L{index + 1}",
                   "\n".join(updated))


EXTERN_POINTER_ARRAY_DECL = re.compile(
    r"^\s*extern\s+((?:struct\s+)?[A-Za-z_]\w*)\s*\*\s*"
    r"([A-Za-z_]\w*)\s*\[\s*[^\]]*\s*\]\s*;"
)


def rule_pointer_slot_volatile(text, name, span):
    """Read one extern pointer-array slot through a volatile-qualified lvalue.

    Old cc1 can CSE ``Objects[i]`` across a long decision tree even when the
    target reloads the pointer slot immediately before a final field read.  A
    site-local ``(*(T *volatile *)&Objects[i])->field`` keeps precisely that
    pointer-object read observable without making ``T`` or every array access
    volatile.  This is guided-only and deliberately narrow: a simple extern
    array of pointers, a simple index expression, and a direct ``->field`` use.
    Authoritative scoring and review must confirm the target's reload.
    """
    lines, ranges = body_line_ranges(text, span, name)
    externs = {}
    for _index, _raw, code in uncommented(lines, (0, len(lines))):
        match = EXTERN_POINTER_ARRAY_DECL.match(code)
        if match:
            externs[match.group(2)] = match.group(1)
    if not externs:
        return

    for rng in ranges:
        for index, raw, code in uncommented(lines, rng):
            if not _guided_site(index + 1):
                continue
            for array, pointee in externs.items():
                access = re.compile(
                    rf"\b{re.escape(array)}\s*\[\s*"
                    r"([A-Za-z_]\w*(?:\s*[+-]\s*\d+)?)\s*\]"
                    r"\s*->\s*([A-Za-z_]\w*)"
                )
                for match in access.finditer(code):
                    index_expr = match.group(1)
                    field = match.group(2)
                    replacement = (
                        f"(*({pointee} *volatile *)&{array}[{index_expr}])"
                        f"->{field}"
                    )
                    changed = (raw[:match.start()] + replacement +
                               raw[match.end():])
                    updated = lines[:]
                    updated[index] = changed
                    yield (f"pointer-slot-volatile {array}[{index_expr}]"
                           f"->{field} L{index + 1}", "\n".join(updated))


def _swap_decl_type(raw, m, rep):
    """Replace the type token in `raw` at the position it occupies in the
    matched (comment-stripped) code. We re-find the type token followed by the
    same variable name to stay robust to a stripped trailing comment."""
    var = re.escape(m.group(6))
    typ = re.escape(m.group(3))
    pat = re.compile(rf"(^\s*(?:const\s+|volatile\s+)*)({typ})(\s+\**\s*{var}\b)")
    new, n = pat.subn(rf"\g<1>{rep}\g<3>", raw, count=1)
    return new if n else None


# ---- AST support (tree-sitter). Gated: if unavailable the AST rules yield
# nothing and autorules still runs the line-based rules. ----------------------
try:
    from tree_sitter_language_pack import get_parser
    _TS = get_parser("c")
except Exception:
    _TS = None


# tree-sitter reports UTF-8 BYTE offsets, so all AST text handling works on
# `data = text.encode()` (bytes) and decodes the result back to str. Mixing byte
# offsets with Python str indexing corrupts any file with non-ASCII (e.g. an
# em-dash in a header comment) — that bug cost a debugging round; keep it bytes.
def _txt(data, n):
    return data[n.start_byte:n.end_byte]


def splice(data, start, end, repl):
    return data[:start] + repl + data[end:]


def _line(data, off):
    return data.count(b"\n", 0, off) + 1


def _byte_span(text, span):
    return (len(text[:span[0]].encode()), len(text[:span[1]].encode()))


def _find(node, types):
    out = []
    stack = [node]
    while stack:
        n = stack.pop()
        if n.type in types:
            out.append(n)
        stack.extend(reversed(n.children))
    return out


def _descend_ident(node):
    """Follow a declarator down to its identifier (through pointer/array/etc)."""
    while node is not None and node.type != "identifier":
        nxt = node.child_by_field_name("declarator")
        if nxt is None:
            ids = [c for c in node.children if c.type == "identifier"]
            return ids[0] if ids else None
        node = nxt
    return node


def _paren_inner(cond):
    """Inner expression of a parenthesized_expression condition."""
    if cond is None:
        return None
    kids = [c for c in cond.named_children if c.type != "comment"]
    return kids[0] if kids else None


def _func_body(data, name, bspan):
    """The body compound_statement node of function `name` within byte-span
    `bspan`. `data` is the utf-8 bytes; return its body node."""
    if _TS is None:
        return None
    want = name.encode()

    def locate(root):
        for n in _find(root, ("function_definition",)):
            if not (bspan[0] <= n.start_byte and n.end_byte <= bspan[1]):
                continue
            fd = n.child_by_field_name("declarator")
            while fd is not None and fd.type != "function_declarator":
                fd = fd.child_by_field_name("declarator")
            if fd is None:
                continue
            idn = _descend_ident(fd.child_by_field_name("declarator"))
            if idn is not None and _txt(data, idn) == want:
                return n.child_by_field_name("body")
        return None

    body = locate(_TS.parse(data).root_node)
    if body is not None:
        return body

    # Some guarded drafts put raw INCLUDE_ASM macro invocations in the other
    # preprocessor arm.  tree-sitter-c can recover the tokens but may wrap the
    # entire #ifndef/#else region in one ERROR node, hiding the perfectly valid
    # function_definition from every AST rule.  Reparse with bytes outside the
    # editable arm blanked.  Keeping newlines and byte count intact preserves
    # every source offset, including UTF-8 header comments, so nodes from the
    # fallback tree remain safe to splice into the original data.
    if bspan != (0, len(data)):
        masked = bytearray(data)
        for start, end in ((0, bspan[0]), (bspan[1], len(masked))):
            for index in range(start, end):
                if masked[index] not in (10, 13):
                    masked[index] = 32
        return locate(_TS.parse(bytes(masked)).root_node)
    return None


def _lone_if(node):
    """`node` is a single if_statement, or a block wrapping exactly one."""
    if node.type == "if_statement":
        return node
    if node.type == "compound_statement":
        stmts = [c for c in node.named_children if c.type != "comment"]
        if len(stmts) == 1 and stmts[0].type == "if_statement":
            return stmts[0]
    return None


def rule_stack_decl_swap(text, name, span):
    """Swap adjacent same-type address-taken object declarations.

    In ``leLayoutEnemy``, old cc1 assigns the two separately declared VECTOR
    locals' stack slots in declaration order.  When otherwise interchangeable
    scratch objects occupy the target's slots in the opposite order, swapping
    only their declarations is a bounded source lever.

    Keep the transform deliberately narrow: both declarations must be plain,
    uninitialized objects of the exact same typedef type, direct children of
    one block, with no comment between them, and both names must be used with
    unary ``&``.  Candidate scoring decides which order is useful.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    addressed = set()
    for pointer in _find(body, ("pointer_expression",)):
        operator = pointer.child_by_field_name("operator")
        argument = pointer.child_by_field_name("argument")
        if (operator is not None and _txt(data, operator) == b"&" and
                argument is not None and argument.type == "identifier"):
            addressed.add(_txt(data, argument))

    def plain_object(declaration):
        if declaration.type != "declaration":
            return None
        type_node = declaration.child_by_field_name("type")
        declarator = declaration.child_by_field_name("declarator")
        if (type_node is None or type_node.type != "type_identifier" or
                declarator is None or declarator.type != "identifier"):
            return None
        named = [child for child in declaration.named_children
                 if child.type != "comment"]
        if len(named) != 2 or named[0] != type_node or named[1] != declarator:
            return None
        return _txt(data, type_node), _txt(data, declarator)

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for first, second in zip(statements, statements[1:]):
            first_object = plain_object(first)
            second_object = plain_object(second)
            if (first_object is None or second_object is None or
                    first_object[0] != second_object[0] or
                    first_object[1] not in addressed or
                    second_object[1] not in addressed or
                    data[first.end_byte:second.start_byte].strip()):
                continue
            gap = data[first.end_byte:second.start_byte]
            replacement = (_txt(data, second) + gap + _txt(data, first))
            yield (
                f"stack-decl-swap {first_object[1].decode()}/"
                f"{second_object[1].decode()} L{_line(data, first.start_byte)}",
                splice(data, first.start_byte, second.end_byte,
                       replacement).decode(),
            )


def rule_and_nest(text, name, span):
    """`if (A && B) S`  <->  `if (A) { if (B) S }` — cc1 collapses a redundant
    `&&` chain (dropping a branch the binary keeps), so NESTing recovers it;
    the merge is the reverse. Only when there is no `else` (nesting rebinds it).
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for iff in _find(body, ("if_statement",)):
        if iff.child_by_field_name("alternative") is not None:
            continue
        cond = iff.child_by_field_name("condition")
        cons = iff.child_by_field_name("consequence")
        if cond is None or cons is None:
            continue
        inner = _paren_inner(cond)
        # SPLIT: condition A && B  ->  if (A) { if (B) S }
        op = inner.child_by_field_name("operator") if inner is not None else None
        if (inner is not None and inner.type == "binary_expression"
                and op is not None and _txt(data, op) == b"&&"):
            L = _txt(data, inner.child_by_field_name("left"))
            R = _txt(data, inner.child_by_field_name("right"))
            repl = b"if (" + L + b") { if (" + R + b") " + _txt(data, cons) + b" }"
            yield (f"&&-split L{_line(data, iff.start_byte)}",
                   splice(data, iff.start_byte, iff.end_byte, repl).decode())
        # MERGE: consequence is a lone if  ->  if (A && B) S
        inner_if = _lone_if(cons)
        if inner_if is not None and inner_if.child_by_field_name("alternative") is None:
            a = _paren_inner(iff.child_by_field_name("condition"))
            b = _paren_inner(inner_if.child_by_field_name("condition"))
            s = inner_if.child_by_field_name("consequence")
            if a is not None and b is not None and s is not None:
                repl = (b"if (" + _txt(data, a) + b" && " + _txt(data, b)
                        + b") " + _txt(data, s))
                yield (f"&&-merge L{_line(data, iff.start_byte)}",
                       splice(data, iff.start_byte, iff.end_byte, repl).decode())


def rule_temp_inline(text, name, span):
    """Inline a single-use local temp: `T x = E; … x …` -> `… (E) …`, dropping
    the declaration. cc1 keeps calls inline in expressions where a temp inserts
    a move (cookbook Expressions). Single-use bounds the risk; scoring guards
    correctness (a wrong inline just fails to improve)."""
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    idents = _find(body, ("identifier",))
    for decl in _find(body, ("declaration",)):
        idecs = [c for c in decl.named_children if c.type == "init_declarator"]
        if len(idecs) != 1:
            continue
        idn = _descend_ident(idecs[0].child_by_field_name("declarator"))
        val = idecs[0].child_by_field_name("value")
        if idn is None or val is None:
            continue
        vid = _txt(data, idn)
        uses = [n for n in idents if n.start_byte >= decl.end_byte
                and _txt(data, n) == vid]
        if len(uses) != 1:
            continue
        use = uses[0]
        if use.start_byte > 0 and data[use.start_byte - 1:use.start_byte] == b"&":
            continue  # address-of: (E) may not be an lvalue
        repl = b"(" + _txt(data, val) + b")"
        t1 = splice(data, use.start_byte, use.end_byte, repl)
        ds = decl.start_byte
        while ds > 0 and data[ds - 1:ds] in (b" ", b"\t"):
            ds -= 1
        de = decl.end_byte + (1 if data[decl.end_byte:decl.end_byte + 1] == b"\n" else 0)
        yield (f"inline {vid.decode()} L{_line(data, decl.start_byte)}",
               splice(t1, ds, de, b"").decode())


# Types wide enough that seeding from any narrower integer is lossless, so
# rewriting E to read the seed cannot change E's value (see rule_copy_seed).
WIDE_TYPES = {b"s32", b"u32", b"int", b"unsigned", b"unsigned int", b"long",
              b"unsigned long", b"signed int", b"signed"}
INT_TYPES = WIDE_TYPES | {b"s8", b"u8", b"s16", b"u16", b"char", b"short",
                          b"signed char", b"unsigned char", b"signed short",
                          b"unsigned short", b"short int"}


def _declared_type(data, body, ident):
    """The declared type spelling of `ident` in `body` or its parameter list.

    Searches the enclosing function_definition, not just the body: parameters
    live in the DECLARATOR, so a body-only search silently reports None for
    every parameter — which made this rule sweep nothing on its first outing.
    """
    scope = body
    while scope is not None and scope.type != "function_definition":
        scope = scope.parent
    scope = scope or body
    for decl in _find(scope, ("declaration", "parameter_declaration")):
        for d in _find(decl, ("identifier",)):
            if _txt(data, d) != ident:
                continue
            spec = decl.child_by_field_name("type")
            if spec is None:
                return None
            # A pointer/array declarator means the type is not the bare spelling.
            for dd in decl.named_children:
                if dd.type in ("pointer_declarator", "array_declarator",
                               "init_declarator"):
                    inner = dd.child_by_field_name("declarator") or dd
                    if inner.type in ("pointer_declarator", "array_declarator"):
                        return None
                if dd.type in ("pointer_declarator", "array_declarator"):
                    return None
            return _txt(data, spec)
    return None


def rule_copy_seed(text, name, span):
    """Seed a local from a value its next expression reads, and make the
    expression read the SEED: `x = E(y);` -> `x = y; x = E(x);`.

    The INVERSE of rule_temp_inline, and the edit that matched AttackBowControl's
    last 15 bytes (`idx2 = n; idx2 = (idx2 << 16) >> 14;`). `local_alloc` colours
    quantities SHORTEST-LIVED-FIRST and the first one takes $v0, so when the
    target's longer chain holds $v0 while a short address temp holds $v1 but the
    draft has them swapped, no respelling of the site moves it (13 variants
    plateaued at exactly 15). Seeding births the quantity EARLIER, flips the
    order, and the copy coalesces away — leaving identical instructions.

    **The substitution is the whole point.** A seed the following expression does
    not read is DEAD, and cc1 deletes it, so `x = y; x = E(y);` is a no-op — this
    rule emitted exactly that in its first draft and would have swept nothing.
    E must be rewritten to read x.

    Safety: at the seed, x == y, so E(x) == E(y) — provided the assignment to x
    does not truncate. So this only fires when x's declared type is 32-bit and y's
    is a no-wider integer. Both types must be plain scalars (a pointer or array
    declarator disqualifies).
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for st in _find(body, ("expression_statement",)):
        asg = next((c for c in st.named_children
                    if c.type == "assignment_expression"), None)
        if asg is None:
            continue
        lhs = asg.child_by_field_name("left")
        val = asg.child_by_field_name("right")
        if lhs is None or val is None or lhs.type != "identifier":
            continue
        if _txt(data, asg)[:len(_txt(data, lhs))] != _txt(data, lhs):
            continue
        op = asg.child_by_field_name("operator")
        if op is not None and _txt(data, op) != b"=":
            continue  # compound assignment already reads x
        if not _find(val, ("binary_expression",)):
            continue  # no chain to reorder
        vid = _txt(data, lhs)
        vtype = _declared_type(data, body, vid)
        if vtype is None or vtype not in WIDE_TYPES:
            continue
        ls = data.rfind(b"\n", 0, st.start_byte) + 1
        indent = data[ls:st.start_byte]
        if indent.strip():
            continue
        seen = set()
        for src in _find(val, ("identifier",)):
            sid = _txt(data, src)
            if sid == vid or sid in seen:
                continue
            if src.parent is not None and src.parent.type == "call_expression":
                continue
            stype = _declared_type(data, body, sid)
            if stype is None or stype not in INT_TYPES:
                continue
            seen.add(sid)
            # Rewrite every read of y inside E to read the seed instead.
            expr, shift = bytearray(_txt(data, val)), 0
            base = val.start_byte
            for occ in _find(val, ("identifier",)):
                if _txt(data, occ) != sid:
                    continue
                if occ.parent is not None and occ.parent.type == "call_expression":
                    continue
                s = occ.start_byte - base + shift
                e = occ.end_byte - base + shift
                expr[s:e] = vid
                shift += len(vid) - (occ.end_byte - occ.start_byte)
            new_text = (vid + b" = " + sid + b";\n" + indent
                        + vid + b" = " + bytes(expr) + b";")
            yield (f"copy-seed {vid.decode()}={sid.decode()} "
                   f"L{_line(data, st.start_byte)}",
                   splice(data, st.start_byte, st.end_byte, new_text).decode())


def rule_binop_operand_seed(text, name, span):
    """Seed a binop's RIGHT operand into a preceding local: `x = A op B;` ->
    `t = B; x = A op t;` (and the mirror for the left operand).

    **Operand-birth order decides local_alloc's colours.** `expand` emits a binary
    op's operand loads LEFT-TO-RIGHT, so the LEFT operand's pseudo is always born
    first and is therefore the LONGER-lived quantity. local_alloc colours quantities
    SHORTEST-LIVED-FIRST, so the RIGHT operand is always coloured first and takes the
    LOWER hard register. When the target has the opposite assignment (left operand in
    the lower register), naming the RIGHT operand in a PRECEDING statement births it
    first, inverts the two lifetimes, and flips the colours — the copy coalesces away.

    Worth 15 bytes on mission_score_screen (8 instructions fixed, zero new diffs):
    `bosses = stats.stageBosses; signedValue = (stats.stageEnemies - bosses);`

    `rule_copy_seed` does NOT cover this: its pattern is `x = E(y)` — one variable,
    substituted into its own expression. This is two distinct operands, and the seed
    must be a DEDICATED LOCAL sitting exactly between the preceding statement and the
    op (a global as the temp measured a no-op; moving the seed one statement earlier
    or fencing it both cost +4 and a LENGTH MISMATCH).
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for st in _find(body, ("expression_statement",)):
        asg = next((c for c in st.named_children
                    if c.type == "assignment_expression"), None)
        if asg is None:
            continue
        op = asg.child_by_field_name("operator")
        if op is not None and _txt(data, op) != b"=":
            continue
        val = asg.child_by_field_name("right")
        lhs = asg.child_by_field_name("left")
        if val is None or lhs is None:
            continue
        # Unwrap one layer of parens so `x = (A - B);` is eligible too.
        inner = val
        while inner.type == "parenthesized_expression" and inner.named_child_count:
            inner = inner.named_children[0]
        if inner.type != "binary_expression":
            continue
        left = inner.child_by_field_name("left")
        right = inner.child_by_field_name("right")
        if left is None or right is None:
            continue
        ls = data.rfind(b"\n", 0, st.start_byte) + 1
        indent = data[ls:st.start_byte]
        if indent.strip():
            continue
        vid = _txt(data, lhs)
        for tag, operand in (("right", right), ("left", left)):
            # Only a side-effect-free operand may be hoisted above the other.
            if _find(operand, ("call_expression", "assignment_expression",
                               "update_expression")):
                continue
            otext = _txt(data, operand)
            if otext == vid or not otext.strip():
                continue
            # A bare literal has no pseudo to birth -- seeding it is a no-op.
            if operand.type in ("number_literal", "char_literal"):
                continue
            temp = b"_seed_" + re.sub(rb"[^A-Za-z0-9]", b"_", otext)[:20]
            head = _txt(data, st)[:operand.start_byte - st.start_byte]
            tail = _txt(data, st)[operand.end_byte - st.start_byte:]
            stmt = (b"s32 " + temp + b" = " + otext + b";\n" + indent
                    + head + temp + tail)
            yield (f"binop-operand-seed {tag} {temp.decode()} "
                   f"L{_line(data, st.start_byte)}",
                   splice(data, st.start_byte, st.end_byte, stmt).decode())


def rule_call_result_split(text, name, span):
    """Give every definition of a reused direct-call result its own local.

    ``T r; r = call(); use(r); r = call(); use(r);`` keeps one pseudo alive
    across all definitions, which can make old cc1 copy each return out of
    ``$v0``.  Separate single-definition locals can coalesce with the return
    register.  Unlike ``temp-inline``, this does not move a call across any
    intervening statement.

    This is deliberately atomic and conservative: the original must be a
    plain scalar local, every write must be a direct-call assignment, and each
    definition must have exactly one later use in the same compound statement
    before the next definition.  Mutually exclusive block-local definitions
    are supported; uses that escape their defining block are not.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    idents = _find(body, ("identifier",))
    assignments = _find(body, ("assignment_expression",))

    def nearest_compound(node):
        node = node.parent
        while node is not None and node is not body.parent:
            if node.type == "compound_statement":
                return (node.start_byte, node.end_byte)
            node = node.parent
        return None

    def has_ancestor(node, node_type, stop):
        node = node.parent
        while node is not None and node is not stop:
            if node.type == node_type:
                return True
            node = node.parent
        return False

    for decl in _find(body, ("declaration",)):
        type_node = decl.child_by_field_name("type")
        declarator = decl.child_by_field_name("declarator")
        if (type_node is None or declarator is None or
                declarator.type != "identifier"):
            continue
        type_text = _txt(data, type_node)
        if type_text.decode() not in TYPES:
            continue
        if re.search(rb"\b(?:const|volatile|static|register|extern)\b",
                     _txt(data, decl)):
            continue

        original = _txt(data, declarator)
        defs = []
        lhs_nodes = set()
        for assignment in assignments:
            left = assignment.child_by_field_name("left")
            right = assignment.child_by_field_name("right")
            operator = assignment.child_by_field_name("operator")
            statement = assignment.parent
            if (left is None or left.type != "identifier" or
                    _txt(data, left) != original):
                continue
            lhs_nodes.add((left.start_byte, left.end_byte))
            if (right is None or right.type != "call_expression" or
                    operator is None or _txt(data, operator) != b"=" or
                    statement is None or
                    statement.type != "expression_statement" or
                    statement.parent is None or
                    statement.parent.type != "compound_statement"):
                defs = []
                break
            defs.append((assignment, statement, right))
        if len(defs) < 2:
            continue
        defs.sort(key=lambda item: item[0].start_byte)

        # A shadow declaration, update, address-take, composite write, or use
        # inside a call RHS makes the simple definition/use partition unsafe.
        uses = []
        unsafe = False
        for ident in idents:
            if _txt(data, ident) != original:
                continue
            key = (ident.start_byte, ident.end_byte)
            if key == (declarator.start_byte, declarator.end_byte) or key in lhs_nodes:
                continue
            if has_ancestor(ident, "declaration", body):
                unsafe = True
                break
            parent = ident.parent
            if (parent is not None and parent.type == "update_expression"):
                unsafe = True
                break
            if (parent is not None and parent.type == "pointer_expression" and
                    _txt(data, parent).lstrip().startswith(b"&")):
                unsafe = True
                break
            # Reject identifiers nested anywhere in another assignment LHS.
            ancestor = ident.parent
            while ancestor is not None and ancestor is not body:
                if ancestor.type == "assignment_expression":
                    left = ancestor.child_by_field_name("left")
                    if (left is not None and
                            left.start_byte <= ident.start_byte < left.end_byte):
                        unsafe = True
                    break
                ancestor = ancestor.parent
            if unsafe:
                break
            uses.append(ident)
        if unsafe:
            continue

        paired = []
        for index, (assignment, statement, right) in enumerate(defs):
            next_start = (defs[index + 1][0].start_byte
                          if index + 1 < len(defs) else body.end_byte)
            region_uses = [use for use in uses
                           if assignment.end_byte < use.start_byte < next_start]
            if (len(region_uses) != 1 or
                    nearest_compound(region_uses[0]) !=
                    (statement.parent.start_byte, statement.parent.end_byte)):
                paired = []
                break
            paired.append((statement, right, region_uses[0]))
        if len(paired) != len(defs) or len(uses) != len(defs):
            continue

        replacements = [(decl.start_byte, decl.end_byte, b"")]
        existing = {_txt(data, ident) for ident in idents}
        collision = False
        for index, (statement, right, use) in enumerate(paired):
            fresh = b"_match_" + original + b"_" + str(index).encode()
            if fresh in existing:
                collision = True
                break
            replacement = type_text + b" " + fresh + b" = " + _txt(data, right) + b";"
            replacements.append((statement.start_byte, statement.end_byte,
                                 replacement))
            replacements.append((use.start_byte, use.end_byte, fresh))
        if collision:
            continue

        candidate = data
        for start, end, replacement in sorted(replacements, reverse=True):
            candidate = splice(candidate, start, end, replacement)
        yield (f"split call-result {original.decode()} x{len(defs)} "
               f"L{_line(data, decl.start_byte)}", candidate.decode())


def rule_param_width(text, name, span):
    """Flip plain integer function parameters across width/signedness.

    Decompilers often infer a parameter's type only from its narrowing stores.
    The entry allocation can still prove that the original parameter stayed
    full-width.  Keep pointers/arrays/function declarators out of this rule;
    authoritative scoring decides among the bounded scalar alternatives.  A
    parameter may need width and signedness changed together (retail SetSmokeS
    uses ``u16`` where the demo prototype says ``int``), so parameters also
    get the two adjacent-width/opposite-sign "diagonal" candidates directly;
    greedy search must not depend on an improving intermediate type.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    function = body.parent
    while function is not None and function.type != "function_definition":
        function = function.parent
    if function is None:
        return
    declarator = function.child_by_field_name("declarator")
    if declarator is None:
        return
    for parameter in _find(declarator, ("parameter_declaration",)):
        type_node = parameter.child_by_field_name("type")
        param_declarator = parameter.child_by_field_name("declarator")
        if (type_node is None or param_declarator is None or
                param_declarator.type != "identifier"):
            continue
        current = _txt(data, type_node).decode()
        if current not in TYPES:
            continue
        width, signed = TYPES[current]
        identifier = _txt(data, param_declarator).decode()
        targets = list(flip_targets(width, signed))
        width_index = WIDTH_ORDER.index(width)
        for delta in (-1, 1):
            adjacent = width_index + delta
            if 0 <= adjacent < len(WIDTH_ORDER):
                diagonal = (WIDTH_ORDER[adjacent], not signed)
                if diagonal not in targets:
                    targets.append(diagonal)
        for next_width, next_signed in targets:
            replacement = CANON[(next_width, next_signed)].encode()
            yield (
                f"param {identifier}: {current}→{replacement.decode()}",
                splice(data, type_node.start_byte, type_node.end_byte,
                       replacement).decode(),
            )


def rule_late_pointer_direct(text, name, span):
    """Drop a repeated pointer-global assignment and use the global directly.

    ``p = Global; ... p = Global; p->x ...`` gives one automatic pointer two
    definitions. When the second region has no mutating calls or writes to
    ``Global``, spelling its member accesses as ``Global->x`` preserves the
    runtime value while letting CSE make a fresh block-local pointer pseudo.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    pointer_names = set()
    for decl in _find(body, ("declaration",)):
        if re.search(rb"\bvolatile\b", _txt(data, decl)):
            continue
        for child in decl.named_children:
            declarator = (child.child_by_field_name("declarator")
                          if child.type == "init_declarator" else child)
            if declarator is None or declarator.type != "pointer_declarator":
                continue
            identifier = _descend_ident(declarator)
            if identifier is not None:
                pointer_names.add(_txt(data, identifier))

    statements = sorted(_find(body, ("expression_statement",)),
                        key=lambda node: node.start_byte)
    assignments = []
    for statement in statements:
        parsed = _plain_assignment(data, statement)
        if parsed is not None:
            assignments.append((statement, parsed[0], _unparen(parsed[1])))

    identifiers = _find(body, ("identifier",))
    calls = _find(body, ("call_expression",))
    all_assignments = _find(body, ("assignment_expression",))
    updates = _find(body, ("update_expression",))

    for index, (statement, local, source) in enumerate(assignments):
        if (local not in pointer_names or source is None or
                source.type != "identifier"):
            continue
        global_name = _txt(data, source)
        if global_name == local:
            continue
        if not any(previous_local == local and previous_source is not None and
                   previous_source.type == "identifier" and
                   _txt(data, previous_source) == global_name
                   for _previous, previous_local, previous_source
                   in assignments[:index]):
            continue

        next_write = next((later.start_byte
                           for later, later_local, _later_source
                           in assignments[index + 1:]
                           if later_local == local), body.end_byte)
        uses = [identifier for identifier in identifiers
                if statement.end_byte <= identifier.start_byte < next_write and
                _txt(data, identifier) == local]
        if not uses:
            continue
        if any(use.parent is None or use.parent.type != "field_expression" or
               use.parent.child_by_field_name("argument") != use
               for use in uses):
            continue
        region_end = max(use.end_byte for use in uses)
        def mutating_call(call):
            function = call.child_by_field_name("function")
            return not (function is not None and function.type == "identifier" and
                        _txt(data, function) == b"__builtin_abs")

        if any(statement.end_byte <= call.start_byte < region_end and
               mutating_call(call) for call in calls):
            continue

        writes_global = False
        for assignment in all_assignments:
            if not (statement.end_byte <= assignment.start_byte < region_end):
                continue
            left = assignment.child_by_field_name("left")
            if (left is not None and left.type == "identifier" and
                    _txt(data, left) == global_name):
                writes_global = True
                break
        if writes_global or any(
                statement.end_byte <= update.start_byte < region_end and
                re.search(rb"\b" + re.escape(global_name) + rb"\b",
                          _txt(data, update))
                for update in updates):
            continue

        rewritten = data
        for use in sorted(uses, key=lambda node: node.start_byte, reverse=True):
            rewritten = splice(rewritten, use.start_byte, use.end_byte,
                               global_name)
        delete_start = statement.start_byte
        while (delete_start > 0 and
               data[delete_start - 1:delete_start] in (b" ", b"\t")):
            delete_start -= 1
        delete_end = statement.end_byte
        if data[delete_end:delete_end + 1] == b"\n":
            delete_end += 1
        yield (
            f"late-pointer-direct {local.decode()}={global_name.decode()} "
            f"L{_line(data, statement.start_byte)}",
            splice(rewritten, delete_start, delete_end, b"").decode(),
        )


def rule_literal_indirect_inline(text, name, span):
    """Atomically inline a byte literal and indirect-call field temporaries.

    Old cc1 can allocate these two source identities as one coupled system:
    changing only the constant rematerialization or only the indirect target
    changes length/cross-jumping and scores worse. Recognize a single-assignment
    ``u8 local = LITERAL`` shape (split declaration is allowed) plus every
    block-local ``void (*proc)(...) = base->field`` used only by a null test and
    direct call. Remove both temp families in one candidate.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    def delete_span(node):
        start = data.rfind(b"\n", 0, node.start_byte) + 1
        end = node.end_byte
        if data[end:end + 1] == b"\n":
            end += 1
        return start, end

    all_identifiers = _find(body, ("identifier",))
    all_assignments = _find(body, ("assignment_expression",))
    constant_sites = []
    for declaration in _find(body, ("declaration",)):
        match = re.fullmatch(
            rb"\s*(?:u8|unsigned\s+char)\s+([A-Za-z_]\w*)\s*;\s*",
            _txt(data, declaration),
        )
        if not match:
            continue
        local = match.group(1)
        writes = []
        for assignment in all_assignments:
            left = _unparen(assignment.child_by_field_name("left"))
            if (left is not None and left.type == "identifier" and
                    _txt(data, left) == local):
                writes.append(assignment)
        if len(writes) != 1:
            continue
        assignment = writes[0]
        right = _unparen(assignment.child_by_field_name("right"))
        statement = assignment.parent
        if (right is None or right.type != "number_literal" or
                statement is None or statement.type != "expression_statement"):
            continue
        try:
            value = int(_txt(data, right), 0)
        except ValueError:
            continue
        if not 0 <= value <= 0xff:
            continue
        uses = [identifier for identifier in all_identifiers
                if _txt(data, identifier) == local and
                not (declaration.start_byte <= identifier.start_byte <
                     declaration.end_byte) and
                not (assignment.start_byte <= identifier.start_byte <
                     assignment.end_byte)]
        if len(uses) < 2 or any(use.start_byte < assignment.end_byte for use in uses):
            continue
        def unsafe_constant_use(use):
            node = use.parent
            while node is not None and node is not body:
                if node.type in {"sizeof_expression", "alignof_expression",
                                 "update_expression"}:
                    return True
                if node.type == "pointer_expression":
                    operator = node.child_by_field_name("operator")
                    if operator is not None and _txt(data, operator) == b"&":
                        return True
                node = node.parent
            return False
        if any(unsafe_constant_use(use) for use in uses):
            continue
        constant_sites.append((
            local, _txt(data, right), declaration, statement, uses,
        ))

    proc_sites = []
    for declaration in _find(body, ("declaration",)):
        match = re.search(rb"\(\s*\*\s*([A-Za-z_]\w*)\s*\)",
                          _txt(data, declaration))
        if not match or _txt(data, declaration).count(b";") != 1:
            continue
        local = match.group(1)
        scope = declaration.parent
        if scope is None or scope.type != "compound_statement":
            continue
        writes = []
        for assignment in _find(scope, ("assignment_expression",)):
            left = _unparen(assignment.child_by_field_name("left"))
            if (left is not None and left.type == "identifier" and
                    _txt(data, left) == local):
                writes.append(assignment)
        if len(writes) != 1:
            continue
        assignment = writes[0]
        source = _unparen(assignment.child_by_field_name("right"))
        statement = assignment.parent
        if (source is None or source.type != "field_expression" or
                statement is None or statement.type != "expression_statement" or
                assignment.start_byte < declaration.end_byte or
                _find(source, ("call_expression", "update_expression",
                               "assignment_expression"))):
            continue
        uses = [identifier for identifier in _find(scope, ("identifier",))
                if _txt(data, identifier) == local and
                not (declaration.start_byte <= identifier.start_byte <
                     declaration.end_byte) and
                not (assignment.start_byte <= identifier.start_byte <
                     assignment.end_byte)]
        if not uses:
            continue
        saw_call = False
        saw_test = False
        valid = True
        for use in uses:
            parent = use.parent
            if (parent is not None and parent.type == "call_expression" and
                    parent.child_by_field_name("function") == use):
                saw_call = True
                continue
            if parent is not None and parent.type == "binary_expression":
                operator = parent.child_by_field_name("operator")
                left = _unparen(parent.child_by_field_name("left"))
                right = _unparen(parent.child_by_field_name("right"))
                other = right if left == use else left if right == use else None
                if (operator is not None and _txt(data, operator) in {b"==", b"!="} and
                        other is not None and other.type == "number_literal" and
                        int(_txt(data, other), 0) == 0):
                    saw_test = True
                    continue
            valid = False
            break
        if valid and saw_call and saw_test:
            proc_sites.append((
                local, _txt(data, source), declaration, statement, uses,
            ))

    if not constant_sites or not proc_sites:
        return
    changed_lines = []
    edits = []
    for local, literal, declaration, statement, uses in constant_sites:
        changed_lines.extend((_line(data, declaration.start_byte),
                              _line(data, statement.start_byte)))
        changed_lines.extend(_line(data, use.start_byte) for use in uses)
        edits.extend((use.start_byte, use.end_byte, literal) for use in uses)
        edits.append((*delete_span(statement), b""))
        edits.append((*delete_span(declaration), b""))
    for local, source, declaration, statement, uses in proc_sites:
        changed_lines.extend((_line(data, declaration.start_byte),
                              _line(data, statement.start_byte)))
        changed_lines.extend(_line(data, use.start_byte) for use in uses)
        edits.extend((use.start_byte, use.end_byte, source) for use in uses)
        edits.append((*delete_span(statement), b""))
        edits.append((*delete_span(declaration), b""))
    if not any(_guided_site(line) for line in changed_lines):
        return
    rewritten = data
    last_start = len(data) + 1
    for start, end, replacement in sorted(edits, reverse=True):
        if end > last_start:
            return
        rewritten = splice(rewritten, start, end, replacement)
        last_start = start
    constants = ",".join(site[0].decode() for site in constant_sites)
    procs = ",".join(site[0].decode() for site in proc_sites)
    yield (
        f"literal-indirect-inline {constants}/{procs}",
        rewritten.decode(),
    )


def rule_call_arg_pair_inline(text, name, span):
    """Inline two adjacent same-call results into one consumer call.

    ``x = rand(); y = rand(); sink(F(x), F(y));`` can serialize both result
    transformations before ``sink``.  Inlining the pair atomically lets cc1
    keep the first transformed argument live across the second call and place
    its final shift in that call's delay slot.  Restrict this to two distinct,
    nonvolatile, non-address-taken automatic locals, byte-identical producer
    calls, one use apiece in the immediately following standalone consumer,
    and no later uses.  The pair is one candidate because inlining only one
    producer changes call order and cannot expose the intended pipeline.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)
    identifiers = _find(body, ("identifier",))
    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for first, second, consumer in zip(
                statements, statements[1:], statements[2:]):
            one = _plain_assignment(data, first)
            two = _plain_assignment(data, second)
            if one is None or two is None or one[0] == two[0]:
                continue
            names = (one[0], two[0])
            producers = (_unparen(one[1]), _unparen(two[1]))
            if (any(local not in locals_ for local in names) or
                    any(producer is None or producer.type != "call_expression"
                        for producer in producers) or
                    _txt(data, producers[0]).strip() !=
                    _txt(data, producers[1]).strip()):
                continue
            if (data[first.end_byte:second.start_byte].strip() or
                    data[second.end_byte:consumer.start_byte].strip() or
                    consumer.type != "expression_statement"):
                continue
            expressions = [child for child in consumer.named_children
                           if child.type != "comment"]
            if len(expressions) != 1 or expressions[0].type != "call_expression":
                continue
            # Before substitution the consumer itself must be the only call;
            # nested calls could reorder unrelated side effects.
            if len(_find(consumer, ("call_expression",))) != 1:
                continue
            uses = {}
            valid = True
            for local, definition in zip(names, (first, second)):
                if re.search(rb"&\s*" + re.escape(local) + rb"\b", body_text):
                    valid = False
                    break
                consumer_uses = [ident for ident in _find(consumer, ("identifier",))
                                 if _txt(data, ident) == local]
                later = [ident for ident in identifiers
                         if ident.start_byte >= definition.start_byte and
                         _txt(data, ident) == local]
                # assignment LHS + exactly one consumer use, with no later use
                if len(consumer_uses) != 1 or len(later) != 2:
                    valid = False
                    break
                uses[local] = consumer_uses[0]
            if not valid:
                continue
            line = _line(data, first.start_byte)
            if not _guided_site(line):
                continue
            raw = data[consumer.start_byte:consumer.end_byte]
            replacements = []
            for local, producer in zip(names, producers):
                use = uses[local]
                replacements.append((use.start_byte - consumer.start_byte,
                                     use.end_byte - consumer.start_byte,
                                     _txt(data, producer).strip()))
            for start, end, replacement in sorted(replacements, reverse=True):
                raw = raw[:start] + replacement + raw[end:]
            yield (
                f"call-arg-pair-inline {names[0].decode()}/{names[1].decode()} "
                f"L{line}",
                splice(data, first.start_byte, consumer.end_byte, raw).decode(),
            )


def _unparen(node):
    """Strip expression parentheses while retaining the underlying AST node."""
    while node is not None and node.type == "parenthesized_expression":
        children = [c for c in node.named_children if c.type != "comment"]
        if len(children) != 1:
            break
        node = children[0]
    return node


def _binary_operator(data, node):
    node = _unparen(node)
    if node is None or node.type != "binary_expression":
        return None
    op = node.child_by_field_name("operator")
    return _txt(data, op) if op is not None else None


def _plus_leaves(data, node):
    """Flatten one parenthesized top-level `+` tree, in source leaf order."""
    node = _unparen(node)
    if _binary_operator(data, node) != b"+":
        return [node]
    return (_plus_leaves(data, node.child_by_field_name("left")) +
            _plus_leaves(data, node.child_by_field_name("right")))


def _integer_constant(node):
    """True for a literal integer leaf, including a unary +/- literal."""
    node = _unparen(node)
    if node is None:
        return False
    if node.type == "number_literal":
        return True
    if node.type != "unary_expression":
        return False
    op = node.child_by_field_name("operator")
    arg = node.child_by_field_name("argument")
    if op is None or arg is None:
        return False
    return op.type in {"+", "-"} and _unparen(arg).type == "number_literal"


def rule_plus_group(text, name, span):
    """Enumerate the six trees for two ordered expressions plus one constant.

    gcc-2.8.1 fold does not treat source-equivalent additive trees alike:
    `A + (B + C)` becomes `(A + C) + B`, whereas `(A + C) + B` can become
    `A + (B + C)`.  That decides which live value receives an `addiu`, and can
    also decide the final `addu` destination.  Keep the two nonconstant leaves
    in their original relative order (so calls retain their source order), move
    only the side-effect-free literal, and try both associations.  Exactly
    three leaves and one literal keeps the search bounded and explainable.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for expr in _find(body, ("binary_expression",)):
        if _binary_operator(data, expr) != b"+":
            continue
        leaves = _plus_leaves(data, expr)
        if len(leaves) != 3:
            continue
        constants = [n for n in leaves if _integer_constant(n)]
        if len(constants) != 1:
            continue
        line = _line(data, expr.start_byte)
        if not _guided_site(line):
            continue
        constant = constants[0]
        values = [n for n in leaves if n is not constant]
        # Preserve the relative order of all possibly effectful expressions;
        # only the literal migrates among them.
        orders = (
            (values[0], values[1], constant, "const-last"),
            (values[0], constant, values[1], "const-middle"),
            (constant, values[0], values[1], "const-first"),
        )
        original_order = tuple(n.start_byte for n in leaves)
        raw = _unparen(expr)
        left = raw.child_by_field_name("left")
        right = raw.child_by_field_name("right")
        original_assoc = None
        if len(_plus_leaves(data, left)) == 2 and len(_plus_leaves(data, right)) == 1:
            original_assoc = "left"
        elif len(_plus_leaves(data, left)) == 1 and len(_plus_leaves(data, right)) == 2:
            original_assoc = "right"
        for a, b, c, position in orders:
            order = (a.start_byte, b.start_byte, c.start_byte)
            parts = [_txt(data, n) for n in (a, b, c)]
            variants = (
                ("left", b"((" + parts[0] + b") + (" + parts[1] +
                 b")) + (" + parts[2] + b")"),
                ("right", b"(" + parts[0] + b") + ((" + parts[1] +
                 b") + (" + parts[2] + b"))"),
            )
            for assoc, repl in variants:
                if order == original_order and assoc == original_assoc:
                    continue
                yield (f"plus-group {position}/{assoc} L{line}",
                       splice(data, expr.start_byte, expr.end_byte, repl).decode())


def rule_add_prefix_temp(text, name, span):
    """Name a promoted two-term seam inside a narrowed three-term sum.

    `(s16)(A + B + C)` lets fold/cse observe that the whole expression is used
    only narrowly; old cc1 may consequently use unsigned halfword loads and a
    different add destination for every leaf.  A signed 32-bit temp for A+B
    preserves that promoted prefix as its own allocno:

        { s32 _match_sum; _match_sum = A + B;
          use((s16)(_match_sum + C)); }

    This was the 97 -> 57 byte step in ControlHumanoid.  Restrict candidates to
    exactly three side-effect-free leaves under an explicit 16-bit cast and a
    standalone expression statement.  The nested block makes the declaration
    legal C89 and bounds its lifetime; authoritative scoring decides whether
    the extra allocno is useful.  Both contiguous seams are tried.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    effects = ("call_expression", "assignment_expression", "update_expression")
    allowed_types = {b"s16", b"short", b"signed short"}
    for cast in _find(body, ("cast_expression", "call_expression")):
        cast_type = cast.child_by_field_name("type")
        value = cast.child_by_field_name("value")
        pseudo_cast = False
        if cast.type == "call_expression":
            # tree-sitter has no typedef table, so `(s16)(expr)` is parsed as
            # a call through parenthesized identifier `s16`. Recognise only
            # our explicit narrow type names; ordinary function calls remain
            # untouched.
            function = cast.child_by_field_name("function")
            arguments = cast.child_by_field_name("arguments")
            function_inner = (_paren_inner(function)
                              if function is not None and
                              function.type == "parenthesized_expression"
                              else None)
            argument_values = ([child for child in arguments.named_children
                                if child.type != "comment"]
                               if arguments is not None else [])
            if function_inner is None or len(argument_values) != 1:
                continue
            cast_type = function_inner
            value = argument_values[0]
            pseudo_cast = True
        if cast_type is None or value is None:
            continue
        normalized_type = b" ".join(_txt(data, cast_type).split())
        if normalized_type not in allowed_types:
            continue
        leaves = _plus_leaves(data, value)
        if len(leaves) != 3 or any(_find(leaf, effects) for leaf in leaves):
            continue
        statement = cast
        while (statement is not None and
               statement.type not in ("expression_statement", "return_statement")):
            statement = statement.parent
        if (statement is None or statement.parent is None or
                statement.parent.type != "compound_statement"):
            continue
        # One outer call or assignment is fine. Nested calls/assignments would
        # make hoisting the pure prefix observably reorder side effects.
        calls = _find(statement, ("call_expression",))
        if pseudo_cast:
            calls = [call for call in calls if call != cast]
        assignments = _find(statement, ("assignment_expression",))
        if len(calls) > 1 or len(assignments) > 1 or \
                _find(statement, ("update_expression",)):
            continue
        line = _line(data, cast.start_byte)
        if not _guided_site(line):
            continue
        temp = "_match_sum"
        suffix = 2
        while re.search(rf"\b{re.escape(temp)}\b", text):
            temp = f"_match_sum{suffix}"
            suffix += 1
        leaf_text = [_txt(data, leaf).strip() for leaf in leaves]
        variants = (
            ("prefix01", leaf_text[0] + b" + " + leaf_text[1],
             temp.encode() + b" + " + leaf_text[2]),
            ("suffix12", leaf_text[1] + b" + " + leaf_text[2],
             leaf_text[0] + b" + " + temp.encode()),
        )
        line_start = data.rfind(b"\n", 0, statement.start_byte) + 1
        indent = data[line_start:statement.start_byte]
        if indent.strip():
            continue
        raw_statement = data[statement.start_byte:statement.end_byte]
        relative_start = value.start_byte - statement.start_byte
        relative_end = value.end_byte - statement.start_byte
        for tag, prefix, replacement in variants:
            changed_statement = (raw_statement[:relative_start] + replacement +
                                 raw_statement[relative_end:])
            repl = (b"{\n" + indent + b"    s32 " + temp.encode() + b";\n" +
                    indent + b"    " + temp.encode() + b" = " + prefix + b";\n" +
                    indent + b"    " + changed_statement + b"\n" + indent + b"}")
            yield (f"add-prefix-temp {tag} L{line}",
                   splice(data, statement.start_byte, statement.end_byte,
                          repl).decode())


def rule_abs_ge(text, name, span):
    """Rewrite an abs ternary to its GE form: `(x < 0) ? -x : x` -> `(x >= 0) ? x : -x`.

    Both are abs(x), so it is ALWAYS semantically safe; scoring decides the bytes.
    Only the GE spelling folds to cc1's `abssi2` insn (one `bgez;move;negu`, negu of
    the copy); the LT spelling takes the branchy `negu dst,src` path. Cookbook:
    "An abs assigned to a variable reaches abssi2 only as the GE ternary." Discovered
    on SoundEx/UpdateMotion — now applied mechanically so no agent re-derives it."""
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for c in _find(body, ("conditional_expression",)):
        cond = c.child_by_field_name("condition")
        then = c.child_by_field_name("consequence")
        els = c.child_by_field_name("alternative")
        if cond is None or then is None or els is None:
            return
        inner = _paren_inner(cond) if cond.type == "parenthesized_expression" else cond
        if inner is None or inner.type != "binary_expression":
            continue
        op = inner.child_by_field_name("operator")
        lhs = inner.child_by_field_name("left")
        rhs = inner.child_by_field_name("right")
        if op is None or lhs is None or rhs is None:
            continue
        # match `X < 0` with consequence `-X` and alternative `X`
        if _txt(data, op) != b"<" or _txt(data, rhs).strip() != b"0":
            continue
        x = _txt(data, lhs).strip()
        neg = _txt(data, then).strip()
        pos = _txt(data, els).strip()
        if pos != x or neg not in (b"-" + x, b"- " + x):
            continue
        # build `(X >= 0) ? X : -X` preserving the original paren style of cond
        newcond = _txt(data, lhs) + b" >= 0"
        if cond.type == "parenthesized_expression":
            newcond = b"(" + newcond + b")"
        repl = newcond + b" ? " + x + b" : -" + x
        yield (f"abs->GE {x.decode()} L{_line(data, c.start_byte)}",
               splice(data, c.start_byte, c.end_byte, repl).decode())


def rule_builtin_abs(text, name, span):
    """Make either common absolute-value spelling an explicit builtin.

    The matching build deliberately passes ``-fno-builtin`` to cc1, so an
    ordinary library call can no longer fold to the target's inline
    ``bgez/move/negu`` sequence.  The explicit builtin restores that source
    choice locally without weakening the translation unit's compiler flags.

    Also recognize ``x = value; ...; if (x < 0) x = -x;`` for a nonvolatile
    automatic local.  Replacing the first assignment and deleting the later
    sign fix keeps ``value``'s evaluation point unchanged; it crosses only
    call-free expression statements that do not mention ``x``.  This second
    form is the ProcMiscDoor register-allocation lever: the builtin's
    internal value can be locally allocated before the DoorData index chain,
    unlike a named pseudo spanning the negative arm.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    local_names = _nonvolatile_local_names(data, body)
    if b"abs" in local_names:
        return
    for call in _find(body, ("call_expression",)):
        function = call.child_by_field_name("function")
        arguments = call.child_by_field_name("arguments")
        if (function is None or function.type != "identifier" or
                _txt(data, function) != b"abs" or arguments is None):
            continue
        values = [child for child in arguments.named_children
                  if child.type != "comment"]
        if len(values) != 1:
            continue
        line = _line(data, call.start_byte)
        if not _guided_site(line):
            continue
        yield (
            f"abs->__builtin_abs L{line}",
            splice(data, function.start_byte, function.end_byte,
                   b"__builtin_abs").decode(),
        )

    def one_statement(node):
        if node is None:
            return None
        if node.type != "compound_statement":
            return node
        children = [child for child in node.named_children
                    if child.type != "comment"]
        return children[0] if len(children) == 1 else None

    for iff in _find(body, ("if_statement",)):
        if iff.child_by_field_name("alternative") is not None:
            continue
        condition = _unparen(iff.child_by_field_name("condition"))
        consequence = one_statement(iff.child_by_field_name("consequence"))
        if (condition is None or condition.type != "binary_expression" or
                consequence is None):
            continue
        operator = condition.child_by_field_name("operator")
        left = condition.child_by_field_name("left")
        right = _unparen(condition.child_by_field_name("right"))
        if (operator is None or _txt(data, operator) != b"<" or
                left is None or left.type != "identifier" or
                right is None or right.type != "number_literal" or
                _txt(data, right).strip() != b"0"):
            continue
        local = _txt(data, left)
        if local not in local_names:
            continue
        update = _plain_assignment(data, consequence)
        if update is None or update[0] != local:
            continue
        negated = _txt(data, _unparen(update[1])).strip()
        if negated not in (b"-" + local, b"- " + local):
            continue

        parent = iff.parent
        if parent is None or parent.type != "compound_statement":
            continue
        siblings = [child for child in parent.named_children
                    if child.type != "comment"]
        try:
            index = siblings.index(iff)
        except ValueError:
            continue
        producer = None
        for statement in reversed(siblings[:index]):
            assignment = _plain_assignment(data, statement)
            mentions = any(_txt(data, ident) == local
                           for ident in _find(statement, ("identifier",)))
            if assignment is not None and assignment[0] == local:
                producer = assignment[1]
                break
            # Moving the sign normalization earlier is semantics-preserving
            # only across straight-line expressions.  Calls and control-flow
            # could exit before the old if (or observe undefined INT_MIN
            # negation on a path that previously skipped it).
            if (mentions or statement.type != "expression_statement" or
                    _find(statement, ("call_expression",))):
                break
        if producer is None or b"__builtin_abs" in _txt(data, producer):
            continue
        line = _line(data, iff.start_byte)
        if not (_guided_site(line) or
                _guided_site(_line(data, producer.start_byte))):
            continue

        # Remove the later if first, so the earlier producer's byte offsets
        # remain valid for the second splice.
        rewritten = splice(data, iff.start_byte, iff.end_byte, b"")
        replacement = b"__builtin_abs(" + _txt(data, producer).strip() + b")"
        rewritten = splice(rewritten, producer.start_byte, producer.end_byte,
                           replacement)
        yield (f"sign-fix->__builtin_abs {local.decode()} L{line}",
               rewritten.decode())


def rule_or_inplace(text, name, span):
    """Rewrite `X = X <op> C;` to the compound `X <op>= C;` for | & + - ^.

    cc1 colours the expression form's fresh temp differently from the in-place
    compound (cookbook: "`x |= C` vs `x = x | C` is a local-alloc lever"). Same
    value; scoring decides. Only when the LHS token equals the binary's left operand
    token exactly (a plain lvalue, no side effects to duplicate)."""
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    COMPOUND = {b"|": b"|=", b"&": b"&=", b"+": b"+=", b"-": b"-=", b"^": b"^="}
    for a in _find(body, ("assignment_expression",)):
        op = a.child_by_field_name("operator")
        lhs = a.child_by_field_name("left")
        rhs = a.child_by_field_name("right")
        if op is None or _txt(data, op) != b"=" or lhs is None or rhs is None:
            continue
        if rhs.type != "binary_expression":
            continue
        bop = rhs.child_by_field_name("operator")
        bl = rhs.child_by_field_name("left")
        br = rhs.child_by_field_name("right")
        if bop is None or bl is None or br is None:
            continue
        comp = COMPOUND.get(_txt(data, bop))
        if comp is None:
            continue
        lt = _txt(data, lhs).strip()
        if _txt(data, bl).strip() != lt:
            continue
        # a bare identifier / field / index LHS only (no call, no deref-with-side-effect)
        if any(ch in lt for ch in (b"(", b"++", b"--")):
            continue
        repl = lt + b" " + comp + b" " + _txt(data, br).strip()
        yield (f"{lt.decode()} {comp.decode()} L{_line(data, a.start_byte)}",
               splice(data, a.start_byte, a.end_byte, repl).decode())


def rule_initialized_global_compound(text, name, span):
    """Split ``Global = A + B`` into initialization plus compound addition.

    A plain commutative expression gives combine/local allocation freedom to
    choose either operand as the destination.  Original source can instead
    encode the desired accumulator identity explicitly::

        Global = A;       /* or initialize from B */
        Global += B;

    AttackLong's last three bytes required ``AttackActionCount = GameClock``
    followed by ``+= EngageLevel * 10``; a compound update without the
    initializing store was not enough.  Emit both operand orders, guided and
    conservatively: the destination must be a nonvolatile file-scope extern,
    neither pure operand may mention it, and calls, updates, dereferences,
    fields, subscripts, division, and modulo are rejected so the earlier store
    cannot become observable through the second expression.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    root = _TS.parse(data).root_node
    globals_ = set()
    volatile_globals = set()
    for declaration in root.named_children:
        if declaration.type != "declaration":
            continue
        raw = _txt(data, declaration)
        if re.search(rb"\bextern\b", raw) is None:
            continue
        names = set()
        function = False
        for child in declaration.named_children:
            if child.type == "function_declarator" or _find(
                    child, ("function_declarator",)):
                function = True
                break
            if child.type == "init_declarator":
                identifier = _descend_ident(child.child_by_field_name("declarator"))
            elif child.type in ("identifier", "pointer_declarator",
                                "array_declarator"):
                identifier = _descend_ident(child)
            else:
                identifier = None
            if identifier is not None:
                names.add(_txt(data, identifier))
        if function:
            continue
        globals_.update(names)
        if re.search(rb"\bvolatile\b", raw):
            volatile_globals.update(names)

    locals_ = _nonvolatile_local_names(data, body)
    forbidden = (
        "assignment_expression", "call_expression", "conditional_expression",
        "field_expression", "pointer_expression", "subscript_expression",
        "update_expression",
    )

    def safe_operand(node, destination):
        if node is None or _find(node, forbidden):
            return False
        identifiers = {_txt(data, identifier)
                       for identifier in _find(node, ("identifier",))}
        if destination in identifiers or identifiers & volatile_globals:
            return False
        for binary in _find(node, ("binary_expression",)):
            operator = binary.child_by_field_name("operator")
            if operator is not None and _txt(data, operator) in {b"/", b"%"}:
                return False
        return True

    for statement in _find(body, ("expression_statement",)):
        expressions = [child for child in statement.named_children
                       if child.type != "comment"]
        if len(expressions) != 1 or expressions[0].type != "assignment_expression":
            continue
        assignment = expressions[0]
        operator = assignment.child_by_field_name("operator")
        lhs = assignment.child_by_field_name("left")
        rhs = _unparen(assignment.child_by_field_name("right"))
        if (operator is None or _txt(data, operator) != b"=" or lhs is None or
                lhs.type != "identifier" or rhs is None or
                rhs.type != "binary_expression"):
            continue
        destination = _txt(data, lhs)
        rhs_operator = rhs.child_by_field_name("operator")
        left = rhs.child_by_field_name("left")
        right = rhs.child_by_field_name("right")
        if (destination not in globals_ or destination in volatile_globals or
                destination in locals_ or rhs_operator is None or
                _txt(data, rhs_operator) != b"+" or
                not safe_operand(left, destination) or
                not safe_operand(right, destination)):
            continue
        line = _line(data, statement.start_byte)
        if not _guided_site(line):
            continue
        indent = _indent_at(data, statement.start_byte)
        for first, second, tag in ((left, right, "left"),
                                   (right, left, "right")):
            replacement = (
                destination + b" = " + _txt(data, first).strip() + b";\n" +
                indent + destination + b" += " +
                _txt(data, second).strip() + b";"
            )
            yield (
                f"initialized-global-compound {destination.decode()} "
                f"from-{tag} L{line}",
                splice(data, statement.start_byte, statement.end_byte,
                       replacement).decode(),
            )


def _nonvolatile_local_names(data, body):
    """Plain local identifiers safe for statement-splitting rewrites.

    Parameters/globals are deliberately excluded: two stores to either can be
    observable, while a nonvolatile automatic local has no observation point
    between adjacent statements.  If a name is ever declared volatile in the
    function, exclude every same-spelled declaration conservatively.
    """
    names = set()
    volatile = set()
    for decl in _find(body, ("declaration",)):
        found = set()
        for child in decl.named_children:
            if child.type == "init_declarator":
                ident = _descend_ident(child.child_by_field_name("declarator"))
            elif child.type in ("identifier", "pointer_declarator",
                                "array_declarator"):
                ident = _descend_ident(child)
            else:
                ident = None
            if ident is not None:
                found.add(_txt(data, ident))
        names.update(found)
        if re.search(rb"\bvolatile\b", _txt(data, decl)):
            volatile.update(found)
    return names - volatile


def _nonvolatile_function_local_names(data, body):
    """Unshadowed, nonvolatile locals declared directly in `body`."""
    safe = _nonvolatile_local_names(data, body)
    direct = set()
    shadowed = set()
    for declaration in _find(body, ("declaration",)):
        found = set()
        for child in declaration.named_children:
            if child.type == "init_declarator":
                ident = _descend_ident(child.child_by_field_name("declarator"))
            elif child.type in ("identifier", "pointer_declarator",
                                "array_declarator"):
                ident = _descend_ident(child)
            else:
                ident = None
            if ident is not None:
                found.add(_txt(data, ident))
        if declaration.parent == body:
            direct.update(found)
        else:
            shadowed.update(found)
    return (direct & safe) - shadowed


def _plain_assignment(data, statement):
    """Return (lhs identifier bytes, rhs node) for a whole `x = rhs;` stmt."""
    if statement.type != "expression_statement":
        return None
    expressions = [c for c in statement.named_children if c.type != "comment"]
    if len(expressions) != 1 or expressions[0].type != "assignment_expression":
        return None
    assignment = expressions[0]
    op = assignment.child_by_field_name("operator")
    lhs = assignment.child_by_field_name("left")
    rhs = assignment.child_by_field_name("right")
    if (op is None or _txt(data, op) != b"=" or lhs is None or
            lhs.type != "identifier" or rhs is None):
        return None
    return _txt(data, lhs), rhs


def _rand_call(data, node):
    """Whether `node` is exactly the side-effecting expression `rand()`."""
    node = _unparen(node)
    if node is None or node.type != "call_expression":
        return False
    function = node.child_by_field_name("function")
    arguments = node.child_by_field_name("arguments")
    return (function is not None and _txt(data, function).strip() == b"rand" and
            arguments is not None and not arguments.named_children)


def _local_rand_mod(data, statement, locals_):
    """Return (name, literal modulus) for `local = rand() % LITERAL;`."""
    assignment = _plain_assignment(data, statement)
    if assignment is None:
        return None
    lhs, rhs = assignment
    rhs = _unparen(rhs)
    if lhs not in locals_ or rhs is None or rhs.type != "binary_expression":
        return None
    op = rhs.child_by_field_name("operator")
    left = rhs.child_by_field_name("left")
    right = _unparen(rhs.child_by_field_name("right"))
    if (op is None or _txt(data, op) != b"%" or not _rand_call(data, left) or
            right is None or right.type != "number_literal"):
        return None
    return lhs, _txt(data, right).strip()


def rule_rand_mod_split(text, name, span):
    """Split/merge `x = rand() % K` and `x = rand(); x = x % K`.

    With this pinned cc1, the split spelling copies `$v0` into the destination
    pseudo before expanding the remainder.  That can make the modulo chain work
    in-place in a saved register; the inline spelling computes on `$v0` first.
    Restrict the rewrite to nonvolatile automatic locals and a literal modulus,
    making the two adjacent forms equivalent in ordinary C execution.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)

    # SPLIT a single assignment statement.
    for statement in _find(body, ("expression_statement",)):
        match = _local_rand_mod(data, statement, locals_)
        if match is None:
            continue
        lhs, modulus = match
        indent = _indent_at(data, statement.start_byte)
        repl = (lhs + b" = rand();\n" + indent + lhs + b" = " + lhs +
                b" % " + modulus + b";")
        yield (f"rand-mod split {lhs.decode()} L{_line(data, statement.start_byte)}",
               splice(data, statement.start_byte, statement.end_byte, repl).decode())

    # MERGE the exact adjacent inverse.  Require whitespace only between the
    # statements so a source comment is never silently deleted.
    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]
        for first, second in zip(statements, statements[1:]):
            one = _plain_assignment(data, first)
            two = _plain_assignment(data, second)
            if one is None or two is None or one[0] != two[0] or one[0] not in locals_:
                continue
            lhs, first_rhs = one
            second_rhs = _unparen(two[1])
            if not _rand_call(data, first_rhs):
                continue
            if second_rhs is None or second_rhs.type != "binary_expression":
                continue
            op = second_rhs.child_by_field_name("operator")
            left = _unparen(second_rhs.child_by_field_name("left"))
            right = _unparen(second_rhs.child_by_field_name("right"))
            if (op is None or _txt(data, op) != b"%" or left is None or
                    left.type != "identifier" or _txt(data, left) != lhs or
                    right is None or right.type != "number_literal"):
                continue
            if data[first.end_byte:second.start_byte].strip():
                continue
            modulus = _txt(data, right).strip()
            repl = lhs + b" = rand() % " + modulus + b";"
            yield (f"rand-mod merge {lhs.decode()} L{_line(data, first.start_byte)}",
                   splice(data, first.start_byte, second.end_byte, repl).decode())


def _mod_bias_assignment(data, statement):
    """Recognise ``dst = base + (value % MOD - BIAS);``.

    Return the source nodes needed by ``rule_mod_bias_temp``.  Keep this
    deliberately narrow: three plain identifiers and literal constants make
    splitting the expression side-effect neutral, and leave volatile/call/
    alias-sensitive expressions to manual review.
    """
    assignment = _plain_assignment(data, statement)
    if assignment is None:
        return None
    destination, rhs = assignment
    rhs = _unparen(rhs)
    if rhs is None or rhs.type != "binary_expression":
        return None
    operator = rhs.child_by_field_name("operator")
    left = _unparen(rhs.child_by_field_name("left"))
    right = _unparen(rhs.child_by_field_name("right"))
    if operator is None or _txt(data, operator) != b"+":
        return None

    for base, biased in ((left, right), (right, left)):
        if base is None or base.type != "identifier" or biased is None:
            continue
        if biased.type != "binary_expression":
            continue
        bias_operator = biased.child_by_field_name("operator")
        remainder = _unparen(biased.child_by_field_name("left"))
        bias = _unparen(biased.child_by_field_name("right"))
        if (bias_operator is None or _txt(data, bias_operator) != b"-" or
                remainder is None or remainder.type != "binary_expression" or
                bias is None or bias.type != "number_literal"):
            continue
        remainder_operator = remainder.child_by_field_name("operator")
        value = _unparen(remainder.child_by_field_name("left"))
        modulus = _unparen(remainder.child_by_field_name("right"))
        if (remainder_operator is None or
                _txt(data, remainder_operator) != b"%" or value is None or
                value.type != "identifier" or modulus is None or
                modulus.type != "number_literal"):
            continue
        return destination, base, biased, value
    return None


def rule_mod_bias_temp(text, name, span):
    """Split a centered-modulo coordinate through a typed temporary.

    ``dst = base + (delta % RANGE - BIAS)`` and
    ``offset = delta % RANGE - BIAS; dst = base + offset`` expose the same
    arithmetic to C but not the same pseudo lifetimes to old cc1.  The latter
    preserved the modulo chain and changed the final ``addu`` allocation in
    ``DrawSnow``.  Try a short block-local temp at each site, plus one
    function-scope temp shared by repeated wraps of the same scalar type.

    The modulo input must be a uniquely declared, non-qualified automatic
    scalar, so the generated temp has exactly the expression's integer type.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    declarations = {}
    for declaration in _find(body, ("declaration",)):
        type_node = declaration.child_by_field_name("type")
        if type_node is None or re.search(
                rb"\b(?:const|volatile|static|register|extern)\b",
                _txt(data, declaration)):
            continue
        type_text = b" ".join(_txt(data, type_node).split())
        if type_text.decode() not in TYPES:
            continue
        for child in declaration.named_children:
            declarator = (child.child_by_field_name("declarator")
                          if child.type == "init_declarator" else child)
            if declarator is None or declarator.type != "identifier":
                continue
            ident = _txt(data, declarator)
            declarations.setdefault(ident, []).append(
                (type_text, declaration, declaration.parent == body))

    existing = {_txt(data, ident) for ident in _find(body, ("identifier",))}
    sites = []
    for statement in _find(body, ("expression_statement",)):
        match = _mod_bias_assignment(data, statement)
        if match is None or statement.parent is None or \
                statement.parent.type != "compound_statement":
            continue
        destination, base, biased, value = match
        value_name = _txt(data, value)
        records = declarations.get(value_name, ())
        if len(records) != 1:
            continue
        type_text, declaration, top_level = records[0]
        sites.append((statement, destination, base, biased, type_text,
                      declaration, top_level))

        index = 0
        fresh = b"_match_mod_offset"
        while fresh in existing:
            index += 1
            fresh = b"_match_mod_offset_" + str(index).encode()
        indent = _indent_at(data, statement.start_byte)
        inner = indent + b"    "
        replacement = (
            b"{\n" + inner + type_text + b" " + fresh + b";\n" +
            inner + fresh + b" = " + _txt(data, biased) + b";\n" +
            inner + destination + b" = " + _txt(data, base) + b" + " +
            fresh + b";\n" + indent + b"}"
        )
        yield (
            f"mod-bias block-temp {destination.decode()} "
            f"L{_line(data, statement.start_byte)}",
            splice(data, statement.start_byte, statement.end_byte,
                   replacement).decode(),
        )

    # Repeated coordinate wraps often used one source scratch.  Emit one
    # atomic shared-temp candidate per exact scalar spelling, but only when all
    # involved inputs are function-scope locals so the insertion is in scope.
    grouped = {}
    for site in sites:
        if site[6]:
            grouped.setdefault(site[4], []).append(site)
    for type_text, group in grouped.items():
        if len(group) < 2:
            continue
        index = 0
        fresh = b"_match_mod_offset"
        while fresh in existing:
            index += 1
            fresh = b"_match_mod_offset_" + str(index).encode()
        latest_declaration = max((site[5] for site in group),
                                 key=lambda node: node.end_byte)
        if latest_declaration.end_byte >= min(site[0].start_byte for site in group):
            continue
        declaration_indent = _indent_at(data, latest_declaration.start_byte)
        insertion = (b"\n" + declaration_indent + type_text + b" " + fresh +
                     b";")
        replacements = [(latest_declaration.end_byte,
                         latest_declaration.end_byte, insertion)]
        for statement, destination, base, biased, *_rest in group:
            indent = _indent_at(data, statement.start_byte)
            replacement = (fresh + b" = " + _txt(data, biased) + b";\n" +
                           indent + destination + b" = " + _txt(data, base) +
                           b" + " + fresh + b";")
            replacements.append((statement.start_byte, statement.end_byte,
                                 replacement))
        candidate = data
        for start, end, replacement in sorted(replacements, reverse=True):
            candidate = splice(candidate, start, end, replacement)
        yield (
            f"mod-bias shared-temp x{len(group)} "
            f"L{_line(data, group[0][0].start_byte)}",
            candidate.decode(),
        )


def _postincrement_name(data, statement):
    """Plain local name for a whole ``name++;`` expression statement."""
    if statement.type != "expression_statement":
        return None
    expressions = [child for child in statement.named_children
                   if child.type != "comment"]
    if len(expressions) != 1 or expressions[0].type != "update_expression":
        return None
    update = expressions[0]
    argument = update.child_by_field_name("argument")
    operator = update.child_by_field_name("operator")
    if (argument is None or argument.type != "identifier" or operator is None or
            _txt(data, operator) != b"++" or
            not _txt(data, update).rstrip().endswith(b"++")):
        return None
    return _txt(data, argument)


def rule_subscript_postinc(text, name, span):
    """Split/merge ``a[i] ...; i++;`` and ``a[i++] ...;``.

    Old cc1 gives the postincremented ARRAY_REF a distinct narrow working copy:
    the target can contain ``addiu work,i,1; move i,work`` where a separate
    increment updates ``i`` in place and is one instruction shorter.  The
    rewrite is limited to a nonvolatile automatic counter used exactly once in
    the affected statement and never address-taken, so moving its increment
    within that full expression cannot change an observable value.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)

    def safe_counter(counter, statement):
        if counter not in locals_:
            return False
        if re.search(rb"&\s*" + re.escape(counter) + rb"\b", body_text):
            return False
        return sum(_txt(data, ident) == counter
                   for ident in _find(statement, ("identifier",))) == 1

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]

        # MERGE an adjacent standalone postincrement into the subscript.
        for first, second in zip(statements, statements[1:]):
            counter = _postincrement_name(data, second)
            if (counter is None or first.type != "expression_statement" or
                    data[first.end_byte:second.start_byte].strip() or
                    not safe_counter(counter, first)):
                continue
            sites = []
            for subscript in _find(first, ("subscript_expression",)):
                index = subscript.child_by_field_name("index")
                if (index is not None and index.type == "identifier" and
                        _txt(data, index) == counter):
                    sites.append(index)
            if len(sites) != 1:
                continue
            index = sites[0]
            raw = data[first.start_byte:first.end_byte]
            rel_start = index.start_byte - first.start_byte
            rel_end = index.end_byte - first.start_byte
            raw = raw[:rel_start] + counter + b"++" + raw[rel_end:]
            yield (
                f"subscript-postinc merge {counter.decode()} "
                f"L{_line(data, first.start_byte)}",
                splice(data, first.start_byte, second.end_byte, raw).decode(),
            )

        # SPLIT a postincremented subscript into a following statement.
        for statement in statements:
            if statement.type != "expression_statement":
                continue
            for subscript in _find(statement, ("subscript_expression",)):
                index = subscript.child_by_field_name("index")
                if index is None or index.type != "update_expression":
                    continue
                argument = index.child_by_field_name("argument")
                operator = index.child_by_field_name("operator")
                if (argument is None or argument.type != "identifier" or
                        operator is None or _txt(data, operator) != b"++" or
                        not _txt(data, index).rstrip().endswith(b"++")):
                    continue
                counter = _txt(data, argument)
                if not safe_counter(counter, statement):
                    continue
                line_end = data.find(b"\n", statement.end_byte)
                if line_end < 0:
                    line_end = len(data)
                if data[statement.end_byte:line_end].strip():
                    continue
                raw = data[statement.start_byte:statement.end_byte]
                rel_start = index.start_byte - statement.start_byte
                rel_end = index.end_byte - statement.start_byte
                raw = raw[:rel_start] + counter + raw[rel_end:]
                indent = _indent_at(data, statement.start_byte)
                repl = raw + b"\n" + indent + counter + b"++;"
                yield (
                    f"subscript-postinc split {counter.decode()} "
                    f"L{_line(data, statement.start_byte)}",
                    splice(data, statement.start_byte, statement.end_byte,
                           repl).decode(),
                )


def rule_ptr_base_split(text, name, span):
    """Name an extern-array base where its lifetime changes address codegen.

    ``T *p = &Global[i]`` and ``T *base = Global; T *p = &base[i]`` are
    equivalent, but the latter gives split-addresses/CSE a distinct base pseudo
    and scheduling UID.  The call-crossing form similarly turns
    ``r = Call(); use(Global[r])`` into ``T *base = Global; r = Call();
    use(base[r])`` so the array address can live in a saved register across the
    call. Keep both forms guided and deliberately narrow.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for decl in _find(body, ("declaration",)):
        idecs = [child for child in decl.named_children
                 if child.type == "init_declarator"]
        if len(idecs) != 1:
            continue
        declarator = idecs[0].child_by_field_name("declarator")
        value = idecs[0].child_by_field_name("value")
        if (declarator is None or declarator.type != "pointer_declarator" or
                value is None or value.type != "pointer_expression"):
            continue
        operator = value.child_by_field_name("operator")
        subscript = value.child_by_field_name("argument")
        if (operator is None or _txt(data, operator) != b"&" or
                subscript is None or subscript.type != "subscript_expression"):
            continue
        base = subscript.child_by_field_name("argument")
        if base is None or base.type != "identifier":
            continue
        line = _line(data, decl.start_byte)
        if not _guided_site(line):
            continue
        identifier = _descend_ident(declarator)
        if identifier is None:
            continue
        suffix = 0
        while True:
            candidate = (b"_match_base" +
                         (str(suffix).encode() if suffix else b""))
            if not re.search(rb"\b" + re.escape(candidate) + rb"\b", data):
                temp = candidate
                break
            suffix += 1
        prefix = data[decl.start_byte:declarator.start_byte]
        stars = data[declarator.start_byte:identifier.start_byte]
        raw = data[decl.start_byte:decl.end_byte]
        rel_start = base.start_byte - decl.start_byte
        rel_end = base.end_byte - decl.start_byte
        rewritten = raw[:rel_start] + temp + raw[rel_end:]
        indent = _indent_at(data, decl.start_byte)
        temp_decl = (prefix + stars + temp + b" = " + _txt(data, base) +
                     b";\n" + indent)
        yield (
            f"ptr-base-split {_txt(data, base).decode()} L{line}",
            splice(data, decl.start_byte, decl.end_byte,
                   temp_decl + rewritten).decode(),
        )

    # Discover simple file-scope `extern T Global[]` declarations. Restricting
    # the declarator to a direct identifier excludes arrays of pointers and
    # other cases where reproducing the element type would be less mechanical.
    root = _TS.parse(data).root_node
    extern_arrays = {}
    for decl in root.named_children:
        if decl.type != "declaration":
            continue
        declarators = [child for child in decl.named_children
                       if child.type in ("array_declarator",
                                         "pointer_declarator",
                                         "function_declarator",
                                         "identifier",
                                         "init_declarator")]
        if len(declarators) != 1 or declarators[0].type != "array_declarator":
            continue
        array = declarators[0]
        identifier = array.child_by_field_name("declarator")
        if identifier is None or identifier.type != "identifier":
            continue
        prefix = data[decl.start_byte:array.start_byte].strip()
        match = re.match(rb"extern\b\s*(.+?)\s*$", prefix)
        if match is None:
            continue
        element_type = match.group(1)
        if element_type:
            extern_arrays[_txt(data, identifier)] = element_type

    if not extern_arrays:
        return

    locals_ = _nonvolatile_local_names(data, body)
    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for position, (call_stmt, consumer) in enumerate(
                zip(statements, statements[1:])):
            # Inserting a declaration here must remain valid for the original
            # compiler's declaration-before-statements dialect.
            if any(stmt.type != "declaration"
                   for stmt in statements[:position]):
                continue
            assignment = _plain_assignment(data, call_stmt)
            if assignment is None:
                continue
            result, rhs = assignment
            if result not in locals_ or _unparen(rhs).type != "call_expression":
                continue

            matches = {}
            for subscript in _find(consumer, ("subscript_expression",)):
                base = subscript.child_by_field_name("argument")
                index = _unparen(subscript.child_by_field_name("index"))
                if (base is None or base.type != "identifier" or
                        index is None or index.type != "identifier" or
                        _txt(data, index) != result):
                    continue
                symbol = _txt(data, base)
                if symbol in extern_arrays:
                    matches.setdefault(symbol, []).append(base)

            line = _line(data, call_stmt.start_byte)
            if not (_guided_site(line) or
                    _guided_site(_line(data, consumer.start_byte))):
                continue
            for symbol, bases in matches.items():
                suffix = 0
                while True:
                    candidate = (b"_match_base" +
                                 (str(suffix).encode() if suffix else b""))
                    if not re.search(rb"\b" + re.escape(candidate) + rb"\b",
                                     data):
                        temp = candidate
                        break
                    suffix += 1
                raw = data[call_stmt.start_byte:consumer.end_byte]
                for base in reversed(bases):
                    start = base.start_byte - call_stmt.start_byte
                    end = base.end_byte - call_stmt.start_byte
                    raw = raw[:start] + temp + raw[end:]
                indent = _indent_at(data, call_stmt.start_byte)
                temp_decl = (extern_arrays[symbol] + b" *" + temp + b" = " +
                             symbol + b";\n" + indent)
                yield (
                    f"ptr-base-call-live {symbol.decode()} L{line}",
                    splice(data, call_stmt.start_byte, consumer.end_byte,
                           temp_decl + raw).decode(),
                )


def rule_deref_address_split(text, name, span):
    """Split ``value = *(T *)address`` into pointer and value identities.

    A single dereference lets local allocation coalesce the computed address
    with the loaded scalar.  The original source can instead have named both:

        T *_match_value_ptr;
        _match_value_ptr = (T *)address;
        value = *_match_value_ptr;

    That spelling recovered Think3callaid's complete BreedLife argument setup.
    Keep the guided transform narrow and semantics-preserving: ``value`` is a
    nonvolatile automatic local, the dereference is through one explicit plain
    pointer cast, and its address expression contains no call, assignment, or
    update.  No operation is reordered; the declaration and two statements
    replace the one assignment at the same source point.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    identifiers = {_txt(data, identifier)
                   for identifier in _find(body, ("identifier",))}

    for statement in _find(body, ("expression_statement",)):
        if statement.parent is None or statement.parent.type != "compound_statement":
            continue
        assignment = _plain_assignment(data, statement)
        if assignment is None or assignment[0] not in locals_:
            continue
        value, rhs = assignment
        dereference = _unparen(rhs)
        if dereference is None or dereference.type != "pointer_expression":
            continue
        operator = dereference.child_by_field_name("operator")
        pointer = _unparen(dereference.child_by_field_name("argument"))
        if (operator is None or _txt(data, operator) != b"*" or
                pointer is None or pointer.type != "cast_expression"):
            continue
        pointer_type = pointer.child_by_field_name("type")
        address = pointer.child_by_field_name("value")
        if pointer_type is None or address is None:
            continue
        type_text = _txt(data, pointer_type).strip()
        if re.fullmatch(
                rb"(?:(?:const|volatile)\s+)*(?:struct\s+)?[A-Za-z_]\w*\s*\*",
                type_text) is None:
            continue
        if _find(address, ("assignment_expression", "call_expression",
                           "update_expression")):
            continue
        line = _line(data, statement.start_byte)
        if not _guided_site(line):
            continue

        stem = re.sub(rb"[^A-Za-z0-9_]", b"_", value)
        suffix = 0
        while True:
            temp = (b"_match_" + stem + b"_ptr" +
                    (str(suffix).encode() if suffix else b""))
            if temp not in identifiers:
                break
            suffix += 1
        # Keep old cc1's declaration-before-statements dialect: put the pointer
        # immediately before the scalar local's own plain declaration, then
        # split only the later executable statement.
        value_declaration = None
        for declaration in statement.parent.named_children:
            if declaration.type != "declaration" or declaration.start_byte >= statement.start_byte:
                continue
            declarators = []
            for child in declaration.named_children:
                if child.type == "init_declarator":
                    identifier = _descend_ident(child.child_by_field_name("declarator"))
                elif child.type in ("identifier", "pointer_declarator",
                                    "array_declarator"):
                    identifier = _descend_ident(child)
                else:
                    identifier = None
                if identifier is not None:
                    declarators.append(_txt(data, identifier))
            if declarators == [value]:
                value_declaration = declaration
                break
        if value_declaration is None:
            continue

        indent = _indent_at(data, statement.start_byte)
        replacement = (temp + b" = " + _txt(data, pointer).strip() + b";\n" +
                       indent + value + b" = *" + temp + b";")
        candidate = splice(data, statement.start_byte, statement.end_byte,
                           replacement)
        declaration_indent = _indent_at(data, value_declaration.start_byte)
        candidate = splice(
            candidate, value_declaration.start_byte, value_declaration.start_byte,
            type_text + b" " + temp + b";\n" + declaration_indent,
        )
        yield (
            f"deref-address-split {value.decode()} L{line}",
            candidate.decode(),
        )


def rule_ptr_index_sum(text, name, span):
    """Rewrite `T *p = base + idx;` to the integer-sum `p = (T*)(idx*sizeof(T) + (int)base)`.

    Pointer arithmetic `base + idx` ALWAYS folds to a base-first `addu`; only integer
    addition preserves operand order, and the target sometimes wants index-first
    (cookbook: "Pointer arithmetic normalises to base+index"). This closed SetBlood and
    SetHinoko. Yields BOTH operand assignments (which is base vs index); a
    semantically-wrong one just produces wrong bytes and is discarded by scoring. Only
    fires on a pointer-typed local declaration whose initializer is a bare `A + B`."""
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for decl in _find(body, ("declaration",)):
        ty = decl.child_by_field_name("type")
        if ty is None:
            continue
        elemtype = _txt(data, ty).strip()
        idecs = [c for c in decl.named_children if c.type == "init_declarator"]
        if len(idecs) != 1:
            continue
        dclr = idecs[0].child_by_field_name("declarator")
        val = idecs[0].child_by_field_name("value")
        if dclr is None or val is None or dclr.type != "pointer_declarator":
            continue  # want exactly `T *p` (one level of pointer)
        if val.type != "binary_expression":
            continue
        op = val.child_by_field_name("operator")
        A = val.child_by_field_name("left")
        B = val.child_by_field_name("right")
        if op is None or _txt(data, op) != b"+" or A is None or B is None:
            continue
        at, bt = _txt(data, A).strip(), _txt(data, B).strip()
        cast = b"(" + elemtype + b" *)"
        sz = b"sizeof(" + elemtype + b")"
        # variant 1: A is base (pointer), B is index
        v1 = cast + b"((int)" + at + b" + " + bt + b" * " + sz + b")"
        # variant 2: B is base, A is index
        v2 = cast + b"((int)" + bt + b" + " + at + b" * " + sz + b")"
        for tag, repl in ((f"ptr-sum {at.decode()}+{bt.decode()}", v1),
                          (f"ptr-sum {bt.decode()}+{at.decode()}", v2)):
            yield (f"{tag} L{_line(data, val.start_byte)}",
                   splice(data, val.start_byte, val.end_byte, repl).decode())

    # The same fold occurs when a pointer is assigned after its declaration.
    # Recover the visible local's full pointer type (including ** depth), and
    # use sizeof(*lhs), which is the exact stride of pointer arithmetic.
    pointer_casts = {}
    for decl in _find(body, ("declaration",)):
        ty = decl.child_by_field_name("type")
        if ty is None:
            continue
        for child in decl.named_children:
            dclr = (child.child_by_field_name("declarator")
                    if child.type == "init_declarator" else child)
            if dclr is None or dclr.type != "pointer_declarator":
                continue
            stars = 0
            cursor = dclr
            while cursor is not None and cursor.type == "pointer_declarator":
                stars += 1
                cursor = cursor.child_by_field_name("declarator")
            if cursor is None or cursor.type != "identifier":
                continue
            pointer_casts[_txt(data, cursor)] = (
                b"(" + _txt(data, ty).strip() + b" " + b"*" * stars + b")"
            )
    for statement in _find(body, ("expression_statement",)):
        expressions = [child for child in statement.named_children
                       if child.type != "comment"]
        if len(expressions) != 1 or expressions[0].type != "assignment_expression":
            continue
        assignment = expressions[0]
        operator = assignment.child_by_field_name("operator")
        lhs = assignment.child_by_field_name("left")
        value = assignment.child_by_field_name("right")
        if (operator is None or _txt(data, operator) != b"=" or lhs is None or
                lhs.type != "identifier" or _txt(data, lhs) not in pointer_casts or
                value is None or value.type != "binary_expression"):
            continue
        plus = value.child_by_field_name("operator")
        left = value.child_by_field_name("left")
        right = value.child_by_field_name("right")
        if (plus is None or _txt(data, plus) != b"+" or
                left is None or right is None):
            continue
        lhs_text = _txt(data, lhs)
        left_text, right_text = _txt(data, left).strip(), _txt(data, right).strip()
        cast = pointer_casts[lhs_text]
        stride = b"sizeof(*" + lhs_text + b")"
        variants = (
            (left_text, right_text),
            (right_text, left_text),
        )
        for base, index in variants:
            replacement = (cast + b"((u32)" + base + b" + " + index +
                           b" * " + stride + b")")
            yield (
                f"ptr-sum-assign {base.decode()}+{index.decode()} "
                f"L{_line(data, value.start_byte)}",
                splice(data, value.start_byte, value.end_byte,
                       replacement).decode(),
            )


def _stmt_expr(data, stmt):
    """(lhs_text, rhs_text) if `stmt` is `X = E;` (assignment or single-init decl)."""
    if stmt.type == "expression_statement":
        e = [c for c in stmt.named_children if c.type != "comment"]
        if len(e) == 1 and e[0].type == "assignment_expression":
            a = e[0]
            op = a.child_by_field_name("operator")
            if op is not None and _txt(data, op) == b"=":
                return (_txt(data, a.child_by_field_name("left")).strip(),
                        _txt(data, a.child_by_field_name("right")).strip())
    if stmt.type == "declaration":
        idecs = [c for c in stmt.named_children if c.type == "init_declarator"]
        if len(idecs) == 1:
            idn = _descend_ident(idecs[0].child_by_field_name("declarator"))
            val = idecs[0].child_by_field_name("value")
            if idn is not None and val is not None:
                return (_txt(data, idn).strip(), _txt(data, val).strip())
    return None


def _vector_field_access(data, node):
    if node is None or node.type != "field_expression":
        return None
    base = node.child_by_field_name("argument")
    field = node.child_by_field_name("field")
    operator = node.child_by_field_name("operator")
    if base is None or field is None or operator is None:
        return None
    op = _txt(data, operator)
    aggregate = _txt(data, base).strip()
    if op == b"->":
        aggregate = b"*(" + aggregate + b")"
    elif op != b".":
        return None
    return aggregate, _txt(data, field).strip()


def _declared_vector_type(text, aggregate):
    """VECTOR/SVECTOR type of a simple aggregate expression, or None.

    Restricting the rewrite to visible declarations prevents four same-named
    fields in a larger struct from becoming an accidental whole-struct copy.
    """
    expr = aggregate.decode().strip()
    pointer = re.fullmatch(r"\*\(\s*([A-Za-z_]\w*)\s*\)", expr)
    ident = pointer.group(1) if pointer else expr
    if not re.fullmatch(r"[A-Za-z_]\w*", ident):
        return None
    star = r"\s*\*+\s*" if pointer else r"\s+"
    decl = re.compile(
        rf"\b(?:struct\s+)?(VECTOR|SVECTOR){star}{re.escape(ident)}\b"
    )
    mask = _comment_mask(text)
    match = next((m for m in decl.finditer(text) if not mask[m.start()]), None)
    return match.group(1) if match else None


def rule_vector_copy(text, name, span):
    """Collapse four adjacent VECTOR/SVECTOR field copies to struct assignment.

    `dst.vx=src.vx; ... dst.pad=src.pad;` and pointer variants describe one
    aggregate copy. cc1 expands `dst = src` through a different block-move path,
    which can recover the target's interleaved four-load/four-store sequence
    (DamageControl). Restricting the exact field set keeps padding/partial-copy
    semantics out of the rule.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    wanted = {b"vx", b"vy", b"vz", b"pad"}
    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]
        for start in range(len(statements) - 3):
            group = statements[start:start + 4]
            fields = []
            lhs_aggregate = rhs_aggregate = None
            valid = True
            for stmt in group:
                if stmt.type != "expression_statement":
                    valid = False
                    break
                exprs = [c for c in stmt.named_children if c.type != "comment"]
                if len(exprs) != 1 or exprs[0].type != "assignment_expression":
                    valid = False
                    break
                assignment = exprs[0]
                operator = assignment.child_by_field_name("operator")
                if operator is None or _txt(data, operator) != b"=":
                    valid = False
                    break
                lhs = _vector_field_access(
                    data, assignment.child_by_field_name("left")
                )
                rhs = _vector_field_access(
                    data, assignment.child_by_field_name("right")
                )
                if lhs is None or rhs is None or lhs[1] != rhs[1]:
                    valid = False
                    break
                if lhs_aggregate is None:
                    lhs_aggregate, rhs_aggregate = lhs[0], rhs[0]
                elif lhs[0] != lhs_aggregate or rhs[0] != rhs_aggregate:
                    valid = False
                    break
                fields.append(lhs[1])
            if not valid or set(fields) != wanted or len(set(fields)) != 4:
                continue
            lhs_type = _declared_vector_type(text, lhs_aggregate)
            rhs_type = _declared_vector_type(text, rhs_aggregate)
            if lhs_type is None or lhs_type != rhs_type:
                continue
            first, last = group[0], group[-1]
            repl = lhs_aggregate + b" = " + rhs_aggregate + b";"
            yield (
                f"vector-copy L{_line(data, first.start_byte)}",
                splice(data, first.start_byte, last.end_byte, repl).decode(),
            )


def _vector_assignment(data, statement):
    """(assignment, lhs aggregate/field, rhs node) for one field store."""
    if statement.type != "expression_statement":
        return None
    expressions = [c for c in statement.named_children if c.type != "comment"]
    if len(expressions) != 1 or expressions[0].type != "assignment_expression":
        return None
    assignment = expressions[0]
    operator = assignment.child_by_field_name("operator")
    lhs = assignment.child_by_field_name("left")
    rhs = assignment.child_by_field_name("right")
    field = _vector_field_access(data, lhs)
    if operator is None or lhs is None or rhs is None or field is None:
        return None
    return assignment, operator, lhs, field, rhs


def _local_aggregate_root(aggregate, locals_):
    """Whether a dotted aggregate is rooted in a nonvolatile local object."""
    if not re.fullmatch(rb"[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*", aggregate):
        return False
    match = re.match(rb"([A-Za-z_]\w*)", aggregate)
    return match is not None and match.group(1) in locals_


def _literal_text(data, node):
    node = _unparen(node)
    if node is None or node.type != "number_literal":
        return None
    value = _txt(data, node).strip()
    return value if re.fullmatch(rb"(?:0x[0-9a-fA-F]+|\d+)", value) else None


def _field_path(data, node):
    """(root identifier, textual path) for a plain . / -> field chain."""
    node = _unparen(node)
    original = node
    while node is not None and node.type == "field_expression":
        node = node.child_by_field_name("argument")
    if node is None or node.type != "identifier" or original is None:
        return None
    text = _txt(data, original).strip()
    if not re.fullmatch(rb"[A-Za-z_]\w*(?:(?:->|\.)[A-Za-z_]\w*)+", text):
        return None
    return _txt(data, node), text


def rule_member_scalar_alias(text, name, span):
    """Toggle a long field read through an equally wide ``s32`` lvalue.

    Old cc1 marks a direct structure-member load as MEM_IN_STRUCT.  Reading the
    same PS1 32-bit representation through a scalar lvalue clears that marker,
    which can stop cse from sinking/coalescing a captured coordinate load.  The
    lever is intentionally aggressive: only an exact ``long local;`` followed
    by a side-effect-free field assignment is eligible, and RTL-guided scoring
    decides whether the changed alias class belongs at that site (DrawSplash).

    The reverse transform is included so the search can back out a stale match
    cast without a hand edit.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    declarations = {}
    for declaration in _find(body, ("declaration",)):
        identifiers = []
        for child in declaration.named_children:
            if child.type == "init_declarator":
                ident = _descend_ident(child.child_by_field_name("declarator"))
            elif child.type in ("identifier", "pointer_declarator",
                                "array_declarator"):
                ident = _descend_ident(child)
            else:
                ident = None
            if ident is not None:
                identifiers.append(_txt(data, ident))
        raw = _txt(data, declaration).strip()
        match = re.fullmatch(rb"long\s+([A-Za-z_]\w*)\s*;", raw)
        scope = declaration.parent
        while scope is not None and scope.type != "compound_statement":
            scope = scope.parent
        if scope is None:
            continue
        scope_key = (scope.start_byte, scope.end_byte)
        exact = (match.group(1) if match is not None and
                 identifiers == [match.group(1)] else None)
        for ident in identifiers:
            declarations.setdefault(ident, []).append(
                (scope_key, declaration.start_byte, ident == exact)
            )

    def is_visible_long_local(statement, ident):
        """Resolve ``ident`` lexically at this statement, including shadows."""
        scope_keys = []
        scope = statement.parent
        while scope is not None:
            if scope.type == "compound_statement":
                scope_keys.append((scope.start_byte, scope.end_byte))
            scope = scope.parent
        candidates = []
        for scope_key, start, exact in declarations.get(ident, []):
            if start >= statement.start_byte or scope_key not in scope_keys:
                continue
            candidates.append((scope_keys.index(scope_key), -start, exact))
        if not candidates:
            return False
        return min(candidates)[2]

    def aliased_field(rhs):
        """The field under exactly ``*(s32 *)&field``, otherwise None."""
        dereference = _unparen(rhs)
        if dereference is None or dereference.type != "pointer_expression":
            return None
        operator = dereference.child_by_field_name("operator")
        cast = _unparen(dereference.child_by_field_name("argument"))
        if (operator is None or _txt(data, operator) != b"*" or
                cast is None or cast.type != "cast_expression"):
            return None
        cast_type = cast.child_by_field_name("type")
        address = _unparen(cast.child_by_field_name("value"))
        if (cast_type is None or
                re.fullmatch(rb"s32\s*\*", _txt(data, cast_type).strip()) is None or
                address is None or address.type != "pointer_expression"):
            return None
        address_operator = address.child_by_field_name("operator")
        field = _unparen(address.child_by_field_name("argument"))
        if (address_operator is None or _txt(data, address_operator) != b"&" or
                field is None or field.type != "field_expression" or
                _field_path(data, field) is None):
            return None
        return field

    edits = []
    for statement in _find(body, ("expression_statement",)):
        assignment = _plain_assignment(data, statement)
        if assignment is None:
            continue
        local, rhs = assignment
        line = _line(data, statement.start_byte)
        if not is_visible_long_local(statement, local) or not _guided_site(line):
            continue

        field = _unparen(rhs)
        if (field is not None and field.type == "field_expression" and
                _field_path(data, field) is not None):
            replacement = b"*(s32 *)&" + _txt(data, field).strip()
            tag = "add"
        else:
            field = aliased_field(rhs)
            if field is None:
                continue
            replacement = _txt(data, field).strip()
            tag = "remove"
        root, _path = _field_path(data, field)
        edits.append((tag, local, root, line, rhs.start_byte, rhs.end_byte,
                      replacement))

    for tag, local, _root, line, start, end, replacement in edits:
        yield (
            f"member-scalar-alias {tag} {local.decode()} L{line}",
            splice(data, start, end, replacement).decode(),
        )

    # Coordinate-style captures commonly need two independent live ranges at
    # once.  A single alias can score worse even though the pair is exact, so
    # emit adjacent same-object edits atomically rather than relying on a wide
    # regression allowance in the beam (DrawSplash's py/pz pair).
    for first, second in zip(edits, edits[1:]):
        if first[0] != second[0] or first[2] != second[2]:
            continue
        tag = first[0]
        locals_ = first[1].decode() + "/" + second[1].decode()
        lines = f"L{first[3]}/{second[3]}"
        changed = data
        for edit in sorted((first, second), key=lambda item: item[4], reverse=True):
            changed = splice(changed, edit[4], edit[5], edit[6])
        yield (
            f"member-scalar-alias {tag} {locals_} {lines}",
            changed.decode(),
        )


def rule_disjoint_local_alias(text, name, span):
    """Give a dead-until-overwrite scalar local an earlier shared identity.

    If uninitialized ``later`` is untouched before its next plain assignment,
    then changing ``early = E; use(early); ... later = L;`` to
    ``later = early = E; use(later); ... later = L;`` preserves all observable
    values on defined paths.  Old cc1 can nevertheless join the two disjoint
    live ranges and change their global-allocation donor/preferences.
    Think1target needed this independently for its x and z coordinate pairs.

    Keep the search deliberately narrow: both names must be same-spelled-type,
    nonvolatile function-scope integer scalars; the alias must be uninitialized
    and untouched before this site; neither name may have its address taken or
    be shadowed; the source cannot be inside a loop; the alias's first later
    occurrence must be its overwrite; and the substituted read must be in the
    source assignment's compound block.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    def declared_names(declaration):
        names = set()
        for child in declaration.named_children:
            if child.type == "init_declarator":
                ident = _descend_ident(child.child_by_field_name("declarator"))
            elif child.type in ("identifier", "pointer_declarator",
                                "array_declarator"):
                ident = _descend_ident(child)
            else:
                ident = None
            if ident is not None:
                names.add(_txt(data, ident))
        return names

    locals_ = _nonvolatile_local_names(data, body)
    types = {}
    declaration_ends = {}
    uninitialized = set()
    for declaration in [child for child in body.named_children
                        if child.type == "declaration"]:
        type_node = declaration.child_by_field_name("type")
        if (type_node is None or
                re.search(rb"\b(?:static|extern|_Thread_local)\b",
                          _txt(data, declaration))):
            continue
        type_text = _txt(data, type_node).strip()
        if type_text.decode(errors="ignore") not in TYPES:
            continue
        for child in declaration.named_children:
            declarator = (child.child_by_field_name("declarator")
                          if child.type == "init_declarator" else child)
            if declarator is None or declarator.type != "identifier":
                continue
            ident = _txt(data, declarator)
            if ident in locals_:
                types[ident] = type_text
                declaration_ends[ident] = declarator.end_byte
                if child.type == "identifier":
                    uninitialized.add(ident)

    # The candidate names above are direct children of the function body.  A
    # same-spelled declaration in a nested scope would make the byte-oriented
    # identifier scan ambiguous, so exclude that name instead of attempting a
    # partial C name resolver here.
    shadowed = set()
    for declaration in _find(body, ("declaration",)):
        if declaration.parent != body:
            shadowed.update(declared_names(declaration) & set(types))

    identifiers = sorted(_find(body, ("identifier",)),
                         key=lambda node: node.start_byte)
    addressed = set()
    for ident in identifiers:
        node = ident
        parent = node.parent
        while parent is not None and parent.type == "parenthesized_expression":
            node = parent
            parent = node.parent
        if parent is None or parent.type != "pointer_expression":
            continue
        operator = parent.child_by_field_name("operator")
        if operator is not None and _txt(data, operator) == b"&":
            addressed.add(_txt(data, ident))

    def plain_write(ident):
        parent = ident.parent
        if parent is None or parent.type != "assignment_expression":
            return False
        operator = parent.child_by_field_name("operator")
        lhs = parent.child_by_field_name("left")
        return (operator is not None and _txt(data, operator) == b"=" and
                lhs is not None and lhs.start_byte == ident.start_byte and
                lhs.end_byte == ident.end_byte)

    def value_read(ident):
        parent = ident.parent
        if parent is None:
            return False
        if parent.type in ("update_expression",):
            return False
        if parent.type == "pointer_expression":
            operator = parent.child_by_field_name("operator")
            if operator is not None and _txt(data, operator) == b"&":
                return False
        if parent.type == "assignment_expression":
            lhs = parent.child_by_field_name("left")
            if (lhs is not None and lhs.start_byte <= ident.start_byte and
                    ident.end_byte <= lhs.end_byte):
                return False
        return True

    def in_source_block(ident, block):
        node = ident.parent
        while node is not None and node != block:
            if node.type == "compound_statement":
                return False
            node = node.parent
        return node == block

    def inside_loop(statement):
        node = statement.parent
        while node is not None and node != body:
            if node.type in ("do_statement", "for_statement",
                             "while_statement"):
                return True
            node = node.parent
        return False

    for statement in _find(body, ("expression_statement",)):
        assignment = _plain_assignment(data, statement)
        if assignment is None:
            continue
        early, rhs = assignment
        line = _line(data, statement.start_byte)
        if (early not in types or early in addressed or early in shadowed or
                inside_loop(statement) or
                not (_guided_site(line) or early in GUIDED_VARIABLES)):
            continue
        block = statement.parent
        if block is None or block.type != "compound_statement":
            continue

        for later, type_text in sorted(types.items()):
            if (later == early or type_text != types[early] or
                    later not in uninitialized or
                    later in addressed or later in shadowed or
                    (GUIDED_VARIABLES and
                     later not in GUIDED_VARIABLES and
                     early not in GUIDED_VARIABLES)):
                continue
            if (declaration_ends[later] >= statement.start_byte or
                    any(declaration_ends[later] < ident.start_byte <
                        statement.start_byte and _txt(data, ident) == later
                        for ident in identifiers)):
                continue
            if any(_txt(data, ident) == later
                   for ident in _find(rhs, ("identifier",))):
                continue
            occurrences = [ident for ident in identifiers
                           if ident.start_byte >= statement.end_byte and
                           _txt(data, ident) == later]
            if not occurrences or not plain_write(occurrences[0]):
                continue
            overwrite = occurrences[0]
            reads = [
                ident for ident in identifiers
                if statement.end_byte <= ident.start_byte < overwrite.start_byte
                and ident.end_byte <= block.end_byte
                and _txt(data, ident) == early and value_read(ident)
                and in_source_block(ident, block)
            ]
            for read in reads[:4]:
                replacement = (later + b" = " + early + b" = " +
                               _txt(data, rhs).strip() + b";")
                changed = splice(data, read.start_byte, read.end_byte, later)
                changed = splice(changed, statement.start_byte,
                                 statement.end_byte, replacement)
                yield (
                    f"disjoint-local-alias {later.decode()}={early.decode()} "
                    f"L{line}/L{_line(data, read.start_byte)}",
                    changed.decode(),
                )


def rule_assignment_chain(text, name, span):
    """Merge/split same-literal stores to distinct fields of one aggregate.

    C chained assignment evaluates right-to-left, so ``a=b=c=K`` is a compact
    source lever for reverse store order and constant/register lifetime.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)

    def one(statement):
        if statement.type != "expression_statement":
            return None
        children = [child for child in statement.named_children
                    if child.type != "comment"]
        if len(children) != 1 or children[0].type != "assignment_expression":
            return None
        assignment = children[0]
        operator = assignment.child_by_field_name("operator")
        lhs = assignment.child_by_field_name("left")
        rhs = assignment.child_by_field_name("right")
        path = _field_path(data, lhs)
        literal = _literal_text(data, rhs)
        if (operator is None or _txt(data, operator) != b"=" or
                path is None or path[0] not in locals_ or literal is None):
            return None
        return path[0], path[1], literal

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        # Merge two-to-four adjacent scalar stores as one right-to-left chain.
        for count in range(2, 5):
            for start in range(len(statements) - count + 1):
                group = statements[start:start + count]
                parsed = [one(statement) for statement in group]
                if any(item is None for item in parsed):
                    continue
                roots = {item[0] for item in parsed}
                paths = [item[1] for item in parsed]
                literals = {item[2] for item in parsed}
                if len(roots) != 1 or len(set(paths)) != count or len(literals) != 1:
                    continue
                if any(data[a.end_byte:b.start_byte].strip()
                       for a, b in zip(group, group[1:])):
                    continue
                line = _line(data, group[0].start_byte)
                if not _guided_site(line):
                    continue
                replacement = b" = ".join(paths + [parsed[0][2]]) + b";"
                yield (
                    f"assignment-chain merge {count} L{line}",
                    splice(data, group[0].start_byte, group[-1].end_byte,
                           replacement).decode(),
                )

    # Split an existing chain into forward/reverse explicit store orders.
    for statement in _find(body, ("expression_statement",)):
        children = [child for child in statement.named_children
                    if child.type != "comment"]
        if len(children) != 1 or children[0].type != "assignment_expression":
            continue
        paths = []
        root = None
        cursor = children[0]
        while cursor is not None and cursor.type == "assignment_expression":
            operator = cursor.child_by_field_name("operator")
            lhs = cursor.child_by_field_name("left")
            rhs = cursor.child_by_field_name("right")
            path = _field_path(data, lhs)
            if operator is None or _txt(data, operator) != b"=" or path is None:
                paths = []
                break
            root = root or path[0]
            if path[0] != root:
                paths = []
                break
            paths.append(path[1])
            cursor = rhs
        literal = _literal_text(data, cursor)
        if (len(paths) < 2 or len(paths) > 4 or len(set(paths)) != len(paths) or
                root not in locals_ or literal is None):
            continue
        line = _line(data, statement.start_byte)
        if not _guided_site(line):
            continue
        indent = _indent_at(data, statement.start_byte)
        for tag, order in (("forward", paths), ("reverse", list(reversed(paths)))):
            replacement = (b";\n" + indent).join(
                path + b" = " + literal for path in order
            ) + b";"
            yield (
                f"assignment-chain split-{tag} {len(paths)} L{line}",
                splice(data, statement.start_byte, statement.end_byte,
                       replacement).decode(),
            )


def rule_field_capture_rhs(text, name, span):
    """Fuse/split a saved field value through the overwriting assignment's RHS.

    ``saved = p->field; p->field = saved + expr;`` and
    ``p->field = expr + (saved = p->field);`` have the same value effects when
    the field is nonvolatile and ``expr`` has no side effects.  Their RTL
    dependency trees differ, which can decide whether old cc1 schedules the
    independent expression before or after the field load.  Keep this guided
    and bounded to adjacent statements, one plain automatic saved local, one
    exact field path, and addition with a pure other operand.
    """
    global GUIDED_LINES
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_function_local_names(data, body)
    safe_objects = locals_ | _nonvolatile_parameter_names(data, body)
    effects = ("call_expression", "assignment_expression", "update_expression")

    def whole_assignment(statement):
        if statement.type != "expression_statement":
            return None
        expressions = [child for child in statement.named_children
                       if child.type != "comment"]
        if len(expressions) != 1 or expressions[0].type != "assignment_expression":
            return None
        assignment = expressions[0]
        operator = assignment.child_by_field_name("operator")
        lhs = assignment.child_by_field_name("left")
        rhs = assignment.child_by_field_name("right")
        if (operator is None or _txt(data, operator) != b"=" or
                lhs is None or rhs is None):
            return None
        return lhs, rhs

    def bare_local(node):
        node = _unparen(node)
        if node is None or node.type != "identifier":
            return None
        ident = _txt(data, node)
        return ident if ident in locals_ else None

    def pure_other(node, saved):
        if _find(node, effects + ("pointer_expression", "subscript_expression")):
            return False
        fields = _find(node, ("field_expression",))
        if any((path := _field_path(data, field)) is None or
               path[0] not in safe_objects for field in fields):
            return False
        identifiers = [_txt(data, ident)
                       for ident in _find(node, ("identifier",))]
        return saved not in identifiers and all(ident in safe_objects
                                                for ident in identifiers)

    # Split capture followed by overwrite -> one dependency-shaped assignment.
    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for first, second in zip(statements, statements[1:]):
            if data[first.end_byte:second.start_byte].strip():
                continue
            first_assignment = whole_assignment(first)
            second_assignment = whole_assignment(second)
            if first_assignment is None or second_assignment is None:
                continue
            saved_lhs, field_read = first_assignment
            field_write, expression = second_assignment
            saved = bare_local(saved_lhs)
            read_path = _field_path(data, field_read)
            write_path = _field_path(data, field_write)
            binary = _unparen(expression)
            if (saved is None or read_path is None or
                    read_path[0] not in safe_objects or
                    write_path != read_path or
                    binary is None or binary.type != "binary_expression" or
                    _binary_operator(data, binary) != b"+"):
                continue
            left = binary.child_by_field_name("left")
            right = binary.child_by_field_name("right")
            if left is None or right is None:
                continue
            if bare_local(left) == saved and pure_other(right, saved):
                other = right
            elif bare_local(right) == saved and pure_other(left, saved):
                other = left
            else:
                continue
            lines = (_line(data, first.start_byte),
                     _line(data, second.start_byte))
            if not any(_guided_site(line) for line in lines):
                continue
            field = read_path[1]
            other_text = b"(" + _txt(data, other).strip() + b")"
            capture = b"(" + saved + b" = " + field + b")"
            for tag, addends in (
                    ("expr-first", (other_text, capture)),
                    ("capture-first", (capture, other_text))):
                replacement = field + b" = " + b" + ".join(addends) + b";"
                yield (
                    f"field-capture-rhs fuse-{tag} {saved.decode()} "
                    f"L{lines[0]}-{lines[1]}",
                    splice(data, first.start_byte, second.end_byte,
                           replacement).decode(),
                )

    # One dependency-shaped assignment -> the conventional adjacent capture.
    for statement in _find(body, ("expression_statement",)):
        outer = whole_assignment(statement)
        if outer is None:
            continue
        field_write, expression = outer
        write_path = _field_path(data, field_write)
        binary = _unparen(expression)
        if (write_path is None or write_path[0] not in safe_objects or
                binary is None or
                binary.type != "binary_expression" or
                _binary_operator(data, binary) != b"+"):
            continue
        left = binary.child_by_field_name("left")
        right = binary.child_by_field_name("right")
        if left is None or right is None:
            continue

        capture = None
        other = None
        for possible_capture, possible_other in ((left, right), (right, left)):
            inner = _unparen(possible_capture)
            if inner is None or inner.type != "assignment_expression":
                continue
            operator = inner.child_by_field_name("operator")
            saved_lhs = inner.child_by_field_name("left")
            field_read = inner.child_by_field_name("right")
            saved = bare_local(saved_lhs)
            if (operator is None or _txt(data, operator) != b"=" or
                    saved is None or _field_path(data, field_read) != write_path or
                    not pure_other(possible_other, saved)):
                continue
            capture = (saved, field_read)
            other = possible_other
            break
        if capture is None:
            continue
        line = _line(data, statement.start_byte)
        if not _guided_site(line):
            continue
        saved, field_read = capture
        field = write_path[1]
        indent = _indent_at(data, statement.start_byte)
        other_text = _txt(data, other).strip()
        for tag, addends in (
                ("saved-first", (saved, other_text)),
                ("expr-first", (other_text, saved))):
            replacement = (saved + b" = " + _txt(data, field_read).strip() +
                           b";\n" + indent + field + b" = " +
                           b" + ".join(addends) + b";")
            yield (
                f"field-capture-rhs split-{tag} {saved.decode()} L{line}",
                splice(data, statement.start_byte, statement.end_byte,
                       replacement).decode(),
            )

    # Private helper macros can contain the same source shape while the
    # function body contains only their invocations.  Parse each invoked
    # function-like macro's continuation body as a synthetic function, reuse
    # the guarded transform above, then map its one edit back to the original
    # preprocessor argument.  Flatten an inserted newline so the surrounding
    # macro continuation remains valid.
    root = _TS.parse(data).root_node
    outer_locals = safe_objects

    def one_edit(original, changed):
        prefix = 0
        limit = min(len(original), len(changed))
        while prefix < limit and original[prefix] == changed[prefix]:
            prefix += 1
        suffix = 0
        while (suffix < len(original) - prefix and
               suffix < len(changed) - prefix and
               original[-suffix - 1] == changed[-suffix - 1]):
            suffix += 1
        old_end = len(original) - suffix
        new_end = len(changed) - suffix
        return prefix, old_end, changed[prefix:new_end]

    for macro in _find(root, ("preproc_function_def",)):
        macro_name_node = macro.child_by_field_name("name")
        macro_params = macro.child_by_field_name("parameters")
        macro_value = macro.child_by_field_name("value")
        if macro_name_node is None or macro_params is None or macro_value is None:
            continue
        macro_name = _txt(data, macro_name_node)
        undef_ranges = []
        for directive in _find(root, ("preproc_call",)):
            if re.fullmatch(rb"#undef\s+" + re.escape(macro_name) + rb"\s*",
                            _txt(data, directive)):
                undef_ranges.append((directive.start_byte, directive.end_byte))
        references = list(re.finditer(rb"\b" + re.escape(macro_name) + rb"\b",
                                      data))
        if any(not (macro_name_node.start_byte <= ref.start() <
                    macro_name_node.end_byte or
                    body.start_byte <= ref.start() < body.end_byte or
                    any(start <= ref.start() < end
                        for start, end in undef_ranges))
               for ref in references):
            continue
        use_lines = []
        for call in _find(body, ("call_expression",)):
            function = _unparen(call.child_by_field_name("function"))
            if (function is not None and function.type == "identifier" and
                    _txt(data, function) == macro_name):
                use_lines.append(_line(data, call.start_byte))
        if not use_lines:
            continue
        if GUIDED_LINES and not any(_guided_site(line) for line in use_lines):
            continue
        raw_value = _txt(data, macro_value)
        if not raw_value.startswith(b"{"):
            continue
        # Keep byte offsets stable: replace only the continuation backslash,
        # not its newline, before parsing the body as ordinary C.
        logical_value = re.sub(rb"\\(?=\r?\n)", b" ", raw_value)
        # The macro may capture a local declared by its caller (StageEndScreen's
        # base_u). Seed those names as dummy ints in the synthetic outer block;
        # tree-sitter needs only lexical locality here, not their real types.
        # Never seed a macro formal: its invocation argument could be a global,
        # volatile access, or effectful expression that this rule cannot prove.
        formal_names = {_txt(data, ident)
                        for ident in _find(macro_params, ("identifier",))}
        captured_outer = outer_locals - formal_names
        declarations = b"".join(b"int " + local + b";"
                                for local in sorted(captured_outer))
        wrapper = b"void __autorule_macro(void) "
        synthetic = (wrapper + logical_value[:1] + declarations +
                     logical_value[1:])
        mapping_bias = len(wrapper) + len(declarations)
        synthetic_text = synthetic.decode()
        saved_guided = GUIDED_LINES
        try:
            GUIDED_LINES = set()
            nested = list(rule_field_capture_rhs(
                synthetic_text, "__autorule_macro", (0, len(synthetic_text))))
        finally:
            GUIDED_LINES = saved_guided
        for label, changed_text in nested:
            start, end, replacement = one_edit(synthetic,
                                                changed_text.encode())
            if start < mapping_bias or end < mapping_bias:
                continue
            source_start = macro_value.start_byte + start - mapping_bias
            source_end = macro_value.start_byte + end - mapping_bias
            replacement = replacement.replace(b"\r\n", b" ").replace(b"\n", b" ")
            yield (
                f"field-capture-rhs macro-{macro_name.decode()} {label}",
                splice(data, source_start, source_end, replacement).decode(),
            )


def rule_vector_copy_adjust(text, name, span):
    """Split/merge an adjusted component in a three-field vector copy.

    `d.x=s.x; d.y=s.y-C; d.z=s.z;` and
    `d.x=s.x; d.y=s.y; d.z=s.z; d.y-=C;` have the same final value for a
    nonvolatile automatic destination.  The latter preserves the unadjusted
    stack store and gives sched2 an extra independent instruction.  Restrict
    both directions to adjacent vx/vy/vz stores, one literal adjustment, and a
    destination rooted in a local object.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    wanted = {b"vx", b"vy", b"vz"}

    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]

        # SPLIT one inline +/- literal into a raw copy plus compound update.
        for start in range(len(statements) - 2):
            group = statements[start:start + 3]
            parsed = [_vector_assignment(data, statement) for statement in group]
            if any(item is None for item in parsed):
                continue
            lhs_aggregate = parsed[0][3][0]
            if not _local_aggregate_root(lhs_aggregate, locals_):
                continue
            fields = []
            source_aggregate = None
            adjusted = None
            valid = True
            for index, (_assignment, operator, _lhs, lhs_field, rhs) in enumerate(parsed):
                if _txt(data, operator) != b"=" or lhs_field[0] != lhs_aggregate:
                    valid = False
                    break
                fields.append(lhs_field[1])
                plain = _vector_field_access(data, _unparen(rhs))
                if plain is not None and plain[1] == lhs_field[1]:
                    source = plain
                else:
                    binary = _unparen(rhs)
                    if binary is None or binary.type != "binary_expression":
                        valid = False
                        break
                    bop = binary.child_by_field_name("operator")
                    source_node = _unparen(binary.child_by_field_name("left"))
                    left = _vector_field_access(data, source_node)
                    literal = _literal_text(data, binary.child_by_field_name("right"))
                    if (bop is None or _txt(data, bop) not in (b"+", b"-") or
                            left is None or left[1] != lhs_field[1] or literal is None or
                            adjusted is not None):
                        valid = False
                        break
                    source = left
                    adjusted = (index, rhs, source_node, _txt(data, bop), literal)
                if source_aggregate is None:
                    source_aggregate = source[0]
                elif source[0] != source_aggregate:
                    valid = False
                    break
            if (not valid or set(fields) != wanted or len(set(fields)) != 3 or
                    adjusted is None):
                continue
            first, last = group[0], group[-1]
            line = _line(data, group[adjusted[0]].start_byte)
            if not _guided_site(line):
                continue
            index, rhs, source_node, operator, literal = adjusted
            lhs_text = lhs_aggregate + b"." + fields[index]
            raw = data[first.start_byte:last.end_byte]
            rs, re_ = rhs.start_byte - first.start_byte, rhs.end_byte - first.start_byte
            raw = raw[:rs] + _txt(data, source_node).strip() + raw[re_:]
            indent = _indent_at(data, last.start_byte)
            raw += b"\n" + indent + lhs_text + b" " + operator + b"= " + literal + b";"
            yield (
                f"vector-copy-adjust split {fields[index].decode()} L{line}",
                splice(data, first.start_byte, last.end_byte, raw).decode(),
            )

        # MERGE the exact inverse four-statement form.
        for start in range(len(statements) - 3):
            copies = statements[start:start + 3]
            adjust_statement = statements[start + 3]
            parsed = [_vector_assignment(data, statement) for statement in copies]
            adjust = _vector_assignment(data, adjust_statement)
            if any(item is None for item in parsed) or adjust is None:
                continue
            lhs_aggregate = parsed[0][3][0]
            if not _local_aggregate_root(lhs_aggregate, locals_):
                continue
            fields, source_aggregate, rhs_nodes = [], None, {}
            valid = True
            for _assignment, operator, _lhs, lhs_field, rhs in parsed:
                source = _vector_field_access(data, _unparen(rhs))
                if (_txt(data, operator) != b"=" or lhs_field[0] != lhs_aggregate or
                        source is None or source[1] != lhs_field[1]):
                    valid = False
                    break
                fields.append(lhs_field[1])
                rhs_nodes[lhs_field[1]] = rhs
                if source_aggregate is None:
                    source_aggregate = source[0]
                elif source[0] != source_aggregate:
                    valid = False
                    break
            _a, adjust_op, _lhs, adjust_field, adjust_rhs = adjust
            literal = _literal_text(data, adjust_rhs)
            op = _txt(data, adjust_op)
            if (not valid or set(fields) != wanted or len(set(fields)) != 3 or
                    adjust_field[0] != lhs_aggregate or adjust_field[1] not in wanted or
                    op not in (b"+=", b"-=") or literal is None or
                    data[copies[-1].end_byte:adjust_statement.start_byte].strip()):
                continue
            line = _line(data, adjust_statement.start_byte)
            if not _guided_site(line):
                continue
            rhs = rhs_nodes[adjust_field[1]]
            first, last = copies[0], copies[-1]
            raw = data[first.start_byte:last.end_byte]
            rs, re_ = rhs.start_byte - first.start_byte, rhs.end_byte - first.start_byte
            adjusted_rhs = _txt(data, rhs).strip() + b" " + op[:1] + b" " + literal
            raw = raw[:rs] + adjusted_rhs + raw[re_:]
            yield (
                f"vector-copy-adjust merge {adjust_field[1].decode()} L{line}",
                splice(data, first.start_byte, adjust_statement.end_byte, raw).decode(),
            )


def _zero_or_one(data, node):
    literal = _literal_text(data, node)
    if literal is None:
        return None
    try:
        value = int(literal, 0)
    except ValueError:
        return None
    return value if value in (0, 1) else None


def rule_flag_return_split(text, name, span):
    """Split a local 0/1 result pseudo into two literal return sites.

    ``flag = 0; if (test) { ...; flag = 1; ...; } return flag;`` keeps the
    default definition live across ``test`` and copy-preferences both values
    through one return pseudo.  Literal returns at the two CFG exits can use
    ``$v0`` independently and shorten the success arm's live ranges.

    Keep the rewrite deliberately narrow: the three outer statements and the
    override must be direct children of compound blocks; ``flag`` is a unique,
    nonvolatile 32-bit automatic scalar referenced only by its declaration,
    the two definitions, and the final return; both values are 0/1; and the
    success arm contains no early exit or label.  Moving the success return to
    the closing brace is then semantics-preserving even when useful work
    follows the override.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    locals_ = _nonvolatile_local_names(data, body)
    wide_locals = set()
    declarations = {}
    for declaration in _find(body, ("declaration",)):
        type_node = declaration.child_by_field_name("type")
        if type_node is None:
            continue
        type_name = _txt(data, type_node).decode(errors="ignore")
        if (type_name not in TYPES or TYPES[type_name][0] != 32 or
                re.search(rb"\b(?:const|volatile|static|extern|register)\b",
                          _txt(data, declaration))):
            continue
        declared = []
        for child in declaration.named_children:
            declarator = (child.child_by_field_name("declarator")
                          if child.type == "init_declarator" else child)
            if declarator is None or declarator.type != "identifier":
                continue
            declared.append((declarator, child))
        # The rule removes the now-unused declaration, so accept only one
        # uninitialized scalar declarator on its own declaration statement.
        if len(declared) != 1 or declared[0][1].type != "identifier":
            continue
        ident = _txt(data, declared[0][0])
        declarations.setdefault(ident, []).append(declaration)
        wide_locals.add(ident)
    wide_locals &= locals_

    identifiers = _find(body, ("identifier",))
    unsafe = ("break_statement", "continue_statement", "goto_statement",
              "return_statement", "labeled_statement")

    def return_identifier(statement):
        if statement.type != "return_statement":
            return None
        values = [child for child in statement.named_children
                  if child.type != "comment"]
        if len(values) != 1 or values[0].type != "identifier":
            return None
        return _txt(data, values[0])

    def standalone_line(node):
        start = data.rfind(b"\n", 0, node.start_byte) + 1
        end = data.find(b"\n", node.end_byte)
        if end < 0:
            end = len(data)
        else:
            end += 1
        if (data[start:node.start_byte].strip() or
                data[node.end_byte:end].strip()):
            return None
        return start, end

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for default_statement, iff, final_return in zip(
                statements, statements[1:], statements[2:]):
            default = _plain_assignment(data, default_statement)
            if (default is None or iff.type != "if_statement" or
                    iff.child_by_field_name("alternative") is not None or
                    data[default_statement.end_byte:iff.start_byte].strip() or
                    data[iff.end_byte:final_return.start_byte].strip()):
                continue
            flag, default_rhs = default
            default_value = _zero_or_one(data, default_rhs)
            if (flag not in wide_locals or len(declarations.get(flag, ())) != 1 or
                    return_identifier(final_return) != flag or
                    default_value is None):
                continue

            # Exactly four occurrences prove that the local is otherwise
            # unobserved: declaration, default LHS, override LHS, final return.
            occurrences = [ident for ident in identifiers
                           if _txt(data, ident) == flag]
            if len(occurrences) != 4:
                continue

            consequence = iff.child_by_field_name("consequence")
            if (consequence is None or
                    consequence.type != "compound_statement" or
                    _find(consequence, unsafe)):
                continue
            arm_statements = [child for child in consequence.named_children
                              if child.type != "comment"]
            overrides = []
            for statement in arm_statements:
                assignment = _plain_assignment(data, statement)
                if assignment is not None and assignment[0] == flag:
                    overrides.append((statement, assignment[1]))
            if len(overrides) != 1:
                continue
            override_statement, override_rhs = overrides[0]
            override_value = _zero_or_one(data, override_rhs)
            if override_value is None or {default_value, override_value} != {0, 1}:
                continue

            declaration_line = standalone_line(declarations[flag][0])
            default_line = standalone_line(default_statement)
            override_line = standalone_line(override_statement)
            close_line = data.rfind(b"\n", consequence.start_byte,
                                    consequence.end_byte)
            if (declaration_line is None or default_line is None or
                    override_line is None or
                    close_line <= consequence.start_byte or
                    data[close_line + 1:consequence.end_byte - 1].strip()):
                continue
            line = _line(data, iff.start_byte)
            if not _guided_site(line):
                continue

            body_indent = _indent_at(data, override_statement.start_byte)
            success_return = (body_indent + b"return " +
                              _txt(data, override_rhs).strip() + b";\n")
            fallback_return = (b"return " +
                               _txt(data, default_rhs).strip() + b";")
            edits = [
                (final_return.start_byte, final_return.end_byte, fallback_return),
                (close_line + 1, close_line + 1, success_return),
                (override_line[0], override_line[1], b""),
                (default_line[0], default_line[1], b""),
                (declaration_line[0], declaration_line[1], b""),
            ]
            changed = data
            for start, end, replacement in sorted(edits, reverse=True):
                changed = splice(changed, start, end, replacement)
            yield (
                f"flag-return-split {flag.decode()} L{line}",
                changed.decode(),
            )


def rule_flag_arm_assign(text, name, span):
    """Move a local 0/1 default between pre-zero and two-arm CFG forms.

    This shortens the flag's live range past the comparisons that precede each
    definition, allowing their hard result register to be reused in the branch
    delay slots.  Only the safe adjacent shape is enumerated: a nonvolatile
    local, no existing else, no use of the flag in the condition/body, and no
    early exit that could bypass the moved override.

    The reverse spelling is also useful: ``if (...) { work; flag = 1; } else
    { flag = 0; }`` becomes ``flag = 0; if (...) { work; flag = 1; }``.  Old
    cc1 can then fall through the false path instead of emitting an
    unconditional jump around the else assignment.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    # Binding identity matters here: a same-spelled declaration in a nested
    # arm can make the two apparent assignments target different objects.
    locals_ = _nonvolatile_function_local_names(data, body)
    known_names = locals_ | _nonvolatile_parameter_names(data, body)
    exits = ("break_statement", "continue_statement", "goto_statement",
             "return_statement")

    # Follow unary `&` through parentheses.  A regex misses `&(flag)` and
    # comment-separated variants, after which an alias could observe an
    # assignment moved across the condition or true-arm prefix.
    addressed = set()
    for pointer in _find(body, ("pointer_expression",)):
        operator = pointer.child_by_field_name("operator")
        argument = _unparen(pointer.child_by_field_name("argument"))
        if (operator is not None and _txt(data, operator) == b"&" and
                argument is not None and argument.type == "identifier"):
            addressed.add(_txt(data, argument))

    # Parsing precedes preprocessing.  Reject every visible macro name in a
    # moved-over region; an included macro is covered by the unknown-name
    # check below.  Function-like macros also parse as calls and are rejected.
    macro_names = set(re.findall(
        rb"(?m)^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)", data
    ))

    def observation_free(flag, nodes):
        """Whether moving a flag definition across `nodes` is conservative."""
        for node in nodes:
            if _find(node, ("call_expression",)):
                return False
            identifiers = {
                _txt(data, identifier)
                for identifier in _find(node, ("identifier",))
            }
            if (flag in identifiers or identifiers & macro_names or
                    not identifiers <= known_names):
                return False
        return True

    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]
        for default_statement, iff in zip(statements, statements[1:]):
            default = _plain_assignment(data, default_statement)
            if default is None or iff.type != "if_statement" or \
                    iff.child_by_field_name("alternative") is not None:
                continue
            flag, default_rhs = default
            default_value = _zero_or_one(data, default_rhs)
            condition = iff.child_by_field_name("condition")
            consequence = iff.child_by_field_name("consequence")
            if (flag not in locals_ or flag in addressed or
                    default_value is None or condition is None or
                    consequence is None or consequence.type != "compound_statement" or
                    data[default_statement.end_byte:iff.start_byte].strip()):
                continue
            arm_statements = [c for c in consequence.named_children if c.type != "comment"]
            if not arm_statements:
                continue
            override_statement = arm_statements[0]
            override = _plain_assignment(data, override_statement)
            if override is None or override[0] != flag:
                continue
            override_value = _zero_or_one(data, override[1])
            if override_value is None or {default_value, override_value} != {0, 1}:
                continue
            if data[consequence.start_byte + 1:override_statement.start_byte].strip():
                continue
            remaining = arm_statements[1:]
            if any(_find(statement, exits) for statement in remaining):
                continue
            if not observation_free(flag, [condition] + remaining):
                continue
            line = _line(data, iff.start_byte)
            if not _guided_site(line):
                continue

            # Remove the leading override's whole line, then insert it directly
            # before the arm's closing brace. Preserve the original condition,
            # nested body, comments, and formatting around every other statement.
            yes = data[consequence.start_byte:consequence.end_byte]
            first_line_start = data.rfind(b"\n", consequence.start_byte,
                                           override_statement.start_byte) + 1
            first_line_end = data.find(b"\n", override_statement.end_byte,
                                       consequence.end_byte)
            if first_line_end < 0:
                continue
            first_line_end += 1
            rel_start = first_line_start - consequence.start_byte
            rel_end = first_line_end - consequence.start_byte
            yes = yes[:rel_start] + yes[rel_end:]
            close = yes.rfind(b"}")
            close_line = yes.rfind(b"\n", 0, close) + 1
            indent = _indent_at(data, iff.start_byte)
            body_indent = indent + b"    "
            override_text = _txt(data, override_statement).strip()
            yes = (yes[:close_line] + body_indent + override_text + b"\n" +
                   yes[close_line:])
            prefix = data[iff.start_byte:consequence.start_byte]
            default_text = _txt(data, default_statement).strip()
            repl = (prefix + yes + b"\n" + indent + b"else\n" + indent + b"{\n" +
                    body_indent + default_text + b"\n" + indent + b"}")
            yield (
                f"flag-arm-assign {flag.decode()} L{line}",
                splice(data, default_statement.start_byte, iff.end_byte, repl).decode(),
            )

    # Reverse the two-arm form into a pre-zero/no-else form.  Keep this as
    # narrow as the forward rewrite: the false arm is only the default, the
    # true arm ends in the opposite override, and no earlier statement can
    # observe the flag or leave before that override.
    for iff in _find(body, ("if_statement",)):
        condition = iff.child_by_field_name("condition")
        consequence = iff.child_by_field_name("consequence")
        alternative = iff.child_by_field_name("alternative")
        if condition is None or consequence is None or alternative is None:
            continue
        if alternative.type == "else_clause":
            arms = [c for c in alternative.named_children if c.type != "comment"]
            if len(arms) != 1:
                continue
            alternative = arms[0]
        if (consequence.type != "compound_statement" or
                alternative.type != "compound_statement"):
            continue
        yes_statements = [c for c in consequence.named_children
                          if c.type != "comment"]
        no_statements = [c for c in alternative.named_children
                         if c.type != "comment"]
        if not yes_statements or len(no_statements) != 1:
            continue
        override_statement = yes_statements[-1]
        default_statement = no_statements[0]
        override = _plain_assignment(data, override_statement)
        default = _plain_assignment(data, default_statement)
        if override is None or default is None or override[0] != default[0]:
            continue
        flag = override[0]
        override_value = _zero_or_one(data, override[1])
        default_value = _zero_or_one(data, default[1])
        if (flag not in locals_ or flag in addressed or override_value is None or
                default_value is None or
                {override_value, default_value} != {0, 1}):
            continue
        before_override = yes_statements[:-1]
        if (any(_find(statement, exits) for statement in before_override) or
                not observation_free(flag, [condition] + before_override)):
            continue
        line = _line(data, iff.start_byte)
        if not _guided_site(line):
            continue

        indent = _indent_at(data, iff.start_byte)
        default_text = _txt(data, default_statement).strip()
        one_arm_if = (data[iff.start_byte:consequence.start_byte] +
                      _txt(data, consequence))
        repl = default_text + b"\n" + indent + one_arm_if
        yield (
            f"flag-arm-prezero {flag.decode()} L{line}",
            splice(data, iff.start_byte, iff.end_byte, repl).decode(),
        )


def rule_default_ladder_hoist(text, name, span):
    """Hoist a three-way literal result's default before two comparisons.

    ``if (A) x=ONE; else { x=DEFAULT; if (B) x=TWO; }`` and
    ``x=DEFAULT; if (A) x=ONE; else if (B) x=TWO;`` are equivalent for a
    nonvolatile unaddressed automatic local.  The latter gives gcc 2.8.1 a
    default definition for a branch delay slot and keeps the two overrides as
    explicit outcome islands; it closed Think4contact's 183-byte CFG residual.

    Keep the rewrite narrow: exactly three plain literal assignments, no work
    or declarations in the arms, no second else, and neither condition reads
    the destination local.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)

    def only_statement(compound):
        if compound is None or compound.type != "compound_statement":
            return None
        statements = [child for child in compound.named_children
                      if child.type != "comment"]
        return statements[0] if len(statements) == 1 else None

    for iff in _find(body, ("if_statement",)):
        condition = iff.child_by_field_name("condition")
        yes = iff.child_by_field_name("consequence")
        alternative = iff.child_by_field_name("alternative")
        if condition is None or yes is None or alternative is None or \
                alternative.type != "else_clause":
            continue
        alternatives = [child for child in alternative.named_children
                        if child.type != "comment"]
        if len(alternatives) != 1 or alternatives[0].type != "compound_statement":
            continue
        no = alternatives[0]
        no_statements = [child for child in no.named_children
                         if child.type != "comment"]
        if len(no_statements) != 2 or no_statements[1].type != "if_statement":
            continue
        default_statement, nested = no_statements
        nested_condition = nested.child_by_field_name("condition")
        nested_yes = nested.child_by_field_name("consequence")
        if nested_condition is None or nested_yes is None or \
                nested.child_by_field_name("alternative") is not None:
            continue
        yes_statement = only_statement(yes)
        nested_statement = only_statement(nested_yes)
        first = (_plain_assignment(data, yes_statement)
                 if yes_statement is not None else None)
        default = _plain_assignment(data, default_statement)
        second = (_plain_assignment(data, nested_statement)
                  if nested_statement is not None else None)
        if (first is None or default is None or second is None or
                not (first[0] == default[0] == second[0])):
            continue
        local = first[0]
        if (local not in locals_ or
                re.search(rb"&\s*\(?\s*" + re.escape(local) + rb"\b",
                          body_text) or
                not all(_integer_constant(rhs)
                        for _lhs, rhs in (first, default, second)) or
                any(_txt(data, ident) == local
                    for cond in (condition, nested_condition)
                    for ident in _find(cond, ("identifier",)))):
            continue
        line = _line(data, iff.start_byte)
        if not any(_guided_site(candidate_line) for candidate_line in (
                line, _line(data, nested.start_byte))):
            continue
        indent = _indent_at(data, iff.start_byte)
        outer = (data[iff.start_byte:yes.start_byte] + _txt(data, yes))
        replacement = (
            _txt(data, default_statement).strip() + b"\n" + indent + outer +
            b"\n" + indent + b"else " + _txt(data, nested)
        )
        yield (
            f"default-ladder-hoist {local.decode()} L{line}",
            splice(data, iff.start_byte, iff.end_byte, replacement).decode(),
        )


def rule_guard_flag_assign(text, name, span):
    """Toggle a local flag definition across a single-goto guard.

    ``flag = 1; if (reject) { goto done; }`` and the edge-local spelling
    ``if (reject) { flag = 1; goto done; } flag = 1;`` are equivalent for a
    nonvolatile, non-address-taken local when the condition provably cannot
    read the flag.  The latter gives jump/reorg identical definitions on the
    taken and fallthrough edges, which can merge the definition into the guard
    branch's delay slot while shortening its live range past the comparison.

    Keep this deliberately narrow: a literal 0/1 (or false/true), adjacent
    statements, no else, a macro/call-free condition containing only known
    locals/parameters, no call-like use that could hide an address capture of
    the flag, and a compound arm containing only the goto (sink) or the
    matching assignment plus goto (hoist). Both directions are enumerated so
    scoring can recover either source shape.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)

    # Inspect unary address expressions through any number of parentheses.
    # A text regex misses spellings such as `&(flag)` and
    # `&(/* alias */ flag)`, after which a pointer-based guard can observe the
    # assignment we are considering moving.
    addressed = set()
    for pointer in _find(body, ("pointer_expression",)):
        operator = pointer.child_by_field_name("operator")
        argument = _unparen(pointer.child_by_field_name("argument"))
        if (operator is not None and _txt(data, operator) == b"&" and
                argument is not None and argument.type == "identifier"):
            addressed.add(_txt(data, argument))

    # Source parsing happens before preprocessing, so an object-like macro in
    # a condition is only an innocent-looking identifier to tree-sitter. Keep
    # the transform conservative around every macro defined in this TU,
    # including continued definitions.
    macro_definitions = {}
    source_lines = data.splitlines(keepends=True)
    line_index = 0
    while line_index < len(source_lines):
        line = source_lines[line_index]
        match = re.match(
            rb"[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)", line
        )
        if match is None:
            line_index += 1
            continue
        definition = [line]
        while (definition[-1].rstrip(b"\r\n").endswith(b"\\") and
               line_index + 1 < len(source_lines)):
            line_index += 1
            definition.append(source_lines[line_index])
        macro_definitions[match.group(1)] = b"".join(definition)
        line_index += 1

    # A condition identifier is accepted only when it names a variable whose
    # binding is visible in this function. Unknown identifiers may be macros
    # supplied by an included header, which this unpreprocessed source cannot
    # expand or prove independent of the flag.
    known_condition_names = set(locals_)
    function = body.parent
    declarator = (function.child_by_field_name("declarator")
                  if function is not None else None)
    if declarator is not None:
        for parameter in _find(declarator, ("parameter_declaration",)):
            parameter_declarator = parameter.child_by_field_name("declarator")
            identifier = _descend_ident(parameter_declarator)
            if identifier is not None:
                known_condition_names.add(_txt(data, identifier))

    def flag_literal(node):
        node = _unparen(node)
        if node is None:
            return None
        raw = _txt(data, node).strip()
        if node.type == "number_literal" and raw in (b"0", b"1"):
            return int(raw)
        if node.type in ("false", "true") and raw in (b"false", b"true"):
            return int(raw == b"true")
        return None

    def safe(flag, rhs, condition):
        if (flag not in locals_ or flag_literal(rhs) is None or
                flag in addressed):
            return False

        condition_identifiers = {
            _txt(data, identifier)
            for identifier in _find(condition, ("identifier",))
        }
        if (_find(condition, ("call_expression",)) or
                flag in condition_identifiers or
                condition_identifiers & macro_definitions.keys() or
                not condition_identifiers <= known_condition_names):
            return False

        # `CAPTURE(flag)` may be a function-like macro that expands to
        # `save(&flag)`, even though the raw source contains no `&flag`.
        # Likewise a no-argument/object macro whose replacement names `flag`
        # can capture it invisibly. Reject both forms anywhere in the function.
        for call in _find(body, ("call_expression",)):
            if any(_txt(data, identifier) == flag
                   for identifier in _find(call, ("identifier",))):
                return False
        for identifier in _find(body, ("identifier",)):
            macro = macro_definitions.get(_txt(data, identifier))
            if (macro is not None and
                    re.search(rb"\b" + re.escape(flag) + rb"\b", macro)):
                return False
        return True

    def render_guard(iff, consequence, arm_statements):
        indent = _indent_at(data, iff.start_byte)
        body_indent = indent + b"    "
        prefix = data[iff.start_byte:consequence.start_byte].rstrip()
        rendered = [prefix, b"\n", indent, b"{\n"]
        for statement in arm_statements:
            rendered.extend((body_indent, statement, b"\n"))
        rendered.extend((indent, b"}"))
        return b"".join(rendered)

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for index, iff in enumerate(statements):
            if (iff.type != "if_statement" or
                    iff.child_by_field_name("alternative") is not None):
                continue
            condition = iff.child_by_field_name("condition")
            consequence = iff.child_by_field_name("consequence")
            if (condition is None or consequence is None or
                    consequence.type != "compound_statement" or
                    any(child.type == "comment"
                        for child in consequence.named_children)):
                continue
            arm = [child for child in consequence.named_children
                   if child.type != "comment"]
            line = _line(data, iff.start_byte)

            # Pre-guard definition -> one definition on each outgoing edge.
            if index > 0 and len(arm) == 1 and arm[0].type == "goto_statement":
                previous = statements[index - 1]
                parsed = _plain_assignment(data, previous)
                if (parsed is not None and
                        not data[previous.end_byte:iff.start_byte].strip() and
                        safe(parsed[0], parsed[1], condition) and
                        (_guided_site(line) or
                         _guided_site(_line(data, previous.start_byte)))):
                    assignment = _txt(data, previous).strip()
                    guard = render_guard(
                        iff, consequence,
                        [assignment, _txt(data, arm[0]).strip()],
                    )
                    indent = _indent_at(data, iff.start_byte)
                    replacement = guard + b"\n" + indent + assignment
                    yield (
                        f"guard-flag-assign sink {parsed[0].decode()} L{line}",
                        splice(data, previous.start_byte, iff.end_byte,
                               replacement).decode(),
                    )

            # Matching edge definitions -> one definition before the guard.
            if (len(arm) != 2 or arm[1].type != "goto_statement" or
                    index + 1 >= len(statements)):
                continue
            arm_assignment = _plain_assignment(data, arm[0])
            following = statements[index + 1]
            fallthrough_assignment = _plain_assignment(data, following)
            if (arm_assignment is None or fallthrough_assignment is None or
                    arm_assignment[0] != fallthrough_assignment[0] or
                    flag_literal(arm_assignment[1]) !=
                    flag_literal(fallthrough_assignment[1]) or
                    data[iff.end_byte:following.start_byte].strip() or
                    not safe(arm_assignment[0], arm_assignment[1], condition) or
                    not (_guided_site(line) or
                         _guided_site(_line(data, following.start_byte)))):
                continue
            assignment = _txt(data, arm[0]).strip()
            guard = render_guard(
                iff, consequence, [_txt(data, arm[1]).strip()]
            )
            indent = _indent_at(data, iff.start_byte)
            replacement = assignment + b"\n" + indent + guard
            yield (
                f"guard-flag-assign hoist {arm_assignment[0].decode()} L{line}",
                splice(data, iff.start_byte, following.end_byte,
                       replacement).decode(),
            )


def rule_guard_exit_copy(text, name, span):
    """Move a simple local copy across a loop's unique break guard.

    ``copy = value; if (test(value)) break;`` and the arm-local spelling
    ``if (test(value)) { copy = value; break; }`` give the copy different RTL
    live ranges and allocation preferences even when reorg puts the move in the
    same branch delay slot.  Enumerate both placements for the narrow shape we
    can justify mechanically: a nonvolatile, non-address-taken local; a pure
    condition using the copied identifier; one break in the loop; and no other
    reference to the destination inside that loop.  A forward goto out of the
    loop is allowed only when its target and everything after it never read the
    destination (debug_menu_player_jump's alternate action path).
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)
    labels = {}
    for labelled in _find(body, ("labeled_statement",)):
        label = labelled.child_by_field_name("label")
        if label is not None:
            labels[_txt(data, label)] = labelled

    def loop_safe(loop, destination):
        if len(_find(loop, ("break_statement",))) != 1:
            return False
        if sum(_txt(data, ident) == destination
               for ident in _find(loop, ("identifier",))) != 1:
            return False
        # The value must be observable on the ordinary loop-exit path; without
        # a later use this rewrite is only dead-source churn.
        if not any(_txt(data, ident) == destination and ident.start_byte >= loop.end_byte
                   for ident in _find(body, ("identifier",))):
            return False
        for jump in _find(loop, ("goto_statement",)):
            label = jump.child_by_field_name("label")
            target = labels.get(_txt(data, label)) if label is not None else None
            if target is None or target.start_byte <= loop.end_byte:
                return False
            if any(_txt(data, ident) == destination and
                   ident.start_byte >= target.start_byte
                   for ident in _find(body, ("identifier",))):
                return False
        return True

    def parse_copy(statement, condition):
        parsed = _plain_assignment(data, statement)
        if parsed is None:
            return None
        destination, rhs = parsed
        rhs = _unparen(rhs)
        condition_ids = [_txt(data, ident)
                         for ident in _find(condition, ("identifier",))]
        if (destination not in locals_ or rhs is None or
                rhs.type != "identifier" or destination == _txt(data, rhs) or
                destination in condition_ids or _txt(data, rhs) not in condition_ids or
                re.search(rb"&\s*" + re.escape(destination) + rb"\b", body_text) or
                _find(condition, ("assignment_expression", "update_expression",
                                  "call_expression", "comma_expression"))):
            return None
        return destination, _txt(data, rhs)

    def render_if(iff, consequence, statements):
        indent = _indent_at(data, iff.start_byte)
        body_indent = indent + b"    "
        prefix = data[iff.start_byte:consequence.start_byte].rstrip()
        rendered = [prefix, b"\n", indent, b"{\n"]
        for statement in statements:
            rendered.extend((body_indent, statement, b"\n"))
        rendered.extend((indent, b"}"))
        return b"".join(rendered)

    loops = ("while_statement", "for_statement", "do_statement")
    for block in _find(body, ("compound_statement",)):
        loop = block.parent
        if (loop is None or loop.type not in loops or
                loop.child_by_field_name("body") != block):
            continue
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for index, iff in enumerate(statements):
            if (iff.type != "if_statement" or
                    iff.child_by_field_name("alternative") is not None):
                continue
            condition = iff.child_by_field_name("condition")
            consequence = iff.child_by_field_name("consequence")
            if (condition is None or consequence is None or
                    consequence.type != "compound_statement" or
                    any(child.type == "comment" for child in consequence.named_children)):
                continue
            arm = [child for child in consequence.named_children
                   if child.type != "comment"]
            line = _line(data, iff.start_byte)

            # Arm-local copy -> pre-guard copy.
            if (len(arm) == 2 and arm[1].type == "break_statement" and
                    not data[arm[0].end_byte:arm[1].start_byte].strip()):
                parsed = parse_copy(arm[0], condition)
                if (parsed is not None and loop_safe(loop, parsed[0]) and
                        (_guided_site(line) or
                         _guided_site(_line(data, arm[0].start_byte)))):
                    assignment = _txt(data, arm[0]).strip()
                    replacement = (assignment + b"\n" +
                                   _indent_at(data, iff.start_byte) +
                                   render_if(iff, consequence,
                                             [_txt(data, arm[1]).strip()]))
                    yield (
                        f"guard-exit-copy hoist {parsed[0].decode()} L{line}",
                        splice(data, iff.start_byte, iff.end_byte,
                               replacement).decode(),
                    )

            # Adjacent pre-guard copy -> arm-local copy.
            if (len(arm) != 1 or arm[0].type != "break_statement" or index == 0):
                continue
            previous = statements[index - 1]
            if data[previous.end_byte:iff.start_byte].strip():
                continue
            parsed = parse_copy(previous, condition)
            if (parsed is None or not loop_safe(loop, parsed[0]) or
                    not (_guided_site(line) or
                         _guided_site(_line(data, previous.start_byte)))):
                continue
            replacement = render_if(
                iff, consequence,
                [_txt(data, previous).strip(), _txt(data, arm[0]).strip()],
            )
            yield (
                f"guard-exit-copy sink {parsed[0].decode()} L{line}",
                splice(data, previous.start_byte, iff.end_byte,
                       replacement).decode(),
            )


def rule_shared_tail_assign(text, name, span):
    """Duplicate one shared assignment into both immediately preceding arms.

    ``if (...) { ... } else { ... } flag = 1`` and the arm-local spelling are
    semantically equivalent, but the latter gives sched/jump2 two independent
    producers.  That can retain a shared store at the target arm and can move a
    constant definition across the join without adding code.  Keep the rewrite
    deliberately local: both arms must be compounds, the shared statement must
    be a plain assignment, and no comment or other token may separate the if
    from that assignment.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]
        for iff, tail in zip(statements, statements[1:]):
            if iff.type != "if_statement" or _plain_assignment(data, tail) is None:
                continue
            yes = iff.child_by_field_name("consequence")
            no = iff.child_by_field_name("alternative")
            if no is not None and no.type == "else_clause":
                arms = [c for c in no.named_children if c.type != "comment"]
                if len(arms) != 1:
                    continue
                no = arms[0]
            if (yes is None or no is None or
                    yes.type != "compound_statement" or
                    no.type != "compound_statement" or
                    b"\n" not in _txt(data, yes) or
                    b"\n" not in _txt(data, no) or
                    data[iff.end_byte:tail.start_byte].strip()):
                continue
            line = _line(data, tail.start_byte)
            if not (_guided_site(line) or _guided_site(_line(data, iff.start_byte))):
                continue

            indent = _indent_at(data, iff.start_byte)
            body_indent = indent + b"    "
            tail_text = _txt(data, tail).strip()

            def append_to_arm(arm):
                raw = _txt(data, arm)
                close = raw.rfind(b"}")
                close_line = raw.rfind(b"\n", 0, close) + 1
                return (raw[:close_line] + body_indent + tail_text + b"\n" +
                        raw[close_line:])

            repl = (data[iff.start_byte:yes.start_byte] + append_to_arm(yes) +
                    data[yes.end_byte:no.start_byte] + append_to_arm(no))
            lhs, _rhs = _plain_assignment(data, tail)
            yield (
                f"shared-tail-assign {lhs.decode()} L{line}",
                splice(data, iff.start_byte, tail.end_byte, repl).decode(),
            )


def rule_shared_writeback_compound(text, name, span):
    """Inline a shared scalar writeback into direct compound-update arms.

    Old cc1 can allocate a manually factored load/result/store chain quite
    differently from source that updates the field in each predecessor and
    lets jump2 rediscover the common store::

        alias = base;                 base->field += delta;
        old = (T)alias->field;   ->   goto done;
        result = old + delta;
        goto writeback;
        ...
    writeback:
        alias->field = result;
        goto done;

    This guided rule only handles a shared label reached by at least two
    byte-adjacent four-statement producer chains.  The base must be a pure
    identifier/field path, the old-value read must be the exact writeback
    field, and the arithmetic is restricted to ``old +/- pure_read`` (or the
    commutative ``pure_read + old``).  Every incoming goto must fit, so removing
    the shared label cannot strand an edge.

    ActNORMAL also needed the arm containing one direct update to become the
    physical fallthrough arm before jump2 merged the stores.  Emit the useful
    *atomic* companions which invert a containing compound if/else as well as
    the plain direct-writeback candidate; this crosses the measured +1-size
    intermediate without relying on a broad stochastic search.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    def strip_casts(node):
        node = _unparen(node)
        while node is not None and node.type == "cast_expression":
            node = _unparen(node.child_by_field_name("value"))
        return node

    def pure_read(node):
        return node is not None and not _find(node, (
            "assignment_expression", "call_expression", "update_expression",
            "comma_expression",
        ))

    def field_assignment(statement):
        if statement is None or statement.type != "expression_statement":
            return None
        expressions = [child for child in statement.named_children
                       if child.type != "comment"]
        if len(expressions) != 1 or expressions[0].type != "assignment_expression":
            return None
        assignment = expressions[0]
        operator = assignment.child_by_field_name("operator")
        lhs = assignment.child_by_field_name("left")
        rhs = assignment.child_by_field_name("right")
        if (operator is None or _txt(data, operator) != b"=" or lhs is None or
                lhs.type != "field_expression" or rhs is None):
            return None
        return lhs, rhs

    def goto_label(statement):
        if statement.type != "goto_statement":
            return None
        label = statement.child_by_field_name("label")
        return _txt(data, label) if label is not None else None

    # A label is eligible only when its own statement is the field store and
    # the immediately following sibling is the shared terminal exit.
    for labeled in _find(body, ("labeled_statement",)):
        label_node = labeled.child_by_field_name("label")
        label = _txt(data, label_node) if label_node is not None else None
        children = [child for child in labeled.named_children
                    if child != label_node and child.type != "comment"]
        if label is None or len(children) != 1:
            continue
        stored = field_assignment(children[0])
        if stored is None:
            continue
        store_lhs, store_rhs = stored
        store_path = _field_path(data, store_lhs)
        result_node = strip_casts(store_rhs)
        if (store_path is None or result_node is None or
                result_node.type != "identifier"):
            continue
        alias, writeback_path = store_path
        result = _txt(data, result_node)

        parent = labeled.parent
        if parent is None:
            continue
        siblings = [child for child in parent.named_children
                    if child.type != "comment"]
        try:
            label_index = siblings.index(labeled)
        except ValueError:
            continue
        if label_index + 1 >= len(siblings):
            continue
        terminal = siblings[label_index + 1]
        if (terminal.type not in {"goto_statement", "return_statement",
                                  "break_statement"} or
                data[labeled.end_byte:terminal.start_byte].strip()):
            continue

        incoming = [statement for statement in _find(body, ("goto_statement",))
                    if goto_label(statement) == label]
        if len(incoming) < 2:
            continue

        producers = []
        valid = True
        for jump in incoming:
            block = jump.parent
            if block is None or block.type != "compound_statement":
                valid = False
                break
            statements = [child for child in block.named_children
                          if child.type != "comment"]
            try:
                jump_index = statements.index(jump)
            except ValueError:
                valid = False
                break
            if jump_index < 3:
                valid = False
                break
            alias_stmt, current_stmt, result_stmt = statements[jump_index - 3:jump_index]
            chain = [alias_stmt, current_stmt, result_stmt, jump]
            if any(data[left.end_byte:right.start_byte].strip()
                   for left, right in zip(chain, chain[1:])):
                valid = False
                break

            alias_assignment = _plain_assignment(data, alias_stmt)
            current_assignment = _plain_assignment(data, current_stmt)
            result_assignment = _plain_assignment(data, result_stmt)
            if (alias_assignment is None or current_assignment is None or
                    result_assignment is None or alias_assignment[0] != alias or
                    result_assignment[0] != result):
                valid = False
                break
            current, current_rhs = current_assignment
            base_node = strip_casts(alias_assignment[1])
            current_field = strip_casts(current_rhs)
            current_path = (_field_path(data, current_field)
                            if current_field is not None else None)
            if (base_node is None or current_path != (alias, writeback_path) or
                    not pure_read(base_node)):
                valid = False
                break
            base = _txt(data, base_node).strip()
            if re.fullmatch(
                    rb"[A-Za-z_]\w*(?:(?:->|\.)[A-Za-z_]\w*)*", base) is None:
                valid = False
                break

            expression = strip_casts(result_assignment[1])
            if expression is None or expression.type != "binary_expression":
                valid = False
                break
            operator_node = expression.child_by_field_name("operator")
            left = expression.child_by_field_name("left")
            right = expression.child_by_field_name("right")
            operator = _txt(data, operator_node) if operator_node is not None else None
            left_value = strip_casts(left)
            right_value = strip_casts(right)
            left_is_current = (left_value is not None and
                               left_value.type == "identifier" and
                               _txt(data, left_value) == current)
            right_is_current = (right_value is not None and
                                right_value.type == "identifier" and
                                _txt(data, right_value) == current)
            if operator == b"+" and left_is_current and pure_read(right):
                other = right
            elif operator == b"+" and right_is_current and pure_read(left):
                other = left
            elif operator == b"-" and left_is_current and pure_read(right):
                other = right
            else:
                valid = False
                break

            direct_lhs = base + writeback_path[len(alias):]
            indent = _indent_at(data, alias_stmt.start_byte)
            replacement = (direct_lhs + b" " + operator + b"= " +
                           _txt(data, other).strip() + b";\n" + indent +
                           _txt(data, terminal).strip())
            producers.append((alias_stmt.start_byte, jump.end_byte,
                              replacement, direct_lhs + b" " + operator + b"="))

        if not valid:
            continue
        lines = [_line(data, start) for start, _end, _replacement, _mark in producers]
        if not (any(_guided_site(line) for line in lines) or
                _guided_site(_line(data, labeled.start_byte))):
            continue

        replacements = [(start, end, replacement)
                        for start, end, replacement, _mark in producers]
        replacements.append((labeled.start_byte, terminal.end_byte, b""))
        collapsed = data
        for start, end, replacement in sorted(replacements, reverse=True):
            collapsed = splice(collapsed, start, end, replacement)

        tag = f"shared-writeback-compound {label.decode()} x{len(producers)}"
        yield (tag, collapsed.decode())

        # Pair the collapse with each containing arm inversion. Reparse because
        # the first phase changed byte offsets; only ifs that now contain one
        # of our direct compound updates are eligible.
        collapsed_body = _func_body(collapsed, name, (0, len(collapsed)))
        if collapsed_body is None:
            continue
        markers = {mark for _start, _end, _replacement, mark in producers}
        for iff in _find(collapsed_body, ("if_statement",)):
            condition = iff.child_by_field_name("condition")
            yes = iff.child_by_field_name("consequence")
            no = iff.child_by_field_name("alternative")
            if condition is None or yes is None or no is None:
                continue
            if no.type == "else_clause":
                arms = [child for child in no.named_children
                        if child.type != "comment"]
                if len(arms) != 1:
                    continue
                no = arms[0]
            if yes.type != "compound_statement" or no.type != "compound_statement":
                continue
            arm_text = _txt(collapsed, yes) + _txt(collapsed, no)
            if not any(marker in arm_text for marker in markers):
                continue
            line = _line(collapsed, iff.start_byte)
            if not _guided_site(line):
                continue
            inner = (_paren_inner(condition)
                     if condition.type == "parenthesized_expression"
                     else condition)
            if inner is None:
                continue
            condition_text = _txt(collapsed, inner).strip()
            if inner.type == "binary_expression":
                comparison = inner.child_by_field_name("operator")
                replacement_operator = ({b"==": b"!=", b"!=": b"=="}.get(
                    _txt(collapsed, comparison)) if comparison is not None else None)
                if replacement_operator is not None:
                    relative_start = comparison.start_byte - inner.start_byte
                    relative_end = comparison.end_byte - inner.start_byte
                    condition_text = (condition_text[:relative_start] +
                                      replacement_operator +
                                      condition_text[relative_end:])
                else:
                    condition_text = b"!(" + condition_text + b")"
            else:
                condition_text = b"!(" + condition_text + b")"
            replacement = (b"if (" + condition_text + b") " +
                           _txt(collapsed, no) + b" else " +
                           _txt(collapsed, yes))
            yield (
                f"{tag} + if-else-invert L{line}",
                splice(collapsed, iff.start_byte, iff.end_byte,
                       replacement).decode(),
            )


def rule_shared_terminal_tail(text, name, span):
    """Duplicate an adjacent assignment+return tail into both if/else arms.

    A terminal ``if/else; flag = K; return value;`` and two complete arm-local
    tails are behaviorally identical, but the latter gives sched2 and jump2
    independent producers and hard CODE_LABEL boundaries before cross-jump
    factors the common suffix.  This is the terminal complement of
    ``shared-tail-assign`` used by ActACTION.

    Moving source into a compound arm could change identifier binding if that
    arm declares a shadowing local, so reject every arm containing a declaration.
    Require exact adjacency and compound arms; the transformation never crosses
    comments or invents a distant switch-case goto.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for index in range(len(statements) - 2):
            iff, assignment, returned = statements[index:index + 3]
            if (iff.type != "if_statement" or
                    _plain_assignment(data, assignment) is None or
                    returned.type != "return_statement"):
                continue
            yes = iff.child_by_field_name("consequence")
            no = iff.child_by_field_name("alternative")
            if no is not None and no.type == "else_clause":
                alternatives = [child for child in no.named_children
                                if child.type != "comment"]
                if len(alternatives) != 1:
                    continue
                no = alternatives[0]
            if (yes is None or no is None or
                    yes.type != "compound_statement" or
                    no.type != "compound_statement" or
                    _find(yes, ("declaration",)) or
                    _find(no, ("declaration",)) or
                    data[iff.end_byte:assignment.start_byte].strip() or
                    data[assignment.end_byte:returned.start_byte].strip()):
                continue
            lines = (
                _line(data, iff.start_byte),
                _line(data, assignment.start_byte),
                _line(data, returned.start_byte),
            )
            if not any(_guided_site(line) for line in lines):
                continue

            indent = _indent_at(data, iff.start_byte)
            body_indent = indent + b"    "
            tail = (_txt(data, assignment).strip() + b"\n" + body_indent +
                    _txt(data, returned).strip())

            def append_to_arm(arm):
                raw = _txt(data, arm)
                close = raw.rfind(b"}")
                close_line = raw.rfind(b"\n", 0, close) + 1
                return (raw[:close_line] + body_indent + tail + b"\n" +
                        raw[close_line:])

            replacement = (
                data[iff.start_byte:yes.start_byte] + append_to_arm(yes) +
                data[yes.end_byte:no.start_byte] + append_to_arm(no)
            )
            lhs, _rhs = _plain_assignment(data, assignment)
            yield (
                f"shared-terminal-tail {lhs.decode()} L{lines[0]}-{lines[2]}",
                splice(data, iff.start_byte, returned.end_byte,
                       replacement).decode(),
            )


def rule_shared_return_split(text, name, span):
    """Replace one goto to a final return label with that literal return.

    ``goto done; ... done: return value;`` and ``return value;`` are
    semantically equivalent on that edge.  They are not scheduler-equivalent:
    before jump2 merges the tails, the second source return contributes a hard
    CODE_LABEL that can keep a narrow return conversion above the epilogue
    restores.  Only labels whose immediate statement is exactly one return are
    eligible.  The return label itself is kept so other incoming edges and the
    ordinary fallthrough remain unchanged.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    returns = {}
    for labeled in _find(body, ("labeled_statement",)):
        label = labeled.child_by_field_name("label")
        statements = [
            child for child in labeled.named_children
            if child != label and child.type != "comment"
        ]
        if label is None or len(statements) != 1 or \
                statements[0].type != "return_statement":
            continue
        returns[_txt(data, label)] = statements[0]

    for goto in _find(body, ("goto_statement",)):
        label = goto.child_by_field_name("label")
        if label is None:
            continue
        return_statement = returns.get(_txt(data, label))
        if return_statement is None:
            continue
        goto_line = _line(data, goto.start_byte)
        return_line = _line(data, return_statement.start_byte)
        if not (_guided_site(goto_line) or _guided_site(return_line)):
            continue
        yield (
            f"shared-return-split {_txt(data, label).decode()} L{goto_line}",
            splice(
                data, goto.start_byte, goto.end_byte,
                _txt(data, return_statement).strip(),
            ).decode(),
        )


def rule_shared_result_return(text, name, span):
    """Funnel one nested local return and the final expression through a label.

    ``if (...) { T result; ...; return result; } return fallback();`` and a
    function-scope ``result`` assigned on both paths before one labeled return
    have the same target-width value.  They are not allocation-equivalent in
    gcc 2.8.1: the single result pseudo can keep the early value in its source
    register across unrelated arithmetic and emit one shared narrow conversion.
    Think3chase lost two instructions only after making that source funnel.

    This is deliberately a guided, two-return rewrite.  Require one plain,
    unaddressed automatic integer local, a final direct return, equal local and
    function return widths, and no same-named references outside the local's
    original scope.  Those checks make hoisting the declaration and replacing
    the early return with a forward goto semantics-preserving; scoring decides
    whether the old compiler wants the shared pseudo or the split form.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None or body.parent is None or \
            body.parent.type != "function_definition":
        return

    function = body.parent
    return_type = function.child_by_field_name("type")
    return_name = (_txt(data, return_type).decode().strip()
                   if return_type is not None else "")
    if return_name not in TYPES:
        return

    returns = _find(body, ("return_statement",))
    direct = [child for child in body.named_children
              if child.type != "comment"]
    if len(returns) != 2 or not direct or \
            direct[-1].type != "return_statement":
        return
    final = direct[-1]
    early = next((statement for statement in returns
                  if statement != final), None)
    if early is None:
        return

    def return_value(statement):
        values = [child for child in statement.named_children
                  if child.type != "comment"]
        return values[0] if len(values) == 1 else None

    early_value = _unparen(return_value(early))
    final_value = return_value(final)
    if early_value is None or early_value.type != "identifier" or \
            final_value is None:
        return
    local = _txt(data, early_value)
    if any(_txt(data, ident) == local
           for ident in _find(final_value, ("identifier",))):
        return

    # Find the unique plain scalar declaration of the returned local.  A
    # multi-declarator, initializer, pointer/array, qualifier, or shadowed
    # parameter would make the scope expansion unsafe.
    declarations = []
    for declaration in _find(body, ("declaration",)):
        declarator = declaration.child_by_field_name("declarator")
        ident = _descend_ident(declarator) if declarator is not None else None
        if ident is not None and _txt(data, ident) == local:
            declarations.append((declaration, declarator))
    if len(declarations) != 1:
        return
    declaration, declarator = declarations[0]
    type_node = declaration.child_by_field_name("type")
    type_name = (_txt(data, type_node).decode().strip()
                 if type_node is not None else "")
    declaration_text = _txt(data, declaration)
    if (type_name not in TYPES or declarator.type != "identifier" or
            TYPES[type_name][0] != TYPES[return_name][0] or
            re.search(rb"\b(?:const|volatile|static|register|extern)\b",
                      declaration_text) or
            re.search(rb"[,=*\[]", declaration_text)):
        return

    function_declarator = function.child_by_field_name("declarator")
    if function_declarator is None or any(
            _txt(data, ident) == local
            for parameter in _find(function_declarator,
                                   ("parameter_declaration",))
            for ident in _find(parameter, ("identifier",))):
        return
    body_text = _txt(data, body)
    if re.search(rb"&\s*\(?\s*" + re.escape(local) + rb"\b", body_text):
        return

    declaration_block = declaration.parent
    if declaration_block is None or declaration_block.type != "compound_statement":
        return
    ancestor = early.parent
    while ancestor is not None and ancestor != declaration_block:
        ancestor = ancestor.parent
    if ancestor is None:
        return
    # Hoisting must not capture a global/macro reference that was outside the
    # nested declaration's old lexical scope.
    if any(_txt(data, ident) == local and
           not (declaration_block.start_byte <= ident.start_byte <
                declaration_block.end_byte)
           for ident in _find(body, ("identifier",))):
        return

    lines = (_line(data, early.start_byte), _line(data, final.start_byte))
    if not any(_guided_site(line) for line in lines):
        return

    label_base = b"_match_return_" + local
    label = label_base
    suffix = 2
    while re.search(rb"\b" + re.escape(label) + rb"\b", body_text):
        label = label_base + str(suffix).encode()
        suffix += 1

    edits = [
        (early.start_byte, early.end_byte, b"goto " + label + b";"),
    ]
    final_indent = _indent_at(data, final.start_byte)
    final_replacement = (
        local + b" = " + _txt(data, final_value).strip() + b";\n" +
        label + b":\n" + final_indent + b"return " + local + b";"
    )
    edits.append((final.start_byte, final.end_byte, final_replacement))

    if declaration_block != body:
        # Insert after the function's direct declaration prefix.  If there is
        # none, place the hoisted declaration on the first body line.
        direct_declarations = []
        for child in direct:
            if child.type != "declaration":
                break
            direct_declarations.append(child)
        raw_declaration = declaration_text.strip()
        if direct_declarations:
            anchor = direct_declarations[-1]
            insertion = (b"\n" + _indent_at(data, anchor.start_byte) +
                         raw_declaration)
            edits.append((anchor.end_byte, anchor.end_byte, insertion))
        else:
            indent = (_indent_at(data, direct[0].start_byte)
                      if direct else _indent_at(data, body.start_byte) + b"    ")
            edits.append((body.start_byte + 1, body.start_byte + 1,
                          b"\n" + indent + raw_declaration))
        edits.append((declaration.start_byte, declaration.end_byte, b""))

    rewritten = data
    last_start = len(data) + 1
    for start, end, replacement in sorted(edits, reverse=True):
        if end > last_start:
            return
        rewritten = splice(rewritten, start, end, replacement)
        last_start = start
    yield (
        f"shared-result-return {local.decode()} L{lines[0]}->{lines[1]}",
        rewritten.decode(),
    )


def rule_terminal_call_return(text, name, span):
    """Return one terminal branch call directly instead of joining a result.

    ``if (...) { result = call(); } ... return result;`` and ``return call();``
    are equivalent on that branch when the top-level conditional is immediately
    followed by the final return.  The split source return can leave the call
    result in ``$v0`` and begin a narrow conversion in the jump delay slot;
    Think3hitaway's status-7 arm went from a 14-byte schedule residual to exact.

    Restrict this to one plain unaddressed integer local, equal local/function
    return widths, a direct call RHS, and an assignment terminal through every
    enclosing compound/else up to the adjacent top-level conditional.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None or body.parent is None or \
            body.parent.type != "function_definition":
        return
    direct = [child for child in body.named_children
              if child.type != "comment"]
    if len(direct) < 2 or direct[-1].type != "return_statement" or \
            direct[-2].type != "if_statement":
        return
    conditional, final = direct[-2], direct[-1]
    values = [child for child in final.named_children
              if child.type != "comment"]
    value = _unparen(values[0]) if len(values) == 1 else None
    if value is None or value.type != "identifier":
        return
    local = _txt(data, value)

    function_type = body.parent.child_by_field_name("type")
    function_type_name = (_txt(data, function_type).decode().strip()
                          if function_type is not None else "")
    declarations = []
    for declaration in _find(body, ("declaration",)):
        declarator = declaration.child_by_field_name("declarator")
        ident = _descend_ident(declarator) if declarator is not None else None
        if ident is not None and _txt(data, ident) == local:
            declarations.append((declaration, declarator))
    if len(declarations) != 1 or function_type_name not in TYPES:
        return
    declaration, declarator = declarations[0]
    type_node = declaration.child_by_field_name("type")
    type_name = (_txt(data, type_node).decode().strip()
                 if type_node is not None else "")
    if (type_name not in TYPES or declarator.type != "identifier" or
            TYPES[type_name][0] != TYPES[function_type_name][0] or
            re.search(rb"\b(?:const|volatile|static|register|extern)\b",
                      _txt(data, declaration)) or
            re.search(rb"[,=*\[]", _txt(data, declaration)) or
            re.search(rb"&\s*\(?\s*" + re.escape(local) + rb"\b",
                      _txt(data, body))):
        return

    def terminal_to_top(statement):
        node = statement
        while node != conditional:
            parent = node.parent
            if parent is None:
                return False
            if parent.type in ("compound_statement", "else_clause"):
                children = [child for child in parent.named_children
                            if child.type != "comment"]
                if not children or children[-1] != node:
                    return False
            elif parent.type == "if_statement":
                if node not in (parent.child_by_field_name("consequence"),
                                parent.child_by_field_name("alternative")):
                    return False
            else:
                return False
            node = parent
        return True

    for statement in _find(conditional, ("expression_statement",)):
        assignment = _plain_assignment(data, statement)
        if assignment is None or assignment[0] != local or \
                not terminal_to_top(statement):
            continue
        rhs = _unparen(assignment[1])
        if rhs is None or rhs.type != "call_expression":
            continue
        lines = (_line(data, statement.start_byte), _line(data, final.start_byte))
        if not any(_guided_site(line) for line in lines):
            continue
        yield (
            f"terminal-call-return {local.decode()} L{lines[0]}",
            splice(data, statement.start_byte, statement.end_byte,
                   b"return " + _txt(data, rhs).strip() + b";").decode(),
        )


def rule_difference_role_fuse(text, name, span):
    """Fuse a scratch-loaded subtraction chain into direct difference locals.

    ``x=A; scratch=B; z=C; x=x-scratch; z=z-D;`` can be written
    ``x=A-B; z=C-D;``.  The direct form makes x/z the subtraction result
    identities from their first definitions, which matters when they survive as
    later call arguments.  Think1random's old split roles were an exact-length
    ten-byte allocation tie; this fusion matched directly.

    Restrict this to five adjacent assignments, pure field-read operands,
    nonvolatile unaddressed locals, and a scratch whose first later occurrence
    is a plain overwrite.  Thus removing the scratch definition cannot expose
    an uninitialised read.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)
    identifiers = _find(body, ("identifier",))

    def field_read(node):
        node = _unparen(node)
        if node is None or node.type != "field_expression" or _find(node, (
                "call_expression", "assignment_expression", "update_expression",
                "subscript_expression", "pointer_expression")):
            return None
        return _txt(data, node).strip()

    def subtraction(rhs, expected_left):
        rhs = _unparen(rhs)
        if rhs is None or rhs.type != "binary_expression":
            return None
        operator = rhs.child_by_field_name("operator")
        left = _unparen(rhs.child_by_field_name("left"))
        right = _unparen(rhs.child_by_field_name("right"))
        if (operator is None or _txt(data, operator) != b"-" or
                left is None or left.type != "identifier" or
                _txt(data, left) != expected_left or right is None):
            return None
        return right

    def first_later_is_overwrite(local, end):
        later = sorted((ident for ident in identifiers
                        if ident.start_byte >= end and _txt(data, ident) == local),
                       key=lambda ident: ident.start_byte)
        if not later:
            return True
        ident = later[0]
        node = ident.parent
        while node is not None and node.type not in (
                "assignment_expression", "expression_statement"):
            node = node.parent
        if node is None or node.type != "assignment_expression":
            return False
        operator = node.child_by_field_name("operator")
        left = _unparen(node.child_by_field_name("left"))
        return (operator is not None and _txt(data, operator) == b"=" and
                left is not None and left.type == "identifier" and
                _txt(data, left) == local)

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for index in range(len(statements) - 4):
            group = statements[index:index + 5]
            if any(statement.type != "expression_statement"
                   for statement in group):
                continue
            if any(data[left.end_byte:right.start_byte].strip()
                   for left, right in zip(group, group[1:])):
                continue
            parsed = [_plain_assignment(data, statement) for statement in group]
            if any(assignment is None for assignment in parsed):
                continue
            x, first_rhs = parsed[0]
            scratch, scratch_rhs = parsed[1]
            z, third_rhs = parsed[2]
            if (len({x, scratch, z}) != 3 or
                    any(local not in locals_ for local in (x, scratch, z)) or
                    re.search(rb"&\s*\(?\s*" + re.escape(scratch) + rb"\b",
                              body_text)):
                continue
            x_right = subtraction(parsed[3][1], x)
            z_right = subtraction(parsed[4][1], z)
            if (parsed[3][0] != x or parsed[4][0] != z or
                    x_right is None or x_right.type != "identifier" or
                    _txt(data, x_right) != scratch or z_right is None):
                continue
            first = field_read(first_rhs)
            scratch_value = field_read(scratch_rhs)
            third = field_read(third_rhs)
            fourth = field_read(z_right)
            if any(value is None
                   for value in (first, scratch_value, third, fourth)) or \
                    not first_later_is_overwrite(scratch, group[-1].end_byte):
                continue
            line = _line(data, group[0].start_byte)
            if not any(_guided_site(_line(data, statement.start_byte))
                       for statement in group):
                continue
            indent = _indent_at(data, group[0].start_byte)
            replacement = (
                x + b" = " + first + b" - " + scratch_value + b";\n" +
                indent + z + b" = " + third + b" - " + fourth + b";"
            )
            yield (
                f"difference-role-fuse {x.decode()}/{z.decode()} L{line}",
                splice(data, group[0].start_byte, group[-1].end_byte,
                       replacement).decode(),
            )


def rule_switch_cse_evict(text, name, span):
    """Dead-overwrite an entry index local before re-reading it in a switch.

    ``entry = obj->mode; ... switch (obj->mode)`` is normally CSE'd to one
    load.  A dead ``entry = 0`` immediately before the switch invalidates that
    equivalence without emitting code, so ``expand_case`` receives the fresh
    field load visible in the target.  This is semantics-preserving only when
    the automatic local is never used from the switch onward; enforce that and
    reject address-taken/volatile locals.  Guided beam search can retain the
    byte-neutral eviction until a following structural edit benefits from it.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)
    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for position, switch in enumerate(statements):
            if switch.type != "switch_statement":
                continue
            condition = _unparen(switch.child_by_field_name("condition"))
            if condition is None:
                continue
            condition_text = _txt(data, condition).strip()
            prior = statements[:position]
            assignments = []
            for statement in prior:
                assignment = _plain_assignment(data, statement)
                if assignment is not None and \
                        _txt(data, _unparen(assignment[1])).strip() == condition_text:
                    assignments.append((statement, assignment[0]))
            if not assignments:
                continue
            _definition, local = assignments[-1]
            if local not in locals_:
                continue
            # Do not duplicate an eviction already present immediately before
            # the switch, regardless of the harmless constant it used.
            if prior:
                immediate = _plain_assignment(data, prior[-1])
                if immediate is not None and immediate[0] == local:
                    continue
            if re.search(rb"&\s*" + re.escape(local) + rb"\b", body_text):
                continue
            if any(_txt(data, ident) == local
                   for ident in _find(body, ("identifier",))
                   if ident.start_byte >= switch.start_byte):
                continue
            line = _line(data, switch.start_byte)
            if not _guided_site(line):
                continue
            indent = _indent_at(data, switch.start_byte)
            repl = local + b" = 0;\n" + indent + _txt(data, switch)
            yield (
                f"switch-cse-evict {local.decode()} L{line}",
                splice(data, switch.start_byte, switch.end_byte, repl).decode(),
            )


def rule_min_ternary(text, name, span):
    """Merge a two-statement min/max into a ternary:
    `x = a; if (b < x) x = b;` -> `x = (b < a) ? b : a;` (min), and the `>` max form.

    A conditional min/max against a MEMORY operand must be the ternary or the two-stmt
    form re-expands the arm's memory read in the destination's narrow mode and cannot CSE
    (cookbook; fixed 22/26 bytes of UpdateMotion). Same value; scoring decides. Only the
    exact adjacent `X = A;` then `if (B REL X) X = B;` shape, no else."""
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for block in _find(body, ("compound_statement",)):
        kids = [c for c in block.named_children if c.type != "comment"]
        for i in range(len(kids) - 1):
            s1, s2 = kids[i], kids[i + 1]
            asg = _stmt_expr(data, s1)
            if asg is None or s2.type != "if_statement":
                continue
            x, a = asg
            if s2.child_by_field_name("alternative") is not None:
                continue  # no else
            cond = s2.child_by_field_name("condition")
            cons = s2.child_by_field_name("consequence")
            inner = _paren_inner(cond) if cond is not None and cond.type == "parenthesized_expression" else cond
            body2 = _stmt_expr(data, cons) if cons is not None and cons.type != "compound_statement" else \
                (_stmt_expr(data, [c for c in cons.named_children if c.type != "comment"][0])
                 if cons is not None and cons.type == "compound_statement"
                 and len([c for c in cons.named_children if c.type != "comment"]) == 1 else None)
            if inner is None or inner.type != "binary_expression" or body2 is None:
                continue
            op = inner.child_by_field_name("operator")
            bl = inner.child_by_field_name("left")
            br = inner.child_by_field_name("right")
            if op is None or _txt(data, op) not in (b"<", b">"):
                continue
            # want `B REL X` with body `X = B` -> min/max of (X's old value a, B)
            bx, by = _txt(data, bl).strip(), _txt(data, br).strip()
            if by != x:
                continue  # right operand must be X (the running value)
            bx2, b_rhs = body2
            if bx2 != x or b_rhs != bx:
                continue  # body must be `X = B` (all bytes)
            rel = _txt(data, op).decode()
            repl = (f"{x.decode()} = ({bx.decode()} {rel} {a.decode()}) ? "
                    f"{bx.decode()} : {a.decode()};").encode()
            yield (f"min-ternary {x.decode()} L{_line(data, s1.start_byte)}",
                   splice(data, s1.start_byte, s2.end_byte, repl).decode())


def rule_clamp_shared_return(text, name, span):
    """Turn two direct clamp returns into assignments plus one shared return.

    ``if (x > HI) return HI; if (x < LO) return LO; return x;`` and
    ``if (x > HI) x = HI; else if (x < LO) x = LO; return x;`` are equivalent
    for a nonvolatile automatic 32-bit scalar.  The latter keeps one return
    pseudo and lets jump2 build a shared clamp tail; FUN_8002fd9c went from a
    29-byte residual to exact with this rewrite.

    Restrict the transform to three adjacent statements, literal bounds, and
    a plain wide local.  That avoids changing observable stores or conversion
    behavior for narrow locals; authoritative scoring chooses the useful form.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    locals_ = _nonvolatile_local_names(data, body)
    wide_locals = set()
    for declaration in _find(body, ("declaration",)):
        type_node = declaration.child_by_field_name("type")
        if type_node is None:
            continue
        type_name = _txt(data, type_node).decode()
        if type_name not in TYPES or TYPES[type_name][0] != 32:
            continue
        for child in declaration.named_children:
            declarator = (child.child_by_field_name("declarator")
                          if child.type == "init_declarator" else child)
            ident = _descend_ident(declarator) if declarator is not None else None
            if ident is not None:
                wide_locals.add(_txt(data, ident))
    wide_locals &= locals_

    def return_value(statement):
        if statement is None or statement.type != "return_statement":
            return None
        values = [child for child in statement.named_children
                  if child.type != "comment"]
        return values[0] if len(values) == 1 else None

    literal = re.compile(rb"[+-]?(?:0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*\Z")
    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for first, second, final in zip(
                statements, statements[1:], statements[2:]):
            if (first.type != "if_statement" or
                    second.type != "if_statement" or
                    first.child_by_field_name("alternative") is not None or
                    second.child_by_field_name("alternative") is not None or
                    data[first.end_byte:second.start_byte].strip() or
                    data[second.end_byte:final.start_byte].strip()):
                continue

            first_return = return_value(first.child_by_field_name("consequence"))
            second_return = return_value(second.child_by_field_name("consequence"))
            final_return = return_value(final)
            if first_return is None or second_return is None or final_return is None:
                continue
            local = _txt(data, final_return).strip()
            if local not in wide_locals:
                continue

            matches = []
            for conditional, returned in ((first, first_return),
                                          (second, second_return)):
                condition = conditional.child_by_field_name("condition")
                condition = (_paren_inner(condition)
                             if condition is not None and
                             condition.type == "parenthesized_expression"
                             else condition)
                if condition is None or condition.type != "binary_expression":
                    matches = []
                    break
                operator = condition.child_by_field_name("operator")
                left = _unparen(condition.child_by_field_name("left"))
                right = _unparen(condition.child_by_field_name("right"))
                if (operator is None or left is None or right is None or
                        left.type != "identifier" or
                        _txt(data, left).strip() != local):
                    matches = []
                    break
                op_text = _txt(data, operator).strip()
                bound = _txt(data, right).strip()
                if (op_text not in (b"<", b"<=", b">", b">=") or
                        not literal.fullmatch(bound) or
                        _txt(data, returned).strip() != bound):
                    matches = []
                    break
                matches.append((op_text, bound,
                                _txt(data, conditional.child_by_field_name(
                                    "condition")).strip()))
            if len(matches) != 2:
                continue
            directions = ({b">", b">="}, {b"<", b"<="})
            if not ((matches[0][0] in directions[0] and
                     matches[1][0] in directions[1]) or
                    (matches[0][0] in directions[1] and
                     matches[1][0] in directions[0])):
                continue

            indent = _indent_at(data, first.start_byte)
            body_indent = indent + b"    "
            replacement = (
                b"if " + matches[0][2] + b"\n" + body_indent + local +
                b" = " + matches[0][1] + b";\n" + indent + b"else if " +
                matches[1][2] + b"\n" + body_indent + local + b" = " +
                matches[1][1] + b";\n" + indent + b"return " + local + b";"
            )
            yield (
                f"clamp-shared-return {local.decode()} "
                f"L{_line(data, first.start_byte)}",
                splice(data, first.start_byte, final.end_byte,
                       replacement).decode(),
            )


CMP_SWAP = {b"<": b">", b">": b"<", b"<=": b">=", b">=": b"<="}


def rule_cmp_swap(text, name, span):
    """Swap a comparison's operands to flip evaluation order: `a > b` -> `b < a`.

    expand evaluates op0 first, so `a > mem` emits the (short) sign-extension slls before
    the load while `mem < a` loads first (cookbook). Same slt; scoring decides. Only fires
    where exactly one side is a MEMORY access (field/subscript/deref) — narrows the site
    count to the cases where it matters."""
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    def is_mem(n):
        return n.type in ("field_expression", "subscript_expression", "pointer_expression")
    for c in _find(body, ("binary_expression",)):
        op = c.child_by_field_name("operator")
        L = c.child_by_field_name("left")
        R = c.child_by_field_name("right")
        if op is None or L is None or R is None:
            continue
        sw = CMP_SWAP.get(_txt(data, op))
        if sw is None:
            continue
        if is_mem(L) == is_mem(R):
            continue  # exactly one side must be memory
        repl = _txt(data, R).strip() + b" " + sw + b" " + _txt(data, L).strip()
        yield (f"cmp-swap L{_line(data, c.start_byte)}",
               splice(data, c.start_byte, c.end_byte, repl).decode())


def rule_cmp_polarity(text, name, span):
    """Swap two non-memory comparison operands: `a < b` -> `b > a`.

    The slt is logically identical, but fold creates and references the two
    operand pseudos in the opposite order.  That can swap their hard-register
    homes without changing an instruction (ActATTACK).  This broader form is
    guided/opt-in; ordinary cmp-swap stays on cheaper memory/local sites.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    memory = {"field_expression", "subscript_expression", "pointer_expression"}
    side_effects = {"call_expression", "update_expression", "assignment_expression"}
    literals = {"number_literal", "char_literal", "string_literal",
                "true", "false", "null"}
    for comp in _find(body, ("binary_expression",)):
        op = comp.child_by_field_name("operator")
        left = comp.child_by_field_name("left")
        right = comp.child_by_field_name("right")
        if op is None or left is None or right is None:
            continue
        swapped = CMP_SWAP.get(_txt(data, op))
        if swapped is None:
            continue
        if (_find(left, tuple(memory | side_effects)) or
                _find(right, tuple(memory | side_effects))):
            continue
        if left.type in literals or right.type in literals:
            continue
        line = _line(data, comp.start_byte)
        if not _guided_site(line):
            continue
        repl = (_txt(data, right).strip() + b" " + swapped + b" " +
                _txt(data, left).strip())
        yield (f"cmp-polarity L{line}",
               splice(data, comp.start_byte, comp.end_byte, repl).decode())


def rule_eq_literal_swap(text, name, span):
    """Swap ``value ==/!= LITERAL`` to ``LITERAL ==/!= value``.

    Equality is commutative, but old cc1 creates/references the load and
    constant pseudos in source operand order.  Reversing them can exchange a
    final ``$v0/$v1`` pair without changing CFG or instruction count.  Limit
    this guided lever to exactly one literal operand and reject calls,
    assignments, and updates, so no observable evaluation order moves.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    literal_types = {"number_literal", "char_literal", "true", "false", "null"}
    side_effects = ("call_expression", "update_expression",
                    "assignment_expression")
    for comp in _find(body, ("binary_expression",)):
        operator = comp.child_by_field_name("operator")
        left = comp.child_by_field_name("left")
        right = comp.child_by_field_name("right")
        if (operator is None or left is None or right is None or
                _txt(data, operator) not in (b"==", b"!=") or
                ((left.type in literal_types) == (right.type in literal_types)) or
                _find(left, side_effects) or _find(right, side_effects)):
            continue
        line = _line(data, comp.start_byte)
        if not _guided_site(line):
            continue
        repl = (_txt(data, right).strip() + b" " + _txt(data, operator) +
                b" " + _txt(data, left).strip())
        yield (f"eq-literal-swap L{line}",
               splice(data, comp.start_byte, comp.end_byte, repl).decode())


def rule_shift16_mul(text, name, span):
    """Respell a casted narrow `x << 16` as `x * 0x10000`.

    For a declared short, old cc1 emits the multiply spelling as one raw `sll`.
    Cast-and-shift spellings can instead introduce `andi` or `sll/sra`
    canonicalization (DamageControl). Restrict this to a bare identifier with a
    visible 16-bit declaration; the broader algebraic rewrite is not safe.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for expr in _find(body, ("binary_expression",)):
        operator = expr.child_by_field_name("operator")
        left = expr.child_by_field_name("left")
        right = expr.child_by_field_name("right")
        if (operator is None or left is None or right is None or
                _txt(data, operator) != b"<<"):
            continue
        try:
            shift = int(_txt(data, right).decode(), 0)
        except ValueError:
            continue
        if shift != 16:
            continue
        casts = []
        value = left
        while value.type in ("cast_expression", "parenthesized_expression"):
            if value.type == "cast_expression":
                ty = value.child_by_field_name("type")
                if ty is not None:
                    casts.append(_txt(data, ty).decode())
                value = value.child_by_field_name("value")
            else:
                value = _paren_inner(value)
            if value is None:
                break
        narrow_casts = {
            "s16", "u16", "short", "signed short", "unsigned short",
        }
        normalized_casts = {" ".join(cast.split()) for cast in casts}
        if (value is None or value.type != "identifier" or
                not (normalized_casts & narrow_casts)):
            continue
        ident = _txt(data, value).decode()
        narrow_decl = re.compile(
            rf"\b(?:s16|u16|short|unsigned\s+short|signed\s+short)\s+{re.escape(ident)}\b"
        )
        comment_mask = _comment_mask(text)
        if not any(not comment_mask[m.start()] for m in narrow_decl.finditer(text)):
            continue
        line = _line(data, expr.start_byte)
        if not _guided_site(line):
            continue
        repl = b"(" + _txt(data, value).strip() + b" * 0x10000)"
        yield (
            f"shift16-mul {ident} L{line}",
            splice(data, expr.start_byte, expr.end_byte, repl).decode(),
        )


def rule_split_chain(text, name, span):
    """Split a fused shift-of-a-binary into two statements:
    `t = (x - C) >> S;` -> `t = x - C; t = t >> S;` (declaration keeps its type).

    A fused `subtract-then-shift` (or any inner binary then a shift) can tie on which
    register the intermediate lands in; splitting it into two statements pins the load's
    destination to match the target (cookbook; fixed 2-byte register ties in DrawFrame
    and DrawAfterimage). Same value; scoring decides. Only when the RHS is
    `<binary> <shift-op> <operand>` and the LHS is a plain identifier."""
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    def emit(stmt, lhs, decl_prefix, val):
        # val is the `E1 >> E2` binary_expression node; E1 must itself be binary.
        vop = val.child_by_field_name("operator")
        e1 = val.child_by_field_name("left")
        e2 = val.child_by_field_name("right")
        if vop is None or e1 is None or e2 is None:
            return None
        if _txt(data, vop) not in (b">>", b"<<"):
            return None
        # strip an outer paren on e1 to test its kind, but keep its text as-is
        inner = _paren_inner(e1) if e1.type == "parenthesized_expression" else e1
        if inner is None or inner.type != "binary_expression":
            return None
        # indentation of the statement's line
        ls = data.rfind(b"\n", 0, stmt.start_byte) + 1
        indent = data[ls:stmt.start_byte]
        if indent.strip():
            indent = b""  # not at line start; bail on injecting a 2nd line cleanly
        two = (decl_prefix + lhs + b" = " + _txt(data, e1).strip() + b";\n" +
               indent + lhs + b" = " + lhs + b" " + _txt(data, vop) + b" " +
               _txt(data, e2).strip() + b";")
        return two

    for stmt in _find(body, ("expression_statement", "declaration")):
        if stmt.type == "expression_statement":
            e = [c for c in stmt.named_children if c.type != "comment"]
            if len(e) != 1 or e[0].type != "assignment_expression":
                continue
            a = e[0]
            if a.child_by_field_name("operator") is None or \
               _txt(data, a.child_by_field_name("operator")) != b"=":
                continue
            lhs = a.child_by_field_name("left")
            val = a.child_by_field_name("right")
            if lhs is None or lhs.type != "identifier" or val is None or \
               val.type != "binary_expression":
                continue
            two = emit(stmt, _txt(data, lhs).strip(), b"", val)
        else:  # declaration
            idecs = [c for c in stmt.named_children if c.type == "init_declarator"]
            ty = stmt.child_by_field_name("type")
            if len(idecs) != 1 or ty is None:
                continue
            idn = _descend_ident(idecs[0].child_by_field_name("declarator"))
            val = idecs[0].child_by_field_name("value")
            if idn is None or val is None or val.type != "binary_expression":
                continue
            two = emit(stmt, _txt(data, idn).strip(), _txt(data, ty).strip() + b" ", val)
        if two is None:
            continue
        yield (f"split-chain L{_line(data, stmt.start_byte)}",
               splice(data, stmt.start_byte, stmt.end_byte, two).decode())


def rule_dead_host_split(text, name, span):
    """Split signed ``lhs = (A +/- B) / K`` through a dead signed-s32 local.

    A same-type automatic local whose last source occurrence is before this
    statement can safely hold the promoted numerator::

        direction = (target_y - y) / 2;
        magnitude = target_y - y;
        direction = magnitude / 2;

    The extra C assignment can disappear while still joining the host's global
    allocno to the numerator and donating its local hard-register preference.
    ControlHumanoid needed this in combination with nested loop weighting.

    This is guided-only and deliberately narrower than general temporary
    invention: both locals are unaddressed, nonvolatile function-scope signed
    32-bit scalars of the same type; the host has an earlier use and no later
    source occurrence; and both the numerator and divisor are provably signed.
    Gotos, potentially capturing macro-like calls, and repeating-loop ancestors
    are rejected; one-shot weighting loops are safe because they execute once.
    """
    if not GUIDED_LINES:
        return
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    if _find(body, ("goto_statement",)):
        return
    for label in _find(body, ("labeled_statement",)):
        if _find(label, ("do_statement", "for_statement", "while_statement")):
            return

    root = body
    while root.parent is not None:
        root = root.parent
    declared_functions = set()
    for declarator in _find(root, ("function_declarator",)):
        ident = _descend_ident(declarator.child_by_field_name("declarator"))
        if ident is not None:
            declared_functions.add(_txt(data, ident))
    macro_names = set(re.findall(
        rb"(?m)^\s*#\s*define\s+([A-Za-z_]\w*)", data
    ))
    call_sites = sorted(_find(body, ("call_expression",)),
                        key=lambda node: node.start_byte)

    scalar_types = {}
    declaration_ends = {}
    for declaration in [child for child in body.named_children
                        if child.type == "declaration"]:
        raw = _txt(data, declaration)
        type_node = declaration.child_by_field_name("type")
        if (type_node is None or
                re.search(rb"\b(?:const|volatile|static|extern|_Thread_local)\b",
                          raw)):
            continue
        type_text = b" ".join(_txt(data, type_node).strip().split()).decode()
        if TYPES.get(type_text) != (32, True):
            continue
        for child in declaration.named_children:
            declarator = (child.child_by_field_name("declarator")
                          if child.type == "init_declarator" else child)
            if declarator is None or declarator.type != "identifier":
                continue
            ident = _txt(data, declarator)
            scalar_types[ident] = type_text
            declaration_ends[ident] = declarator.end_byte

    if len(scalar_types) < 2:
        return

    value_types = dict(scalar_types)
    function = body.parent
    declarator = (function.child_by_field_name("declarator")
                  if function is not None else None)
    if declarator is not None:
        for parameter in _find(declarator, ("parameter_declaration",)):
            type_node = parameter.child_by_field_name("type")
            ident = _descend_ident(parameter.child_by_field_name("declarator"))
            if type_node is None or ident is None:
                continue
            type_text = b" ".join(_txt(data, type_node).strip().split()).decode()
            if TYPES.get(type_text) == (32, True):
                value_types[_txt(data, ident)] = type_text

    shadowed = set()
    value_shadowed = set()
    for declaration in _find(body, ("declaration",)):
        if declaration.parent == body:
            continue
        for ident in _find(declaration, ("identifier",)):
            token = _txt(data, ident)
            if token in scalar_types:
                shadowed.add(token)
            if token in value_types:
                value_shadowed.add(token)

    identifiers = sorted(_find(body, ("identifier",)),
                         key=lambda node: node.start_byte)
    addressed = set()
    for ident in identifiers:
        node = ident
        parent = node.parent
        while parent is not None and parent.type == "parenthesized_expression":
            node = parent
            parent = node.parent
        if parent is None or parent.type != "pointer_expression":
            continue
        operator = parent.child_by_field_name("operator")
        if operator is not None and _txt(data, operator) == b"&":
            addressed.add(_txt(data, ident))

    def signed_s32_expression(node):
        """Whether this narrow syntax has signed-32 C arithmetic type."""
        node = _unparen(node)
        if node is None:
            return False
        if node.type == "identifier":
            token = _txt(data, node)
            return token in value_types and token not in value_shadowed
        if node.type == "number_literal":
            literal = _txt(data, node).strip()
            if re.search(rb"[uU]", literal):
                return False
            try:
                value = int(re.sub(rb"[lL]+$", b"", literal), 0)
            except ValueError:
                return False
            return 0 <= value <= 0x7fffffff
        if node.type == "unary_expression":
            operator = node.child_by_field_name("operator")
            argument = node.child_by_field_name("argument")
            return (operator is not None and _txt(data, operator) in (b"+", b"-")
                    and signed_s32_expression(argument))
        if node.type == "cast_expression":
            type_node = node.child_by_field_name("type")
            if type_node is None:
                return False
            type_text = b" ".join(_txt(data, type_node).strip().split()).decode()
            return TYPES.get(type_text) == (32, True)
        return False

    def signed_cast_value(node):
        """Value under an explicit signed-s32 cast, including typedef syntax.

        Tree-sitter has no typedef table, so ``(s32)(expr)`` can be parsed as a
        call to ``s32`` rather than as ``cast_expression``. Recognize that exact
        one-argument shape as a cast only when the token names a signed-32 type.
        """
        node = _unparen(node)
        if node is None:
            return None
        if node.type == "cast_expression":
            type_node = node.child_by_field_name("type")
            if type_node is None:
                return None
            type_text = b" ".join(_txt(data, type_node).strip().split()).decode()
            if TYPES.get(type_text) != (32, True):
                return None
            return _unparen(node.child_by_field_name("value"))
        if node.type != "call_expression":
            return None
        function = _unparen(node.child_by_field_name("function"))
        arguments = node.child_by_field_name("arguments")
        if function is None or function.type != "identifier" or arguments is None:
            return None
        type_text = _txt(data, function).decode()
        values = [child for child in arguments.named_children
                  if child.type != "comment"]
        if TYPES.get(type_text) != (32, True) or len(values) != 1:
            return None
        return _unparen(values[0])

    def has_potential_macro_capture_after(statement):
        """A later local/unknown macro may expand to a read of the host."""
        if any(ident.start_byte >= statement.end_byte and
               _txt(data, ident) in macro_names for ident in identifiers):
            return True
        for call in call_sites:
            if call.start_byte < statement.end_byte:
                continue
            function = _unparen(call.child_by_field_name("function"))
            if function is None or function.type != "identifier":
                return True
            token = _txt(data, function)
            if token in macro_names or token not in declared_functions:
                return True
        return False

    for statement in _find(body, ("expression_statement",)):
        assignment = _plain_assignment(data, statement)
        if assignment is None:
            continue
        ancestor = statement.parent
        unsafe_loop = False
        while ancestor is not None and ancestor != body:
            if ancestor.type in ("for_statement", "while_statement"):
                unsafe_loop = True
                break
            if (ancestor.type == "do_statement" and
                    _one_shot_loop(data, ancestor) is None):
                unsafe_loop = True
                break
            ancestor = ancestor.parent
        if unsafe_loop:
            continue
        if has_potential_macro_capture_after(statement):
            continue
        lhs, rhs = assignment
        line = _line(data, statement.start_byte)
        if (lhs not in scalar_types or lhs in addressed or lhs in shadowed or
                not _guided_site(line)):
            continue
        outer = _unparen(rhs)
        if outer is None or outer.type != "binary_expression":
            continue
        operator = outer.child_by_field_name("operator")
        numerator = _unparen(outer.child_by_field_name("left"))
        right = _unparen(outer.child_by_field_name("right"))
        if (operator is None or _txt(data, operator) != b"/" or
                numerator is None or
                right is None or right.type != "number_literal"):
            continue
        cast_inner = signed_cast_value(numerator)
        explicitly_signed = cast_inner is not None
        inner = cast_inner if explicitly_signed else numerator
        if inner is None or inner.type != "binary_expression":
            continue
        inner_operator = inner.child_by_field_name("operator")
        if inner_operator is None or _txt(data, inner_operator) not in (b"+", b"-"):
            continue
        if (not explicitly_signed and
                not (signed_s32_expression(inner.child_by_field_name("left")) and
                     signed_s32_expression(inner.child_by_field_name("right")))):
            continue
        literal = _txt(data, right).strip()
        if re.search(rb"[uU]", literal):
            continue
        try:
            divisor = int(re.sub(rb"[lL]+$", b"", literal), 0)
        except ValueError:
            continue
        if divisor <= 0 or divisor > 0x7fffffff:
            continue
        if _find(outer, ("assignment_expression", "update_expression")):
            continue
        calls = _find(outer, ("call_expression",))
        if any(not (explicitly_signed and
                    call.start_byte == numerator.start_byte and
                    call.end_byte == numerator.end_byte) for call in calls):
            continue
        rhs_names = {_txt(data, ident) for ident in _find(rhs, ("identifier",))}
        indent = _indent_at(data, statement.start_byte)
        candidates = []
        for host, host_type in sorted(scalar_types.items()):
            if (host == lhs or host_type != scalar_types[lhs] or
                    host in addressed or host in shadowed or host in rhs_names or
                    (GUIDED_VARIABLES and host not in GUIDED_VARIABLES)):
                continue
            earlier = any(
                declaration_ends[host] < ident.start_byte < statement.start_byte and
                _txt(data, ident) == host for ident in identifiers
            )
            later = any(
                ident.start_byte >= statement.end_byte and _txt(data, ident) == host
                for ident in identifiers
            )
            if earlier and not later:
                candidates.append(host)
        for host in candidates[:4]:
            replacement = (
                host + b" = " + _txt(data, numerator).strip() + b";\n" + indent +
                lhs + b" = " + host + b" / " + literal + b";"
            )
            yield (
                f"dead-host-split {host.decode()} L{line}",
                splice(data, statement.start_byte, statement.end_byte,
                       replacement).decode(),
            )


def rule_shift_stage(text, name, span):
    """Split one constant right shift into every equivalent two-stage shift.

    ``x >> 8`` and ``(x >> 7) >> 1`` are equal under this compiler's signed
    and unsigned right-shift behavior, but the intermediate RTL can acquire a
    distinct allocation lifetime.  Restrict this to guided lines and do not
    resplit an already staged left operand.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for expr in _find(body, ("binary_expression",)):
        operator = expr.child_by_field_name("operator")
        left = expr.child_by_field_name("left")
        right = expr.child_by_field_name("right")
        if (operator is None or _txt(data, operator) != b">>" or
                left is None or right is None):
            continue
        bare_left = _unparen(left)
        if bare_left is not None and bare_left.type == "binary_expression":
            left_operator = bare_left.child_by_field_name("operator")
            if left_operator is not None and _txt(data, left_operator) == b">>":
                continue
        literal = _literal_text(data, right)
        try:
            total = int(literal, 0) if literal is not None else 0
        except ValueError:
            continue
        if total < 2 or total > 31:
            continue
        line = _line(data, expr.start_byte)
        if not _guided_site(line):
            continue
        source = _txt(data, left).strip()
        for first in range(1, total):
            second = total - first
            replacement = (b"((" + source + b" >> " + str(first).encode() +
                           b") >> " + str(second).encode() + b")")
            yield (
                f"shift-stage {first}+{second} L{line}",
                splice(data, expr.start_byte, expr.end_byte,
                       replacement).decode(),
            )


def rule_mul_affine_shape(text, name, span):
    """Toggle ``x*M+C`` with ``(x+C/M)*M`` when exactly divisible.

    Algebraically identical tails can be merged by jump2.  Reassociating only
    one arm keeps its RTL distinct and preserves a target's separate multiply
    tail without adding a runtime fence.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    def literal(node):
        token = _literal_text(data, node)
        if token is None:
            return None
        try:
            return int(token, 0)
        except ValueError:
            return None

    def add_text(value):
        return ((b" + " + str(value).encode()) if value >= 0 else
                (b" - " + str(-value).encode()))

    for expr in _find(body, ("binary_expression",)):
        operator = expr.child_by_field_name("operator")
        left = expr.child_by_field_name("left")
        right = expr.child_by_field_name("right")
        if operator is None or left is None or right is None:
            continue
        op = _txt(data, operator)
        replacement = tag = None
        if op in (b"+", b"-"):
            # Normalize to MULTIPLY + signed constant.
            multiply, constant_node, sign = left, right, 1 if op == b"+" else -1
            if multiply.type != "binary_expression" or literal(constant_node) is None:
                if op != b"+":
                    continue
                multiply, constant_node, sign = right, left, 1
            multiply = _unparen(multiply)
            if multiply is None or multiply.type != "binary_expression":
                continue
            mop = multiply.child_by_field_name("operator")
            ma = multiply.child_by_field_name("left")
            mb = multiply.child_by_field_name("right")
            if mop is None or _txt(data, mop) != b"*" or ma is None or mb is None:
                continue
            m = literal(mb)
            x = ma
            if m is None:
                m = literal(ma)
                x = mb
            c0 = literal(constant_node)
            c = sign * c0 if c0 is not None else None
            if m in (None, 0) or c is None or c % m != 0:
                continue
            q = c // m
            replacement = (b"((" + _txt(data, x).strip() + add_text(q) +
                           b") * " + str(m).encode() + b")")
            tag = f"fold {m},{c}"
        elif op == b"*":
            m = literal(right)
            grouped = _unparen(left)
            if m is None:
                m = literal(left)
                grouped = _unparen(right)
            if m is None or grouped is None or grouped.type != "binary_expression":
                continue
            gop = grouped.child_by_field_name("operator")
            ga = grouped.child_by_field_name("left")
            gb = grouped.child_by_field_name("right")
            if (gop is None or _txt(data, gop) not in (b"+", b"-") or
                    ga is None or gb is None or literal(gb) is None):
                continue
            q = literal(gb)
            if _txt(data, gop) == b"-":
                q = -q
            c = q * m
            replacement = (b"((" + _txt(data, ga).strip() + b" * " +
                           str(m).encode() + b")" + add_text(c) + b")")
            tag = f"expand {m},{c}"
        if replacement is None:
            continue
        line = _line(data, expr.start_byte)
        if not _guided_site(line):
            continue
        yield (
            f"mul-affine-shape {tag} L{line}",
            splice(data, expr.start_byte, expr.end_byte,
                   replacement).decode(),
        )


# extern object decl `extern T NAME;` (single line, not already an array/function).
# Optional leading qualifiers + a single type token + optional one `*` + NAME + `;`.
EXTERN_OBJ = re.compile(
    r"^[ \t]*extern[ \t]+"
    r"(?:(?:const|volatile|struct|union|unsigned|signed)[ \t]+)*"
    r"[A-Za-z_]\w*[ \t]+"
    r"\*?[ \t]*"
    r"([A-Za-z_]\w*)[ \t]*;[ \t]*$",
    re.M)


def _comment_mask(text):
    """Byte-per-char mask: True where `text[i]` is inside a /*..*/ or // comment.
    Keeps the extern-array rewrite from mangling identifiers named in comments."""
    mask = bytearray(len(text))
    i, n, state = 0, len(text), 0  # 0 code, 1 block, 2 line
    while i < n:
        two = text[i:i + 2]
        if state == 0:
            if two == "/*":
                mask[i] = mask[i + 1] = 1; state = 1; i += 2; continue
            if two == "//":
                mask[i] = mask[i + 1] = 1; state = 2; i += 2; continue
            i += 1
        elif state == 1:
            mask[i] = 1
            if two == "*/":
                mask[i + 1] = 1; state = 0; i += 2; continue
            i += 1
        else:
            if text[i] == "\n":
                state = 0; i += 1; continue
            mask[i] = 1; i += 1
    return mask


def rule_extern_array(text, name, span):
    """Respell a file-local `extern T NAME;` as an unknown-size array
    `extern T NAME[];` + rewrite every use `NAME` -> `NAME[0]`.

    THE recurring -G8 address lever (cookbook "gp vs absolute globals"): a
    <=8-byte extern is small-data-eligible, so cc1 forms its address as one
    `la`/macro; the unknown-size array is non-small, forcing the two-register
    HIGH/LO_SUM split whose `%hi` can then reorg-hoist. Closed DebugMenuItemSet,
    DoBriefingAndInventorySelection, ProcItemGun, ProcItemSmoke, ReqItemUse.

    Identity-preserving: same linker symbol at the same address, `NAME[0]` reads
    the same object — only the address codegen changes. Confined to decls that
    live IN THIS .c (never a shared header, whose flip would hit other TUs); if
    the decl isn't here the rule yields nothing for that symbol. A symbol whose
    target asm uses `%gp_rel` is excluded: the target has already proved that it
    must retain small-data addressing."""
    mask = _comment_mask(text)
    target_gp = _target_gp_symbols(name)
    for m in EXTERN_OBJ.finditer(text):
        if mask[m.start()]:                      # decl commented out
            continue
        sym = m.group(1)
        if sym in target_gp:
            continue
        decl_name = (m.start(1), m.end(1))
        occ = [mm.span() for mm in re.finditer(rf"\b{re.escape(sym)}\b", text)
               if not mask[mm.start()]]
        if len(occ) < 2:                         # need the decl + >=1 real use
            continue
        new = text
        for (s, e) in sorted(occ, reverse=True):  # right-to-left keeps offsets valid
            if (s, e) == decl_name:
                repl = f"{sym}[]"
            elif text[e:e + 1] == "[":            # already indexed; leave it
                continue
            else:
                repl = f"{sym}[0]"
            new = new[:s] + repl + new[e:]
        if new != text:
            yield (f"{sym}: extern scalar->unknown-array []", new)


# ---------------------------------------------------------------------------
# Guided/advanced rules.  These are runtime-value preserving pure-C rewrites,
# but deliberately opt-in because loop notes and CFG labels can perturb
# allocation far beyond their source line.  rtlguide narrows them to the lines
# owned by the residual; `--aggressive` is the explicit blind mode.
# ---------------------------------------------------------------------------


def rule_array_alias_remat(text, name, span):
    """Rebuild selected local-array member addresses instead of retaining an alias.

    ``p = &bank[i]; ... p->field = value;`` normally encourages old cc1 to
    retain ``p`` across the whole straight-line region.  Re-addressing just the
    member store through a typed byte offset can instead reproduce a target
    that emits another ``addiu reg,sp,K`` at that site.  This closed the final
    rank/character sprite pivot stores in mission_score_screen.

    Keep this guided and deliberately narrow: ``bank`` and ``p`` are
    unshadowed function-scope automatic locals with exactly matching typedef
    types, the index is an unshadowed local or parameter, the definition and
    use are in one straight-line compound block, neither pointer nor index has
    its address taken, and neither changes between the two statements.  The
    rewrite changes only the lvalue address spelling; scoring determines which
    target sites actually rematerialize it.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    arrays = {}
    pointers = {}
    shadowed = set()
    declaration_names = set()
    array_pattern = re.compile(
        rb"(?:(?:register|auto)\s+)?([A-Za-z_]\w*)\s+"
        rb"([A-Za-z_]\w*)\s*\[[^\],;]+\]\s*;"
    )
    pointer_pattern = re.compile(
        rb"(?:(?:register|auto)\s+)?([A-Za-z_]\w*)\s*\*\s*"
        rb"([A-Za-z_]\w*)\s*;"
    )
    for declaration in _find(body, ("declaration",)):
        names = set()
        for child in declaration.named_children:
            if child.type == "init_declarator":
                ident = _descend_ident(child.child_by_field_name("declarator"))
            elif child.type in ("identifier", "pointer_declarator",
                                "array_declarator"):
                ident = _descend_ident(child)
            else:
                ident = None
            if ident is not None:
                names.add(_txt(data, ident))
        declaration_names.update(names)
        if declaration.parent != body:
            shadowed.update(names)
            continue
        raw = _txt(data, declaration).strip()
        array = array_pattern.fullmatch(raw)
        pointer = pointer_pattern.fullmatch(raw)
        if array is not None:
            arrays[array.group(2)] = array.group(1)
        elif pointer is not None:
            pointers[pointer.group(2)] = pointer.group(1)

    safe_locals = _nonvolatile_function_local_names(data, body)
    arrays = {local: typ for local, typ in arrays.items()
              if local in safe_locals and local not in shadowed}
    pointers = {local: typ for local, typ in pointers.items()
                if local in safe_locals and local not in shadowed}
    if not arrays or not pointers:
        return

    # Parameters are safe index identities too, provided a nested declaration
    # does not shadow them.  Function-scope locals are already resolved above.
    parameters = set()
    function = body.parent
    declarator = (function.child_by_field_name("declarator")
                  if function is not None else None)
    if declarator is not None:
        for parameter in _find(declarator, ("parameter_declaration",)):
            if re.search(rb"\bvolatile\b", _txt(data, parameter)):
                continue
            ident = _descend_ident(parameter.child_by_field_name("declarator"))
            if ident is not None:
                parameters.add(_txt(data, ident))
    safe_indices = ((safe_locals - set(arrays) - set(pointers)) |
                    (parameters - declaration_names - shadowed))

    def address_taken(local):
        for pointer in _find(body, ("pointer_expression",)):
            operator = pointer.child_by_field_name("operator")
            argument = _unparen(pointer.child_by_field_name("argument"))
            if (operator is not None and _txt(data, operator) == b"&" and
                    argument is not None and argument.type == "identifier" and
                    _txt(data, argument) == local):
                return True
        return False

    def writes(statement, local):
        for assignment in _find(statement, ("assignment_expression",)):
            lhs = _unparen(assignment.child_by_field_name("left"))
            if (lhs is not None and lhs.type == "identifier" and
                    _txt(data, lhs) == local):
                return True
        for update in _find(statement, ("update_expression",)):
            argument = _unparen(update.child_by_field_name("argument"))
            if (argument is not None and argument.type == "identifier" and
                    _txt(data, argument) == local):
                return True
        return False

    def alias_definition(statement):
        assignment = _plain_assignment(data, statement)
        if assignment is None:
            return None
        alias, rhs = assignment
        rhs = _unparen(rhs)
        if alias not in pointers or rhs is None or rhs.type != "pointer_expression":
            return None
        operator = rhs.child_by_field_name("operator")
        subscript = _unparen(rhs.child_by_field_name("argument"))
        if (operator is None or _txt(data, operator) != b"&" or
                subscript is None or subscript.type != "subscript_expression"):
            return None
        bank = _unparen(subscript.child_by_field_name("argument"))
        index = _unparen(subscript.child_by_field_name("index"))
        if (bank is None or bank.type != "identifier" or
                index is None or index.type != "identifier"):
            return None
        bank_name = _txt(data, bank)
        index_name = _txt(data, index)
        if (bank_name not in arrays or arrays[bank_name] != pointers[alias] or
                index_name not in safe_indices):
            return None
        return alias, bank_name, index_name, pointers[alias]

    def field_store(statement, alias, index):
        if statement.type != "expression_statement":
            return None
        expressions = [child for child in statement.named_children
                       if child.type != "comment"]
        if len(expressions) != 1 or expressions[0].type != "assignment_expression":
            return None
        assignment = expressions[0]
        operator = assignment.child_by_field_name("operator")
        lhs = _unparen(assignment.child_by_field_name("left"))
        rhs = assignment.child_by_field_name("right")
        if (operator is None or _txt(data, operator) != b"=" or lhs is None or
                lhs.type != "field_expression" or rhs is None):
            return None
        match = re.fullmatch(
            re.escape(alias) + rb"->([A-Za-z_]\w*)", _txt(data, lhs).strip()
        )
        if match is None:
            return None
        if any(_txt(data, ident) == index
               for ident in _find(rhs, ("identifier",))):
            return None
        return lhs, match.group(1)

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for definition_index, definition_statement in enumerate(statements):
            definition = alias_definition(definition_statement)
            if definition is None:
                continue
            alias, bank, index, typ = definition
            if address_taken(alias) or address_taken(index):
                continue
            edits = []
            for statement_index in range(definition_index + 1, len(statements)):
                statement = statements[statement_index]
                # Do not reason across branches, labels, declarations, or
                # nested loops; the defining pointer may not dominate them.
                if statement.type != "expression_statement":
                    break
                if writes(statement, alias) or writes(statement, index):
                    break
                store = field_store(statement, alias, index)
                if store is None:
                    continue
                lhs, field = store
                line = _line(data, lhs.start_byte)
                if not _guided_site(line):
                    continue
                replacement = (
                    b"((" + typ + b" *)((u8 *)(" + bank + b") + (" + index +
                    b") * sizeof(" + typ + b")))->" + field
                )
                edits.append((statement_index, lhs.start_byte, lhs.end_byte,
                              replacement, field, line))
                yield (
                    f"array-alias-remat {alias.decode()}->{field.decode()} "
                    f"L{line}",
                    splice(data, lhs.start_byte, lhs.end_byte,
                           replacement).decode(),
                )

            # Two adjacent pivot/member stores commonly need the same source
            # identity change together.  Emit that bounded atomic candidate so
            # a neutral first edit does not hide an exact pair.
            for first, second in zip(edits, edits[1:]):
                if second[0] != first[0] + 1:
                    continue
                changed = data
                for edit in (second, first):
                    changed = splice(changed, edit[1], edit[2], edit[3])
                yield (
                    f"array-alias-remat {alias.decode()}->"
                    f"{first[4].decode()}/{second[4].decode()} "
                    f"L{first[5]}/{second[5]}",
                    changed.decode(),
                )


def _pure_dot_field_assignment(data, statement, locals_):
    """Return (base, lhs) for a pure `local.field = local-expr` statement."""
    if statement.type != "expression_statement":
        return None
    expressions = [c for c in statement.named_children if c.type != "comment"]
    if len(expressions) != 1 or expressions[0].type != "assignment_expression":
        return None
    assignment = expressions[0]
    operator = assignment.child_by_field_name("operator")
    lhs = assignment.child_by_field_name("left")
    rhs = assignment.child_by_field_name("right")
    if (operator is None or _txt(data, operator) != b"=" or
            lhs is None or lhs.type != "field_expression" or rhs is None or
            b"->" in _txt(data, lhs)):
        return None
    argument = lhs.child_by_field_name("argument")
    while argument is not None and argument.type == "field_expression":
        if b"->" in _txt(data, argument):
            return None
        argument = argument.child_by_field_name("argument")
    if argument is None or argument.type != "identifier":
        return None
    base = _txt(data, argument)
    if base not in locals_:
        return None
    forbidden = (
        "assignment_expression", "call_expression", "pointer_expression",
        "subscript_expression", "update_expression",
    )
    if (_find(rhs, forbidden) or
            any(b"->" in _txt(data, field)
                for field in _find(rhs, ("field_expression",)))):
        return None
    identifiers = {_txt(data, ident) for ident in _find(rhs, ("identifier",))}
    if base in identifiers or not identifiers.issubset(locals_):
        return None
    return base, _txt(data, lhs).strip()


def rule_redundant_field_donor(text, name, span):
    """Repeat a pure automatic-struct field assignment after nearby fields.

    cc1 normally removes the repeated store, but its pre-elimination references
    can change pseudo priorities and hard-register colouring.  Keep this narrow:
    the aggregate must be nonvolatile and never address-taken, the RHS may read
    only automatic locals through direct `.` fields, and the intervening one to
    three statements must initialise other fields of that same aggregate.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)
    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]
        for index, statement in enumerate(statements):
            original = _pure_dot_field_assignment(data, statement, locals_)
            if original is None:
                continue
            base, lhs = original
            if re.search(rb"&\s*" + re.escape(base) + rb"\b", body_text):
                continue
            source_line = _line(data, statement.start_byte)
            seen_fields = {lhs}
            for distance in range(1, 4):
                field_index = index + distance
                next_index = field_index + 1
                if next_index >= len(statements):
                    break
                following = _pure_dot_field_assignment(
                    data, statements[field_index], locals_)
                if following is None or following[0] != base or following[1] in seen_fields:
                    break
                seen_fields.add(following[1])
                next_statement = statements[next_index]
                destination_line = _line(data, next_statement.start_byte)
                if not (_guided_site(source_line) or
                        _guided_site(destination_line)):
                    continue
                indent = _indent_at(data, next_statement.start_byte)
                duplicate = _txt(data, statement).strip() + b"\n" + indent
                yield (
                    f"redundant-field-donor {lhs.decode()} "
                    f"L{source_line}->L{destination_line}",
                    splice(data, next_statement.start_byte,
                           next_statement.start_byte, duplicate).decode(),
                )


def _indent_at(data, off):
    start = data.rfind(b"\n", 0, off) + 1
    prefix = data[start:off]
    return prefix if not prefix.strip() else b""



def rule_loop_fence(text, name, span):
    """Wrap a statement in a one-shot do loop to create zero-code loop notes.

    cc1's loop notes are barriers to CSE/local-copy propagation and add weighted
    references for allocation.  Wrapping an existing loop is semantics-neutral;
    an if is eligible only when it contains no break/continue whose target the
    new wrapper could change.  ActATTACK's MotionUpdateMode scans are the worked
    example.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for stmt in _find(body, (
            "expression_statement", "if_statement", "for_statement",
            "while_statement")):
        if (stmt.type == "expression_statement" and
                (stmt.parent is None or stmt.parent.type != "compound_statement")):
            continue
        if stmt.parent is not None and stmt.parent.type == "do_statement":
            continue
        if stmt.type == "if_statement" and _find(
                stmt, ("break_statement", "continue_statement")):
            continue
        line = _line(data, stmt.start_byte)
        if not _guided_site(line):
            continue
        indent = _indent_at(data, stmt.start_byte)
        original = _txt(data, stmt)
        indented = original.replace(b"\n", b"\n  ")
        repl = (b"do {\n" + indent + b"  " + indented + b"\n" + indent +
                b"} while (0);")
        yield (f"loop-fence {stmt.type.removesuffix('_statement')} L{line}",
               splice(data, stmt.start_byte, stmt.end_byte, repl).decode())


def rule_nested_loop_fence(text, name, span):
    """Atomically add two or three zero-code loop weights at one safe site."""
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for stmt in _find(body, (
            "expression_statement", "if_statement", "for_statement",
            "while_statement")):
        if (stmt.type == "expression_statement" and
                (stmt.parent is None or stmt.parent.type != "compound_statement")):
            continue
        if stmt.type == "if_statement" and _find(
                stmt, ("break_statement", "continue_statement")):
            continue
        line = _line(data, stmt.start_byte)
        if not _guided_site(line):
            continue
        indent = _indent_at(data, stmt.start_byte)
        original = _txt(data, stmt)
        for depth in (2, 3):
            wrapped = original
            for _ in range(depth):
                wrapped = (b"do {\n" + indent + b"  " +
                           wrapped.replace(b"\n", b"\n  ") + b"\n" + indent +
                           b"} while (0);")
            yield (
                f"nested-loop-fence {depth} L{line}",
                splice(data, stmt.start_byte, stmt.end_byte, wrapped).decode(),
            )


def rule_paired_loop_fence(text, name, span):
    """Put adjacent statement groups behind two separate one-shot loop notes.

    A useful allocation can require raising two pseudo priorities together;
    either individual fence can cross the wrong priority boundary or leave a
    scheduler nop. Enumerate only two-to-four safe adjacent statements and
    every split into two nonempty groups, as one atomic guided candidate.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    eligible = {
        "compound_statement", "expression_statement", "for_statement",
        "goto_statement", "if_statement", "return_statement",
        "switch_statement", "while_statement",
    }
    forbidden = ("break_statement", "continue_statement", "labeled_statement",
                 "case_statement")
    for block in _find(body, ("compound_statement",)):
        if block.parent is not None and block.parent.type == "do_statement":
            continue
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for count in range(2, 5):
            for start in range(len(statements) - count + 1):
                group = statements[start:start + count]
                if (any(stmt.type not in eligible for stmt in group) or
                        any(_find(stmt, forbidden) for stmt in group)):
                    continue
                lines = [_line(data, stmt.start_byte) for stmt in group]
                if not any(_guided_site(line) for line in lines):
                    continue
                for split in range(1, count):
                    first_group, second_group = group[:split], group[split:]
                    first, last = group[0], group[-1]
                    indent = _indent_at(data, first.start_byte)

                    def fenced(part):
                        source = data[part[0].start_byte:part[-1].end_byte]
                        indented = source.replace(b"\n", b"\n  ")
                        return (b"do {\n" + indent + b"  " + indented + b"\n" +
                                indent + b"} while (0);")

                    middle = data[first_group[-1].end_byte:
                                  second_group[0].start_byte]
                    replacement = fenced(first_group) + middle + fenced(second_group)
                    yield (
                        f"paired-loop-fence {split}+{count - split} "
                        f"L{lines[0]}-{lines[-1]}",
                        splice(data, first.start_byte, last.end_byte,
                               replacement).decode(),
                    )


def rule_loop_range(text, name, span):
    """Wrap two-to-four adjacent statements in one zero-code `do` loop.

    A loop-note fence often needs to cover a producer, intervening tests, and
    consumer as one unit; fencing any single statement can be neutral or can
    worsen allocation. AttackIndirect required exactly three adjacent `if`s.
    Top-level declarations and any nested break/continue/label are excluded so
    the wrapper cannot change scope or control-flow targets.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    eligible = {
        "compound_statement", "expression_statement", "for_statement",
        "goto_statement", "if_statement", "return_statement",
        "switch_statement", "while_statement",
    }
    forbidden = ("break_statement", "continue_statement", "labeled_statement",
                 "case_statement")
    for block in _find(body, ("compound_statement",)):
        if block.parent is not None and block.parent.type == "do_statement":
            continue
        statements = [c for c in block.named_children if c.type != "comment"]
        for count in range(2, 5):
            for start in range(0, len(statements) - count + 1):
                group = statements[start:start + count]
                if any(stmt.type not in eligible for stmt in group):
                    continue
                if any(_find(stmt, forbidden) for stmt in group):
                    continue
                lines = [_line(data, stmt.start_byte) for stmt in group]
                if not any(_guided_site(line) for line in lines):
                    continue
                first, last = group[0], group[-1]
                indent = _indent_at(data, first.start_byte)
                original = data[first.start_byte:last.end_byte]
                indented = original.replace(b"\n", b"\n  ")
                repl = (b"do {\n" + indent + b"  " + indented + b"\n" +
                        indent + b"} while (0);")
                yield (
                    f"loop-range {count} statements L{lines[0]}-{lines[-1]}",
                    splice(data, first.start_byte, last.end_byte, repl).decode(),
                )


def _one_shot_loop(data, statement):
    """The compound body of `do { ... } while (0)`, or None."""
    if statement.type != "do_statement":
        return None
    condition = _unparen(statement.child_by_field_name("condition"))
    loop_body = statement.child_by_field_name("body")
    if (condition is None or condition.type != "number_literal" or
            _txt(data, condition).strip() not in (b"0", b"0x0") or
            loop_body is None or loop_body.type != "compound_statement"):
        return None
    return loop_body


def _empty_one_shot_loop(data, statement):
    """Whether `statement` is a standalone `do { } while (0)` fence."""
    loop_body = _one_shot_loop(data, statement)
    return (loop_body is not None and
            not [c for c in loop_body.named_children if c.type != "comment"])


def rule_empty_loop_boundary(text, name, span):
    """Insert or remove a weight-free LOOP_END scheduler barrier.

    Wrapping a producer in a one-shot loop also raises the allocation weight of
    every reference it contains.  A standalone empty loop creates the useful
    post-LOOP_END scheduling fence without reweighting either neighbouring
    statement.  It is diagnostic, not semantic source: once a real dependency
    repair lands, the fence can become the final mismatch.  Therefore enumerate
    removal of every guided empty fence as well as insertion at executable
    boundaries, and avoid stacking another directly beside an existing one.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    for loop in _find(body, ("do_statement",)):
        if not _empty_one_shot_loop(data, loop):
            continue
        line = _line(data, loop.start_byte)
        if not _guided_site(line):
            continue
        yield (
            f"empty-loop-boundary remove L{line}",
            splice(data, loop.start_byte, loop.end_byte, b"").decode(),
        )

    eligible = {
        "compound_statement", "do_statement", "expression_statement",
        "for_statement", "goto_statement", "if_statement",
        "return_statement", "switch_statement", "while_statement",
    }
    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]
        for before, after in zip(statements, statements[1:]):
            if before.type not in eligible or after.type not in eligible:
                continue
            if (_empty_one_shot_loop(data, before) or
                    _empty_one_shot_loop(data, after)):
                continue
            lines = (_line(data, before.start_byte), _line(data, after.start_byte))
            if not any(_guided_site(line) for line in lines):
                continue
            indent = _indent_at(data, after.start_byte)
            fence = (b"do {\n" + indent + b"} while (0);\n" + indent)
            yield (
                f"empty-loop-boundary L{lines[0]}-L{lines[1]}",
                splice(data, after.start_byte, after.start_byte, fence).decode(),
            )


def rule_fence_unwrap(text, name, span):
    """Remove an existing non-empty one-shot fence, keeping its body.

    An equivalence fence (``do { ... } while (0);``) is diagnostic scaffolding:
    it pins scheduling by making its body a separate block.  Once neighbouring
    repairs land, a stale fence can BE the final mismatch — it blocks cc1 from
    floating an independent statement (typically a call-argument copy) to the
    slot the target schedules it in, which reads as a whole exact-length block
    rotated around one instruction (mission_score_screen: an ~80-byte rotation
    collapsed to a 9-instruction shift when a stale fence was unwrapped,
    525 -> 498).  Enumerate unwrapping every non-empty fence; empty fences
    belong to rule_empty_loop_boundary's removal side.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for loop in _find(body, ("do_statement",)):
        loop_body = _one_shot_loop(data, loop)
        if loop_body is None or _empty_one_shot_loop(data, loop):
            continue
        inner = data[loop_body.start_byte + 1:loop_body.end_byte - 1].strip()
        if not inner:
            continue
        line = _line(data, loop.start_byte)
        yield (
            f"fence-unwrap L{line}",
            splice(data, loop.start_byte, loop.end_byte, inner).decode(),
        )


def rule_working_copy_seed_merge(text, name, span):
    """Move a working-copy identity from its seed to its update/writeback.

    A common decompiler spelling seeds a temporary and immediately publishes
    it, then later rebuilds that temporary and updates it in two statements::

        work = seed;
        persistent = work;
        ...                         /* no intervening use of work */
        do { work = persistent - K; } while (0);
        work += delta;
        persistent = work;
        consumer = work;

    Old CSE can coalesce the first pair into ``work; move persistent,work`` but
    fold the later update directly into ``persistent``, putting the one useful
    move at the wrong site.  Seed the persistent value directly and merge the
    rebuild/update instead.  ActSTICKON changed from 372 to 127 differing bytes
    with exactly this paired edit; either edit alone was one instruction short.

    Keep the transformation deliberately narrow.  ``work`` and ``persistent``
    must be unique, unaddressed, nonvolatile signed-32 locals of the same type;
    the rebuild is a pure scalar expression; the compound RHS cannot mention
    ``work``; and the seed-to-rebuild interval contains no use of ``work``.
    Calls remain allowed in ``delta`` because neither local is observable and
    the call is still evaluated exactly once at the same sequence point.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    locals_ = _nonvolatile_local_names(data, body)
    types = {}
    counts = {}
    for declaration in _find(body, ("declaration",)):
        raw = _txt(data, declaration)
        type_node = declaration.child_by_field_name("type")
        if (type_node is None or
                re.search(rb"\b(?:const|volatile|static|extern|register)\b", raw)):
            continue
        type_text = b" ".join(_txt(data, type_node).strip().split()).decode()
        if TYPES.get(type_text) != (32, True):
            continue
        for child in declaration.named_children:
            declarator = (child.child_by_field_name("declarator")
                          if child.type == "init_declarator" else child)
            if declarator is None or declarator.type != "identifier":
                continue
            ident = _txt(data, declarator)
            types[ident] = type_text
            counts[ident] = counts.get(ident, 0) + 1

    body_text = _txt(data, body)
    addressed = {
        token for token in types
        if re.search(rb"&\s*" + re.escape(token) + rb"\b", body_text)
    }

    def whole_assignment(statement):
        if statement.type != "expression_statement":
            return None
        expressions = [child for child in statement.named_children
                       if child.type != "comment"]
        if len(expressions) != 1 or expressions[0].type != "assignment_expression":
            return None
        assignment = expressions[0]
        operator = assignment.child_by_field_name("operator")
        lhs = assignment.child_by_field_name("left")
        rhs = assignment.child_by_field_name("right")
        if (operator is None or lhs is None or lhs.type != "identifier" or
                rhs is None):
            return None
        return _txt(data, lhs), _txt(data, operator), rhs

    def plain_copy(statement):
        parsed = whole_assignment(statement)
        if parsed is None or parsed[1] != b"=":
            return None
        lhs, _operator, rhs = parsed
        rhs = _unparen(rhs)
        if rhs is None or rhs.type != "identifier":
            return None
        return lhs, _txt(data, rhs)

    forbidden_rebuild = (
        "assignment_expression", "call_expression", "field_expression",
        "pointer_expression", "subscript_expression", "update_expression",
    )

    # Find the later rebuild/update/writeback/consumer chain first.  It may
    # start with either a direct assignment or the one-shot loop frequently
    # used to preserve the rand call's delay-slot schedule.
    chains = []
    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for index in range(len(statements) - 3):
            holder, update, writeback, consumer = statements[index:index + 4]
            rebuild_statement = holder
            loop_body = _one_shot_loop(data, holder)
            if loop_body is not None:
                loop_statements = [child for child in loop_body.named_children
                                   if child.type != "comment"]
                if len(loop_statements) != 1:
                    continue
                rebuild_statement = loop_statements[0]
            rebuild = whole_assignment(rebuild_statement)
            compound = whole_assignment(update)
            copied = plain_copy(writeback)
            consumed = plain_copy(consumer)
            if (rebuild is None or rebuild[1] != b"=" or compound is None or
                    compound[1] not in (b"+=", b"-=") or copied is None or
                    consumed is None):
                continue
            work, _equals, rebuild_rhs = rebuild
            if compound[0] != work or copied[1] != work:
                continue
            persistent = copied[0]
            if (consumed[1] != work or consumed[0] in (work, persistent) or
                    work == persistent):
                continue
            if (work not in locals_ or persistent not in locals_ or
                    types.get(work) != types.get(persistent) or
                    counts.get(work) != 1 or counts.get(persistent) != 1 or
                    work in addressed or persistent in addressed):
                continue
            if _find(rebuild_rhs, forbidden_rebuild):
                continue
            rebuild_identifiers = [_txt(data, ident)
                                   for ident in _find(rebuild_rhs, ("identifier",))]
            if persistent not in rebuild_identifiers or work in rebuild_identifiers:
                continue
            delta = compound[2]
            if any(_txt(data, ident) == work
                   for ident in _find(delta, ("identifier",))):
                continue
            if any(data[first.end_byte:second.start_byte].strip()
                   for first, second in zip(
                       (holder, update, writeback),
                       (update, writeback, consumer))):
                continue
            chains.append((holder, update, work, persistent, rebuild_rhs,
                           compound[1], delta))

    for holder, update, work, persistent, rebuild_rhs, compound_op, delta in chains:
        for block in _find(body, ("compound_statement",)):
            statements = [child for child in block.named_children
                          if child.type != "comment"]
            for seed_statement, publish_statement in zip(statements, statements[1:]):
                seed = whole_assignment(seed_statement)
                publish = plain_copy(publish_statement)
                if (seed is None or seed[1] != b"=" or seed[0] != work or
                        publish != (persistent, work) or
                        publish_statement.end_byte >= holder.start_byte or
                        data[seed_statement.end_byte:publish_statement.start_byte].strip()):
                    continue
                interval = data[publish_statement.end_byte:holder.start_byte]
                if re.search(rb"\b" + re.escape(work) + rb"\b", interval):
                    continue
                lines = (_line(data, holder.start_byte),
                         _line(data, update.start_byte))
                if not any(_guided_site(line) for line in lines):
                    continue

                seed_replacement = (
                    persistent + b" = " + _txt(data, seed[2]).strip() + b";"
                )
                binary_op = b"+" if compound_op == b"+=" else b"-"
                update_replacement = (
                    work + b" = " + _txt(data, rebuild_rhs).strip() + b" " +
                    binary_op + b" " + _txt(data, delta).strip() + b";"
                )
                rewritten = splice(data, holder.start_byte, update.end_byte,
                                   update_replacement)
                rewritten = splice(
                    rewritten, seed_statement.start_byte,
                    publish_statement.end_byte, seed_replacement,
                )
                yield (
                    f"working-copy-seed-merge {work.decode()}->{persistent.decode()} "
                    f"L{lines[0]}",
                    rewritten.decode(),
                )


def rule_deferred_global_capture(text, name, span):
    """Move a scalar-global capture behind a read-only decision tree.

    Decompilers often turn path-specific global reads into one cached local
    plus a reload in one arm::

        value = Global;
        if (value > 0) { ... }
        else { if (value < 0) { ... } value = Global; }
        use(value);

    When the decision tree cannot mutate ``Global``, the source-equivalent
    spelling can test ``Global`` directly and capture it once at the join.
    Think3escape needed that shape to recover the target's path-specific loads
    and post-join hard-register identity.

    Require a nonvolatile extern scalar, a nonvolatile unaddressed local, one
    exact reload in the tree, no calls/exits/updates/memory stores, and no other
    use of the local in the tree.  Those restrictions prove that moving the
    automatic assignment and deleting the redundant reload is unobservable.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    externs = set()
    extern_types = {}
    root = _TS.parse(data).root_node
    for declaration in _find(root, ("declaration",)):
        raw = _txt(data, declaration)
        if (re.search(rb"\bextern\b", raw) is None or
                re.search(rb"\bvolatile\b", raw) or
                _find(declaration, ("array_declarator", "function_declarator",
                                    "pointer_declarator"))):
            continue
        type_node = declaration.child_by_field_name("type")
        type_text = (b" ".join(_txt(data, type_node).strip().split()).decode()
                     if type_node is not None else "")
        for child in declaration.named_children:
            declarator = (child.child_by_field_name("declarator")
                          if child.type == "init_declarator" else child)
            if declarator is not None and declarator.type == "identifier":
                token = _txt(data, declarator)
                externs.add(token)
                extern_types[token] = TYPES.get(type_text)

    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)
    unsafe_control = (
        "break_statement", "continue_statement", "goto_statement",
        "labeled_statement", "return_statement",
    )

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for capture_statement, decision in zip(statements, statements[1:]):
            capture = _plain_assignment(data, capture_statement)
            if capture is None or decision.type != "if_statement":
                continue
            local, global_node = capture
            global_node = _unparen(global_node)
            if (local not in locals_ or global_node is None or
                    global_node.type != "identifier"):
                continue
            global_name = _txt(data, global_node)
            if (global_name not in externs or local == global_name or
                    re.search(rb"&\s*" + re.escape(local) + rb"\b", body_text) or
                    data[capture_statement.end_byte:decision.start_byte].strip()):
                continue
            if (_find(decision, ("call_expression", "update_expression",
                                *unsafe_control))):
                continue
            assignments_are_plain_locals = True
            for assignment in _find(decision, ("assignment_expression",)):
                operator = assignment.child_by_field_name("operator")
                lhs = assignment.child_by_field_name("left")
                if (operator is None or _txt(data, operator) != b"=" or
                        lhs is None or lhs.type != "identifier" or
                        _txt(data, lhs) not in locals_ or
                        assignment.parent is None or
                        assignment.parent.type != "expression_statement"):
                    assignments_are_plain_locals = False
                    break
            if not assignments_are_plain_locals:
                continue

            condition_local_ids = []
            for iff in _find(decision, ("if_statement",)):
                condition = iff.child_by_field_name("condition")
                if condition is None:
                    continue
                condition_local_ids.extend(
                    ident for ident in _find(condition, ("identifier",))
                    if _txt(data, ident) == local
                )
            if not condition_local_ids:
                continue

            reloads = []
            allowed_local_spans = {
                (ident.start_byte, ident.end_byte) for ident in condition_local_ids
            }
            stores_are_local = True
            for expression in _find(decision, ("expression_statement",)):
                assignment = _plain_assignment(data, expression)
                if assignment is None or assignment[0] not in locals_:
                    stores_are_local = False
                    break
                rhs = _unparen(assignment[1])
                if (assignment[0] == local and rhs is not None and
                        rhs.type == "identifier" and
                        _txt(data, rhs) == global_name):
                    reloads.append(expression)
                    allowed_local_spans.update(
                        (ident.start_byte, ident.end_byte)
                        for ident in _find(expression, ("identifier",))
                        if _txt(data, ident) == local
                    )
            if not stores_are_local or len(reloads) != 1:
                continue
            if any(
                    (ident.start_byte, ident.end_byte) not in allowed_local_spans
                    for ident in _find(decision, ("identifier",))
                    if _txt(data, ident) == local):
                continue

            line = _line(data, capture_statement.start_byte)
            if not (_guided_site(line) or
                    _guided_site(_line(data, decision.start_byte))):
                continue

            edits = [
                (reloads[0].start_byte - decision.start_byte,
                 reloads[0].end_byte - decision.start_byte, b""),
            ]
            edits.extend(
                (ident.start_byte - decision.start_byte,
                 ident.end_byte - decision.start_byte, global_name)
                for ident in condition_local_ids
            )
            decision_text = _txt(data, decision)
            for start, end, replacement in sorted(edits, reverse=True):
                decision_text = splice(decision_text, start, end, replacement)
            gap = data[capture_statement.end_byte:decision.start_byte]
            replacement = decision_text + gap + _txt(data, capture_statement)
            base_label = (
                f"deferred-global-capture {local.decode()}={global_name.decode()} "
                f"L{line}"
            )
            rewritten = splice(
                data, capture_statement.start_byte, decision.end_byte,
                replacement,
            )
            yield base_label, rewritten.decode()

            # An exact-length draft cannot enter a temporarily short beam
            # state.  Emit the common coordinated width repair atomically:
            # widen a later s16 local whose sole definition captures a signed
            # s16 extern.  The value set is unchanged, but old cc1's pseudo and
            # sign-extension shape is not.  Think3escape needed Degree's
            # ``degree2`` widened together with the deferred ``degree`` capture.
            for declaration in _find(body, ("declaration",)):
                if declaration.start_byte <= decision.end_byte:
                    continue
                type_node = declaration.child_by_field_name("type")
                if type_node is None:
                    continue
                old_type = b" ".join(_txt(data, type_node).strip().split()).decode()
                if TYPES.get(old_type) != (16, True):
                    continue
                declarators = []
                for child in declaration.named_children:
                    declarator = (child.child_by_field_name("declarator")
                                  if child.type == "init_declarator" else child)
                    if declarator is not None and declarator.type == "identifier":
                        declarators.append((declarator, child))
                if len(declarators) != 1 or declarators[0][1].type == "init_declarator":
                    continue
                widened = _txt(data, declarators[0][0])
                if (widened not in locals_ or widened in (local, global_name) or
                        re.search(rb"&\s*" + re.escape(widened) + rb"\b", body_text) or
                        any(_txt(data, ident) == widened
                            for update in _find(body, ("update_expression",))
                            for ident in _find(update, ("identifier",)))):
                    continue
                definitions = []
                for expression in _find(body, ("expression_statement",)):
                    assignment = _plain_assignment(data, expression)
                    if assignment is not None and assignment[0] == widened:
                        definitions.append(assignment[1])
                writes = []
                for assignment in _find(body, ("assignment_expression",)):
                    lhs = assignment.child_by_field_name("left")
                    if (lhs is not None and lhs.type == "identifier" and
                            _txt(data, lhs) == widened):
                        writes.append(assignment)
                if len(definitions) != 1 or len(writes) != 1:
                    continue
                source = _unparen(definitions[0])
                if (source is None or source.type != "identifier" or
                        extern_types.get(_txt(data, source)) != (16, True)):
                    continue
                typed = splice(data, type_node.start_byte, type_node.end_byte, b"s32")
                combined = splice(
                    typed, capture_statement.start_byte, decision.end_byte,
                    replacement,
                )
                yield (
                    base_label + f" + widen {widened.decode()}",
                    combined.decode(),
                )


def rule_loop_boundary_shift(text, name, span):
    """Move the following statement inside an existing one-shot loop.

    `do { A; } while (0); B;` and `do { A; B; } while (0);` execute the same
    statements once, but the latter moves NOTE_INSN_LOOP_END from before B to
    after it.  gcc-2.8.1 sched treats the first instruction after LOOP_END as a
    full register/memory fence.  This is a distinct lever from wrapping a new
    range.  Reject captured break/continue/labels and declarations so moving B
    cannot change its control-flow target or scope.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    forbidden = ("break_statement", "case_statement", "continue_statement",
                 "labeled_statement")
    eligible = {"compound_statement", "expression_statement", "if_statement",
                "return_statement", "switch_statement", "while_statement"}
    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]
        for loop, following in zip(statements, statements[1:]):
            loop_body = _one_shot_loop(data, loop)
            if loop_body is None or following.type not in eligible:
                continue
            if _find(loop_body, forbidden) or _find(following, forbidden):
                continue
            lines = (_line(data, loop.start_byte), _line(data, following.start_byte))
            if not any(_guided_site(line) for line in lines):
                continue
            close = loop_body.end_byte - 1
            close_line = data.rfind(b"\n", loop_body.start_byte, close) + 1
            indent = _indent_at(data, loop.start_byte)
            moved = _txt(data, following).strip().replace(b"\n", b"\n    ")
            inserted = indent + b"    " + moved + b"\n"
            changed_loop = (data[loop.start_byte:close_line] + inserted +
                            data[close_line:loop.end_byte])
            # Consume the whitespace between the loop and B, but retain the
            # indentation already preceding the loop's own start byte.
            repl = changed_loop
            yield (
                f"loop-boundary-shift L{lines[0]}<-L{lines[1]}",
                splice(data, loop.start_byte, following.end_byte, repl).decode(),
            )


def _guaranteed_defs(data, statement):
    """Plain locals assigned on every path through one statement."""
    direct = _plain_assignment(data, statement)
    if direct is not None:
        return [direct[0]]
    loop_body = _one_shot_loop(data, statement)
    if loop_body is None:
        return []
    out = []
    for child in [c for c in loop_body.named_children if c.type != "comment"]:
        out.extend(_guaranteed_defs(data, child))
    return out


def _pure_discriminator_condition(data, condition):
    """Whether evaluating a condition has no C-visible side effect.

    This is intentionally narrower than a general purity analysis.  It admits
    scalar/local/global field reads and ordinary comparisons/bit operations,
    but rejects calls, writes, indexing, division, and increment/decrement.
    The result is used only to exchange the discriminator of byte-identical
    arms, whose selected body is independent of the condition.
    """
    if condition is None or _find(condition, (
            "call_expression", "assignment_expression", "update_expression",
            "comma_expression", "subscript_expression", "sizeof_expression",
            "alignof_expression")):
        return False
    raw = _txt(data, condition)
    if (_find(condition, ("comment",)) or b"++" in raw or b"--" in raw or
            b"/" in raw or b"%" in raw):
        return False
    return bool(_find(condition, ("identifier",)))


def rule_identical_arm_condition(text, name, span):
    """Permute the condition of an existing byte-identical if/else fence.

    jump2 removes the branch and duplicate body, but the chosen discriminator
    still changes pre-jump2 liveness, pseudo conflicts, and allocation.  Reuse
    only side-effect-free conditions already present within 64 source lines;
    guided exact-byte scoring selects among these source-authentic probes.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    local_names = _nonvolatile_local_names(data, body)
    condition_sites = []
    for other in _find(body, ("if_statement",)):
        condition = other.child_by_field_name("condition")
        inner = (_paren_inner(condition)
                 if condition is not None and
                 condition.type == "parenthesized_expression"
                 else condition)
        if _pure_discriminator_condition(data, inner):
            condition_sites.append((_line(data, other.start_byte), inner))

    for iff in _find(body, ("if_statement",)):
        condition = iff.child_by_field_name("condition")
        yes = iff.child_by_field_name("consequence")
        no = iff.child_by_field_name("alternative")
        if condition is None or yes is None or no is None:
            continue
        if no.type == "else_clause":
            alternatives = [child for child in no.named_children
                            if child.type != "comment"]
            if len(alternatives) != 1:
                continue
            no = alternatives[0]
        if (yes.type != "compound_statement" or
                no.type != "compound_statement" or
                _txt(data, yes).strip() != _txt(data, no).strip()):
            continue
        current = (_paren_inner(condition)
                   if condition.type == "parenthesized_expression"
                   else condition)
        if not _pure_discriminator_condition(data, current):
            continue
        line = _line(data, iff.start_byte)
        end_line = _line(data, iff.end_byte)
        if not any(_guided_site(candidate_line)
                   for candidate_line in range(line - 1, end_line + 2)):
            continue
        current_text = _txt(data, current).strip()
        current_identifiers = {
            _txt(data, identifier)
            for identifier in _find(current, ("identifier",))
        }
        candidates = []
        seen = {current_text}
        for source_line, candidate in sorted(
                condition_sites, key=lambda item: (abs(item[0] - line), item[0])):
            candidate_text = _txt(data, candidate).strip()
            if (abs(source_line - line) > 64 or candidate_text in seen):
                continue
            candidate_identifiers = {
                _txt(data, identifier)
                for identifier in _find(candidate, ("identifier",))
            }
            # A nested automatic from the candidate's original scope may not
            # exist (or be initialized) at this fence. Globals, parameters,
            # and locals already used by the fence's original condition are
            # the bounded safe set.
            if ((candidate_identifiers & local_names) - current_identifiers):
                continue
            seen.add(candidate_text)
            candidates.append((source_line, candidate_text))
            if len(candidates) == 12:
                break
        for source_line, candidate_text in candidates:
            replacement = b"(" + candidate_text + b")"
            yield (
                f"identical-arm-condition L{source_line}->L{line}",
                splice(data, condition.start_byte, condition.end_byte,
                       replacement).decode(),
            )


def rule_identical_arm_fence(text, name, span):
    """Duplicate one expression statement into byte-identical if/else arms.

    jump optimization removes the branch, but the temporary CFG can fence
    sched/CSE without assigning loop-depth weight to the statement's operands.
    Conditions come only from nonvolatile locals either used by the statement
    already or unconditionally defined in the three preceding statements; no
    new uninitialised discriminator is invented.  For a 16-bit-returning
    function, also emit an atomic variant that widens a directly-returned
    function-scope 16-bit carrier.  Think3firstattack needed both edits at once:
    either edit alone changed length by two instructions, so an exact-length
    guided beam could not retain the intermediate state.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    body_text = _txt(data, body)

    # Bounded atomic companion for a narrow result carrier.  Keep only plain,
    # unshadowed function-scope integer locals returned directly by name, and
    # only when the function itself still performs the final 16-bit conversion.
    # This deliberately mirrors one useful type-width edge rather than taking a
    # Cartesian product with every local/type permutation.
    returned_names = set()
    for returned in _find(body, ("return_statement",)):
        values = [child for child in returned.named_children
                  if child.type != "comment"]
        value = _unparen(values[0]) if len(values) == 1 else None
        if value is not None and value.type == "identifier":
            returned_names.add(_txt(data, value))
    function = body.parent
    function_type = (function.child_by_field_name("type")
                     if function is not None and
                     function.type == "function_definition" else None)
    function_type_name = (_txt(data, function_type).decode().strip()
                          if function_type is not None else "")
    safe_function_locals = _nonvolatile_function_local_names(data, body)
    return_wideners = []
    if (function_type_name in TYPES and
            TYPES[function_type_name][0] == 16):
        for declaration in [child for child in body.named_children
                            if child.type == "declaration"]:
            type_node = declaration.child_by_field_name("type")
            declarator = declaration.child_by_field_name("declarator")
            ident = (_descend_ident(declarator)
                     if declarator is not None else None)
            type_name = (_txt(data, type_node).decode().strip()
                         if type_node is not None else "")
            local = _txt(data, ident) if ident is not None else None
            if (local is None or local not in returned_names or
                    local not in safe_function_locals or
                    declarator.type != "identifier" or
                    type_name not in TYPES or TYPES[type_name][0] != 16 or
                    re.search(rb"\b(?:const|volatile|static|register|extern)\b",
                              _txt(data, declaration)) or
                    re.search(rb"[,=*\[]", _txt(data, declaration)) or
                    re.search(rb"&\s*\(?\s*" + re.escape(local) + rb"\b",
                              body_text)):
                continue
            wider = CANON[(32, TYPES[type_name][1])].encode()
            return_wideners.append((local, type_node, wider))
    for block in _find(body, ("compound_statement",)):
        statements = [c for c in block.named_children if c.type != "comment"]
        for index, statement in enumerate(statements):
            if statement.type != "expression_statement":
                continue
            line = _line(data, statement.start_byte)
            if not _guided_site(line):
                continue
            assigned = _plain_assignment(data, statement)
            lhs = assigned[0] if assigned is not None else None
            candidates = []
            for ident in _find(statement, ("identifier",)):
                token = _txt(data, ident)
                if token in locals_ and token != lhs and token not in candidates:
                    candidates.append(token)
            for previous in reversed(statements[max(0, index - 3):index]):
                for token in reversed(_guaranteed_defs(data, previous)):
                    if token in locals_ and token != lhs and token not in candidates:
                        candidates.append(token)
            if not candidates:
                continue
            indent = _indent_at(data, statement.start_byte)
            original = _txt(data, statement).strip().replace(b"\n", b"\n    ")
            for discriminator in candidates[:4]:
                repl = (
                    b"if (" + discriminator + b" != 0)\n" + indent + b"{\n" +
                    indent + b"    " + original + b"\n" + indent + b"}\n" +
                    indent + b"else\n" + indent + b"{\n" + indent + b"    " +
                    original + b"\n" + indent + b"}"
                )
                yield (
                    f"identical-arm-fence {discriminator.decode()} L{line}",
                    splice(data, statement.start_byte, statement.end_byte,
                           repl).decode(),
                )
                for return_local, type_node, wider in return_wideners:
                    rewritten = data
                    edits = [
                        (statement.start_byte, statement.end_byte, repl),
                        (type_node.start_byte, type_node.end_byte, wider),
                    ]
                    for start, end, replacement in sorted(edits, reverse=True):
                        rewritten = splice(rewritten, start, end, replacement)
                    yield (
                        f"identical-arm-fence {discriminator.decode()} L{line} "
                        f"+ widen {return_local.decode()}",
                        rewritten.decode(),
                    )


def _nonvolatile_parameter_names(data, body):
    """Function parameters whose value is safe to read as a discriminator."""
    function = body.parent
    while function is not None and function.type != "function_definition":
        function = function.parent
    if function is None:
        return set()
    declarator = function.child_by_field_name("declarator")
    if declarator is None:
        return set()
    names = set()
    for parameter in _find(declarator, ("parameter_declaration",)):
        if re.search(rb"\bvolatile\b", _txt(data, parameter)):
            continue
        ident = _descend_ident(parameter.child_by_field_name("declarator"))
        if ident is not None:
            names.add(_txt(data, ident))
    return names


def _inside_conditional_arm(statement, body):
    """Whether a statement is already controlled by an if/else arm."""
    node = statement.parent
    while node is not None and node is not body:
        if node.type in ("if_statement", "else_clause"):
            return True
        node = node.parent
    return False


def _read_by_enclosing_if(data, statement, body, name):
    """Whether an enclosing arm has already evaluated local ``name``.

    Such a local is a safe identical-arm discriminator even when it is not a
    parameter: reaching either arm proves that its condition already read the
    value.  This is narrower than attempting general definite-assignment
    analysis and covers the allocation-donor shape used by Think1target.
    """
    def definitely_read(condition, ident):
        node = ident
        while node != condition:
            parent = node.parent
            if parent is None:
                return False
            if parent.type in ("sizeof_expression", "alignof_expression",
                               "conditional_expression"):
                return False
            if parent.type == "pointer_expression":
                operator = parent.child_by_field_name("operator")
                if operator is not None and _txt(data, operator) == b"&":
                    return False
            if parent.type == "assignment_expression":
                lhs = parent.child_by_field_name("left")
                if (lhs is not None and lhs.start_byte <= ident.start_byte and
                        ident.end_byte <= lhs.end_byte):
                    return False
            if parent.type == "binary_expression":
                operator = parent.child_by_field_name("operator")
                if operator is not None and _txt(data, operator) in (b"&&", b"||"):
                    return False
            node = parent
        return True

    node = statement.parent
    while node is not None and node != body:
        if node.type == "if_statement":
            condition = node.child_by_field_name("condition")
            if condition is not None and any(
                    _txt(data, ident) == name and
                    definitely_read(condition, ident)
                    for ident in _find(condition, ("identifier",))):
                return True
        node = node.parent
    return False


def rule_allocation_donor_fence(text, name, span):
    """Add one RTL reference to a misplaced value at an earlier CFG site.

    A final register-only residual can identify the right local but point at a
    source line too late to influence global allocation.  Duplicating an
    assignment into identical arms makes the parameter discriminator take part
    in the temporary CFG; jump/cross-jump then remove both the test and the
    duplicate store.  FUN_80033bc0 needed precisely this extra weighted `pos`
    reference to exchange its $s5/$s6 homes.

    This guided-only variant is intentionally narrow.  Donors are nonvolatile
    values named by rtlguide's register substitutions.
    Function parameters are initialized by definition.  An automatic local is
    accepted only when an enclosing if condition already reads it, which proves
    the value is available on both arms without requiring broad data-flow
    guesses.  Sites must be standalone assignments already inside a conditional
    arm, so the rule searches useful off-residual sites without flooding the
    normal rule sweep.
    """
    if not GUIDED_VARIABLES:
        return
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    parameters = _nonvolatile_parameter_names(data, body)
    locals_ = _nonvolatile_local_names(data, body)
    donors = sorted((parameters | locals_) & GUIDED_VARIABLES)
    if not donors:
        return
    for statement in _find(body, ("expression_statement",)):
        expressions = [c for c in statement.named_children if c.type != "comment"]
        if (len(expressions) != 1 or
                expressions[0].type != "assignment_expression" or
                not _inside_conditional_arm(statement, body)):
            continue
        operator = expressions[0].child_by_field_name("operator")
        if operator is None or _txt(data, operator) != b"=":
            continue
        line = _line(data, statement.start_byte)
        indent = _indent_at(data, statement.start_byte)
        original = _txt(data, statement).strip().replace(b"\n", b"\n    ")
        safe_donors = [donor for donor in donors
                       if donor in parameters or
                       _read_by_enclosing_if(data, statement, body, donor)]
        for donor in safe_donors[:4]:
            repl = (
                b"if (" + donor + b" != 0)\n" + indent + b"{\n" +
                indent + b"    " + original + b"\n" + indent + b"}\n" +
                indent + b"else\n" + indent + b"{\n" + indent + b"    " +
                original + b"\n" + indent + b"}"
            )
            yield (
                f"allocation-donor-fence {donor.decode()} L{line}",
                splice(data, statement.start_byte, statement.end_byte,
                       repl).decode(),
            )


def rule_case_fence(text, name, span):
    """Respell a two-arm if as a two-way switch with a real case label.

    gcc-2.8.1's find_cross_jump stops at a referenced case CODE_LABEL.  This is
    the StateTransition cross-jump fence, now enumerable for if/else sites.  A
    source `break` inside either arm would change meaning under a switch, so
    those sites are excluded.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for iff in _find(body, ("if_statement",)):
        cond = iff.child_by_field_name("condition")
        yes = iff.child_by_field_name("consequence")
        no = iff.child_by_field_name("alternative")
        if cond is None or yes is None or no is None:
            continue
        if no.type == "else_clause":
            arms = [c for c in no.named_children if c.type != "comment"]
            if len(arms) != 1:
                continue
            no = arms[0]
        if _find(yes, ("break_statement",)) or _find(no, ("break_statement",)):
            continue
        line = _line(data, iff.start_byte)
        if not _guided_site(line):
            continue
        c = _paren_inner(cond) if cond.type == "parenthesized_expression" else cond
        if c is None:
            continue
        ct, yt, nt = _txt(data, c), _txt(data, yes), _txt(data, no)
        variants = (
            (b"case 0", nt, b"default", yt, "case0"),
            (b"case 1", yt, b"default", nt, "case1"),
        )
        for lab1, arm1, lab2, arm2, tag in variants:
            repl = (b"switch (!!(" + ct + b")) {\n  " + lab1 + b": { " + arm1 +
                    b" } break;\n  " + lab2 + b": { " + arm2 + b" } break;\n}")
            yield (f"case-fence {tag} L{line}",
                   splice(data, iff.start_byte, iff.end_byte, repl).decode())


def rule_sparse_eq_switch(text, name, span):
    """Turn a three-arm literal equality ladder into a sparse switch.

    Old GCC's ``expand_case`` sorts sparse values into a comparison tree that
    a chain of ``if`` statements cannot reproduce.  Restrict the transform to
    a nonvolatile local identifier compared with three integer literals; this
    makes evaluate-once switch semantics identical.  Enumerate case source
    order because it controls body placement independently of the sorted test
    tree.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    volatile_locals = set()
    for decl in _find(body, ("declaration",)):
        if b"volatile" not in _txt(data, decl):
            continue
        for ident in _find(decl, ("identifier",)):
            volatile_locals.add(_txt(data, ident))

    for top in _find(body, ("if_statement",)):
        if top.parent is not None and top.parent.type == "else_clause":
            continue
        arms = []
        cursor = top
        tail = None
        while cursor is not None and cursor.type == "if_statement":
            condition = _unparen(cursor.child_by_field_name("condition"))
            consequence = cursor.child_by_field_name("consequence")
            if (condition is None or condition.type != "binary_expression" or
                    consequence is None):
                arms = []
                break
            operator = condition.child_by_field_name("operator")
            left = condition.child_by_field_name("left")
            right = condition.child_by_field_name("right")
            if (operator is None or _txt(data, operator) != b"==" or
                    left is None or right is None):
                arms = []
                break
            if left.type == "identifier" and right.type == "number_literal":
                variable, literal = _txt(data, left), _txt(data, right)
            elif right.type == "identifier" and left.type == "number_literal":
                variable, literal = _txt(data, right), _txt(data, left)
            else:
                arms = []
                break
            if (_find(consequence, ("break_statement", "case_statement")) or
                    variable in volatile_locals):
                arms = []
                break
            arms.append((variable, literal, _txt(data, consequence)))
            alternative = cursor.child_by_field_name("alternative")
            if alternative is None:
                tail = None
                break
            if alternative.type == "else_clause":
                children = [child for child in alternative.named_children
                            if child.type != "comment"]
                if len(children) != 1:
                    arms = []
                    break
                alternative = children[0]
            if alternative.type == "if_statement":
                cursor = alternative
            else:
                tail = alternative
                cursor = None
        if len(arms) != 3 or len({arm[0] for arm in arms}) != 1:
            continue
        if len({arm[1] for arm in arms}) != 3:
            continue
        if tail is not None and _find(tail, ("break_statement", "case_statement")):
            continue
        line = _line(data, top.start_byte)
        if not _guided_site(line):
            continue
        variable = arms[0][0]
        indent = _indent_at(data, top.start_byte)
        for order in itertools.permutations(arms):
            pieces = [b"switch (" + variable + b")\n" + indent + b"{"]
            for _var, literal, consequence in order:
                pieces.append(indent + b"case " + literal + b":\n" + indent +
                              consequence + b"\n" + indent + b"    break;")
            if tail is not None:
                pieces.append(indent + b"default:\n" + indent + _txt(data, tail) +
                              b"\n" + indent + b"    break;")
            pieces.append(indent + b"}")
            tag = ",".join(literal.decode() for _v, literal, _c in order)
            yield (
                f"sparse-eq-switch {tag} L{line}",
                splice(data, top.start_byte, top.end_byte,
                       b"\n".join(pieces)).decode(),
            )


def rule_if_else_invert(text, name, span):
    """Invert a two-compound-arm if/else without changing its behavior.

    cc1 lays the source consequence and alternative out asymmetrically.  The
    exact same control flow with the other body first can therefore require
    ``if (!(condition)) { no } else { yes }``.  Restrict this guided rule to
    compound arms so it cannot create a dangling-else ambiguity or disturb
    statement ownership; the condition is still evaluated exactly once.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for iff in _find(body, ("if_statement",)):
        cond = iff.child_by_field_name("condition")
        yes = iff.child_by_field_name("consequence")
        no = iff.child_by_field_name("alternative")
        if cond is None or yes is None or no is None:
            continue
        if no.type == "else_clause":
            arms = [child for child in no.named_children if child.type != "comment"]
            if len(arms) != 1:
                continue
            no = arms[0]
        if yes.type != "compound_statement" or no.type != "compound_statement":
            continue
        line = _line(data, iff.start_byte)
        if not _guided_site(line):
            continue
        inner = _paren_inner(cond) if cond.type == "parenthesized_expression" else cond
        if inner is None:
            continue
        repl = (b"if (!(" + _txt(data, inner).strip() + b")) " +
                _txt(data, no) + b" else " + _txt(data, yes))
        yield (f"if-else-invert L{line}",
               splice(data, iff.start_byte, iff.end_byte, repl).decode())


def rule_terminal_arm_flip(text, name, span):
    """Swap two adjacent terminal arms by negating their guard.

    A decompiler commonly renders a two-way terminal dispatch without an
    explicit ``else``::

        if (condition) { arm_a; goto tail; }
        { arm_b; goto tail; }

    The equivalent ``if (!condition) { arm_b; goto tail; } { arm_a; goto
    tail; }`` exposes the opposite physical fallthrough arm to jump2 while
    retaining both branch-local scopes and call sites.  The second arm may be
    a compound or a short sequence of plain expressions followed by its exit;
    the latter is wrapped in a new block. Keep this guided rule deliberately
    narrow: both arms must end in returns, or in gotos to the same label, and
    no comment may sit between them.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    def terminal(block):
        children = [child for child in block.named_children
                    if child.type != "comment"]
        if not children:
            return None
        last = children[-1]
        if last.type == "return_statement":
            return "return", None
        if last.type != "goto_statement":
            return None
        match = re.fullmatch(
            rb"\s*goto\s+([A-Za-z_]\w*)\s*;\s*", _txt(data, last)
        )
        return ("goto", match.group(1)) if match else None

    for block in _find(body, ("compound_statement", "case_statement")):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for index, iff in enumerate(statements[:-1]):
            if (iff.type != "if_statement" or
                    iff.child_by_field_name("alternative") is not None):
                continue
            consequence = iff.child_by_field_name("consequence")
            condition = iff.child_by_field_name("condition")
            if (consequence is None or consequence.type != "compound_statement" or
                    condition is None or _find(condition, ("comment",))):
                continue
            first_terminal = terminal(consequence)
            following = statements[index + 1]
            following_end = following
            if following.type == "compound_statement":
                second_terminal = terminal(following)
                following_text = _txt(data, following)
            else:
                arm_statements = []
                second_terminal = None
                for candidate in statements[index + 1:index + 5]:
                    if candidate.type not in {
                            "expression_statement", "goto_statement",
                            "return_statement"}:
                        break
                    if (arm_statements and
                            data[arm_statements[-1].end_byte:
                                 candidate.start_byte].strip()):
                        break
                    arm_statements.append(candidate)
                    if candidate.type in {"goto_statement", "return_statement"}:
                        following_end = candidate
                        if candidate.type == "return_statement":
                            second_terminal = "return", None
                        else:
                            match = re.fullmatch(
                                rb"\s*goto\s+([A-Za-z_]\w*)\s*;\s*",
                                _txt(data, candidate),
                            )
                            second_terminal = (("goto", match.group(1))
                                               if match else None)
                        break
                if second_terminal is None:
                    continue
                indent = _indent_at(data, iff.start_byte)
                rendered = (b"\n" + indent + b"    ").join(
                    _txt(data, statement).strip()
                    for statement in arm_statements
                )
                following_text = (
                    b"{\n" + indent + b"    " + rendered +
                    b"\n" + indent + b"}"
                )
            if (first_terminal is None or first_terminal != second_terminal or
                    data[iff.end_byte:following.start_byte].strip()):
                continue
            line = _line(data, iff.start_byte)
            following_line = _line(data, following.start_byte)
            if not (_guided_site(line) or _guided_site(following_line)):
                continue
            inner = (_paren_inner(condition)
                     if condition.type == "parenthesized_expression"
                     else condition)
            if inner is None:
                continue
            between = data[iff.end_byte:following.start_byte]
            replacement = (
                b"if (!(" + _txt(data, inner).strip() + b")) " +
                following_text + between + _txt(data, consequence)
            )
            yield (
                f"terminal-arm-flip L{line}",
                splice(data, iff.start_byte, following_end.end_byte,
                       replacement).decode(),
            )


def rule_terminal_guard_flip(text, name, span):
    """Flip an adjacent equality guard between ``return`` and ``goto``.

    These two source layouts have identical C control flow but expose opposite
    physical fallthrough arms to jump2/reorg::

        if (x != K) return;     if (x == K) goto body;
        goto body;              return;

    The equality operands are evaluated exactly once in either spelling, so
    they may contain ordinary side effects; only ``==``/``!=`` is changed.
    Keep the transformation deliberately local: the if has no else, its sole
    consequence and the immediately following statement are one return and
    one goto, and comments cannot be crossed or discarded.  Guided scoring
    decides which polarity has the target's terminal block layout.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return

    def one_statement(node):
        if node is None:
            return None
        if node.type != "compound_statement":
            return node
        children = [child for child in node.named_children
                    if child.type != "comment"]
        return children[0] if len(children) == 1 else None

    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for iff, following in zip(statements, statements[1:]):
            if (iff.type != "if_statement" or
                    iff.child_by_field_name("alternative") is not None):
                continue
            consequence_node = iff.child_by_field_name("consequence")
            consequence = one_statement(consequence_node)
            if consequence is None or {consequence.type, following.type} != {
                    "return_statement", "goto_statement"}:
                continue
            if (_find(consequence_node, ("comment",)) or
                    _find(following, ("comment",)) or
                    data[iff.end_byte:following.start_byte].strip()):
                continue

            condition_node = iff.child_by_field_name("condition")
            condition = _unparen(condition_node)
            if (condition is None or condition.type != "binary_expression" or
                    _find(condition_node, ("comment",))):
                continue
            operator = condition.child_by_field_name("operator")
            if operator is None:
                continue
            current = _txt(data, operator)
            flipped_operator = {b"==": b"!=", b"!=": b"=="}.get(current)
            if flipped_operator is None:
                continue

            line = _line(data, iff.start_byte)
            following_line = _line(data, following.start_byte)
            if not (_guided_site(line) or _guided_site(following_line)):
                continue

            raw_condition = _txt(data, condition)
            relative_start = operator.start_byte - condition.start_byte
            relative_end = operator.end_byte - condition.start_byte
            flipped_condition = (raw_condition[:relative_start] +
                                 flipped_operator +
                                 raw_condition[relative_end:])
            indent = _indent_at(data, iff.start_byte)
            rendered_if = (
                b"if (" + flipped_condition + b")\n" + indent + b"{\n" +
                indent + b"    " + _txt(data, following).strip() + b"\n" +
                indent + b"}"
            )
            between = data[iff.end_byte:following.start_byte]
            replacement = (rendered_if + between +
                           _txt(data, consequence).strip())
            direction = ("return-to-goto" if consequence.type == "return_statement"
                         else "goto-to-return")
            yield (
                f"terminal-guard-flip {direction} L{line}",
                splice(data, iff.start_byte, following.end_byte,
                       replacement).decode(),
            )


def _literal_field_store(data, statement):
    """Return (base, field) for ``base.field = integer_literal;``."""
    if statement.type != "expression_statement":
        return None
    expressions = [child for child in statement.named_children
                   if child.type != "comment"]
    if len(expressions) != 1 or expressions[0].type != "assignment_expression":
        return None
    assignment = expressions[0]
    operator = assignment.child_by_field_name("operator")
    lhs = assignment.child_by_field_name("left")
    rhs = _unparen(assignment.child_by_field_name("right"))
    if (operator is None or _txt(data, operator) != b"=" or lhs is None or
            lhs.type != "field_expression" or rhs is None or
            rhs.type != "number_literal"):
        return None
    base = lhs.child_by_field_name("argument")
    field = lhs.child_by_field_name("field")
    if base is None or field is None:
        return None
    return _txt(data, base).strip(), _txt(data, field).strip()


def rule_adjacent_field_store_swap(text, name, span):
    """Swap adjacent literal stores to distinct fields of the same base.

    With no intervening observation, volatile qualifier, aliasing RHS, or
    overlapping field, the final C state is identical. Source order still
    changes sched2's priority and can flip an unrelated late register/order
    tie. Literal-only RHSes make that proof deliberately narrow.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    for block in _find(body, ("compound_statement",)):
        statements = [child for child in block.named_children
                      if child.type != "comment"]
        for first, second in zip(statements, statements[1:]):
            left = _literal_field_store(data, first)
            right = _literal_field_store(data, second)
            if (left is None or right is None or left[0] != right[0] or
                    left[1] == right[1]):
                continue
            between = data[first.end_byte:second.start_byte]
            if between.strip():
                continue
            line = _line(data, first.start_byte)
            if not (_guided_site(line) or
                    _guided_site(_line(data, second.start_byte))):
                continue
            replacement = _txt(data, second) + between + _txt(data, first)
            yield (
                f"adjacent-field-store-swap {left[1].decode()}/{right[1].decode()} L{line}",
                splice(data, first.start_byte, second.end_byte, replacement).decode(),
            )


RULES = [
    ("type-width", "flip a local's integer type across width/signedness", rule_type_width),
    ("param-width", "flip a scalar parameter's integer type across width/signedness", rule_param_width),
    ("stack-decl-swap", "swap adjacent same-type address-taken object declarations", rule_stack_decl_swap),
    ("extern-array", "extern T NAME; -> extern T NAME[]; + NAME->NAME[0] (-G8 split)", rule_extern_array),
    ("and-nest", "split/merge if(a && b) <-> nested ifs (no else)", rule_and_nest),
    ("temp-inline", "inline a single-use local temp into its use", rule_temp_inline),
    ("copy-seed", "x = E(y); -> x = y; x = E(x); (local_alloc quantity-order lever)", rule_copy_seed),
    ("binop-operand-seed", "x = A op B; -> t = B; x = A op t; (operand-birth order)", rule_binop_operand_seed),
    ("call-result-split", "split a reused direct-call result into single-definition locals", rule_call_result_split),
    ("late-pointer-direct", "inline a repeated pointer-global assignment in its late region", rule_late_pointer_direct),
    ("literal-indirect-inline", "atomically inline a byte literal and indirect-call field temps", rule_literal_indirect_inline),
    ("call-arg-pair", "inline adjacent same-call temps into one consumer call", rule_call_arg_pair_inline),
    ("abs-ge", "rewrite (x<0)?-x:x -> (x>=0)?x:-x (the abssi2 fold)", rule_abs_ge),
    ("builtin-abs", "abs call/sign-fix -> __builtin_abs under cc1 -fno-builtin", rule_builtin_abs),
    ("or-inplace", "rewrite X = X|C -> X |= C (local-alloc lever)", rule_or_inplace),
    ("rand-mod", "split/merge x=rand()%K (call-result allocation lever)", rule_rand_mod_split),
    ("mod-bias-temp", "split base+(delta%range-bias) through a typed temp", rule_mod_bias_temp),
    ("subscript-postinc", "split/merge a[i];i++ <-> a[i++] (working-copy lever)", rule_subscript_postinc),
    ("ptr-index-sum", "T *p = base+idx -> (T*)(idx*sizeof(T)+(int)base)", rule_ptr_index_sum),
    ("vector-copy", "four vx/vy/vz/pad field copies -> one struct assignment", rule_vector_copy),
    ("assignment-chain", "merge/split same-literal field stores", rule_assignment_chain),
    ("vector-copy-adjust", "split/merge a literal-adjusted vx/vy/vz field copy", rule_vector_copy_adjust),
    ("split-chain", "t = (x-C)>>S -> t = x-C; t = t>>S (split fused chain)", rule_split_chain),
    ("min-ternary", "x=a; if(b<x) x=b; -> x = (b<a)?b:a (min-vs-memory)", rule_min_ternary),
    ("clamp-shared-return", "two direct clamp returns -> assignments plus one return", rule_clamp_shared_return),
    ("flag-return-split", "local 0/1 default+override -> literal return sites", rule_flag_return_split),
    ("cmp-swap", "a>mem -> mem<a (comparison operand-order lever)", rule_cmp_swap),
    # In the DEFAULT set deliberately: fences are our own scaffolding, their
    # candidate count is tiny (one per existing fence), and their byte effect
    # can be NON-LOCAL (a prologue-region fence gates spill/init scheduling
    # ~100 lines away — FUN_800519bc closed 25 bytes; mission_score_screen 27),
    # so guided line-adjacency must never be the only path to trying removal.
    ("fence-unwrap", "remove an existing non-empty one-shot fence, keeping its body", rule_fence_unwrap),
]

AGGRESSIVE_RULES = [
    ("allocation-donor-fence", "add a guided initialized-value ref in zero-code identical arms", rule_allocation_donor_fence),
    ("field-capture-rhs", "fuse/split an adjacent saved field through the overwrite RHS", rule_field_capture_rhs),
    ("initialized-global-compound", "split a commutative global sum into accumulator initialization plus +=", rule_initialized_global_compound),
    ("cmp-polarity", "swap two local comparison operands (regalloc ref-order lever)", rule_cmp_polarity),
    ("eq-literal-swap", "swap ==/!= literal operand order (v0/v1 lever)", rule_eq_literal_swap),
    ("shift16-mul", "casted short x<<16 -> x*0x10000 (raw sll lever)", rule_shift16_mul),
    ("shift-stage", "split x>>N into every two-stage constant shift", rule_shift_stage),
    ("dead-host-split", "split signed (A+/-B)/K through a dead signed-s32 local", rule_dead_host_split),
    ("mul-affine-shape", "toggle x*M+C with (x+C/M)*M", rule_mul_affine_shape),
    ("ptr-base-split", "name an extern array base before indexed address/call", rule_ptr_base_split),
    ("deref-address-split", "name a casted dereference address before its scalar load", rule_deref_address_split),
    ("working-copy-seed-merge", "move a working-copy identity from seed to update/writeback", rule_working_copy_seed_merge),
    ("deferred-global-capture", "test a scalar global directly, then capture once at the join", rule_deferred_global_capture),
    ("plus-group", "enumerate 3-term + grouping/constant placement (fold lever)", rule_plus_group),
    ("add-prefix-temp", "name a signed 2-term seam in a narrowed 3-term sum", rule_add_prefix_temp),
    ("flag-arm-assign", "move local 0/1 definitions between pre-zero and two-arm forms", rule_flag_arm_assign),
    ("default-ladder-hoist", "hoist a three-way literal result default before both comparisons", rule_default_ladder_hoist),
    ("guard-flag-assign", "move a local flag definition before a guard or onto both goto edges", rule_guard_flag_assign),
    ("guard-exit-copy", "move a local value copy across its unique loop-break guard", rule_guard_exit_copy),
    ("shared-tail-assign", "duplicate one shared assignment into both preceding arms", rule_shared_tail_assign),
    ("shared-writeback-compound", "inline a shared field writeback into compound-update arms", rule_shared_writeback_compound),
    ("shared-terminal-tail", "duplicate an adjacent assignment+return into both if/else arms", rule_shared_terminal_tail),
    ("shared-result-return", "funnel two value returns through one result pseudo", rule_shared_result_return),
    ("terminal-call-return", "return a terminal branch call before the shared result join", rule_terminal_call_return),
    ("difference-role-fuse", "fuse scratch-loaded subtractions into direct difference locals", rule_difference_role_fuse),
    ("shared-return-split", "replace a goto to a return label with a second literal return", rule_shared_return_split),
    ("terminal-arm-flip", "negate and swap adjacent arms ending at one terminal tail", rule_terminal_arm_flip),
    ("if-else-invert", "invert a compound if/else to swap physical body layout", rule_if_else_invert),
    ("terminal-guard-flip", "flip an adjacent ==/!= return/goto terminal guard", rule_terminal_guard_flip),
    ("adjacent-field-store-swap", "swap adjacent literal stores to distinct fields", rule_adjacent_field_store_swap),
    ("switch-cse-evict", "dead-overwrite an entry index before a fresh switch load", rule_switch_cse_evict),
    ("pointee-volatile", "toggle volatile on a local integer pointer's pointee", rule_pointee_volatile),
    ("pointer-slot-volatile", "force one extern pointer-array slot reload", rule_pointer_slot_volatile),
    ("array-alias-remat", "rebuild selected local-array member lvalues instead of retaining an alias", rule_array_alias_remat),
    ("member-scalar-alias", "toggle long field read through s32 lvalue (MEM_IN_STRUCT lever)", rule_member_scalar_alias),
    ("disjoint-local-alias", "join a dead-until-overwrite scalar to an earlier live range", rule_disjoint_local_alias),
    ("redundant-field-donor", "repeat a pure local-aggregate field assignment", rule_redundant_field_donor),
    ("empty-loop-boundary", "insert a weight-free LOOP_END between statements", rule_empty_loop_boundary),
    ("loop-fence", "wrap an if/loop in a zero-code one-shot do loop", rule_loop_fence),
    ("nested-loop-fence", "atomically add two or three loop weights at one site", rule_nested_loop_fence),
    ("paired-loop-fence", "wrap adjacent groups in two atomic one-shot loops", rule_paired_loop_fence),
    ("loop-range", "wrap 2-4 adjacent statements in one zero-code do loop", rule_loop_range),
    ("loop-boundary-shift", "move the next statement across an existing LOOP_END", rule_loop_boundary_shift),
    ("identical-arm-condition", "permute the pure discriminator of existing identical arms", rule_identical_arm_condition),
    ("identical-arm-fence", "duplicate one statement into zero-code identical arms", rule_identical_arm_fence),
    ("case-fence", "if/else -> two-way switch (hard cross-jump CODE_LABEL)", rule_case_fence),
    ("sparse-eq-switch", "three literal equality arms -> sparse switch permutations", rule_sparse_eq_switch),
]

LINE_RULES = {rule_type_width, rule_extern_array, rule_pointee_volatile,
              rule_pointer_slot_volatile}
AST_RULES = {gen for _key, _desc, gen in RULES + AGGRESSIVE_RULES
             if gen not in LINE_RULES}


def select_rules(requested=(), guided=None, aggressive=False):
    """Return rules in the caller's priority order.

    ``rtlguide`` orders its suggestions by the residual's dominant compiler
    pass.  Guided beam search has a finite candidate-build budget, so converting
    that list to a set and then restoring registry order can spend the entire
    budget on generic rules before reaching the RTL-ranked ones.  Preserve both
    rtlguide and explicit ``--rules`` order; ordinary/aggressive sweeps retain
    their registry order.
    """
    available = RULES + AGGRESSIVE_RULES
    by_key = {entry[0]: entry for entry in available}
    if requested:
        keys = requested
    elif guided is not None:
        keys = guided
    elif aggressive:
        return list(available)
    else:
        return list(RULES)

    selected = []
    seen = set()
    for key in keys:
        if key in by_key and key not in seen:
            selected.append(by_key[key])
            seen.add(key)
    return selected


# ---------------------------------------------------------------------------
# Scoring: build the candidate, then asmdiff -n (reuse that build) for the
# instruction-distance. Separating the build lets us tell "didn't compile"
# (INVALID) from "compiled, scored N" — asmdiff alone conflates them.
# ---------------------------------------------------------------------------

SUMMARY = re.compile(
    r": (\d+) (?:displayed )?differing lines in \d+ blocks; "
    r"(?:raw aligned residual \d+ lines in \d+ blocks; )?"
    r"length ours (\d+) vs target (\d+)(?:;[^\]]*)?\]"
)

# Process lifecycle is shared with permute.py so the two long-running drivers
# cannot drift apart; see tools/proclife.py for why each piece exists.
_LIBC = proclife.LIBC
_set_parent_death_signal = proclife.set_parent_death_signal


def arm_parent_death_signal():
    """Prevent a command runner's death from orphaning the autorules driver."""
    if _LIBC is None:
        return
    parent = os.getppid()
    _set_parent_death_signal(signal.SIGTERM)
    if os.getppid() != parent:
        raise InterruptedError("autorules parent exited during startup")


def interruption_handlers():
    return proclife.interruption_handlers("autorules")


def _terminate_process_group(proc):
    return proclife.terminate_process_group(proc)


def _run_owned(args, **kwargs):
    """subprocess.run equivalent whose process group cannot escape an abort."""
    proc = proclife.owned_popen(args, **kwargs)
    try:
        stdout, stderr = proc.communicate()
    except BaseException:
        _terminate_process_group(proc)
        raise
    return subprocess.CompletedProcess(args, proc.returncode, stdout, stderr)


def _source_override_var(name):
    """Shake's per-function staged-source environment oracle."""
    return SOURCE_OVERRIDE_PREFIX + name





def _write_text(path, text):
    with open(path, "w") as stream:
        stream.write(text)


def atomic_write(path, text):
    """Publish the selected result in one rename, preserving source mode."""
    directory = os.path.dirname(os.path.abspath(path))
    mode = os.stat(path).st_mode
    fd, temporary = tempfile.mkstemp(
        prefix="." + os.path.basename(path) + ".autorules-", dir=directory,
        text=True,
    )
    try:
        with os.fdopen(fd, "w") as stream:
            stream.write(text)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temporary, mode)
        os.replace(temporary, path)
    finally:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass


@contextmanager
def staged_candidate(name, original):
    """Yield a private candidate file consumed through matchingSource."""
    os.makedirs(os.path.join(ROOT, ".shake"), exist_ok=True)
    with tempfile.TemporaryDirectory(
            prefix=f"autorules-{name}-", dir=os.path.join(ROOT, ".shake")) as directory:
        path = os.path.join(directory, name + ".c")
        _write_text(path, original)
        yield path


def score(name, partial, source_override=None):
    env = dict(os.environ)
    if partial:
        env["NON_MATCHING"] = name
    override_var = _source_override_var(name)
    if source_override is None:
        env.pop(override_var, None)
    else:
        env[override_var] = os.path.abspath(source_override)
    b = _run_owned(["./Build"], stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL, env=env)
    if b.returncode in (126, 127):
        # Could not exec ./Build (execve E2BIG: an env string > 128 KiB, see
        # docs/build-system.md). Scoring that as "this edit does not compile"
        # would silently poison the greedy search -- a good edit discarded as
        # INVALID. Abort instead.
        sys.exit("autorules: could not EXEC ./Build -- environment failure, not a "
                 "bad edit. An env var exceeds 128 KiB (env | awk -F= "
                 "'length($0) > 131072 {print $1}'). See docs/build-system.md")
    if b.returncode != 0:
        return (False, INVALID, None, None)
    # Score by matchdiff's AUTHORITATIVE whole-image byte count, not asmdiff's
    # differing-*lines*. The line count can drop while real bytes rise (an
    # instruction-alignment artifact) -- that false win was adopted on
    # Think3firstattack (13->9 lines but 28->29 bytes) and had to be caught by an
    # agent. matchdiff -n reuses this same build (no rebuild). A LENGTH MISMATCH
    # (wrong-length draft, everything shifts) gets a large penalty so the greedy
    # search never prefers it.
    r = _run_owned([sys.executable, MATCHDIFF, name, "-n"],
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                   text=True, env=env)
    out = r.stdout
    if "MATCH!" in out:
        return (True, 0, 0, 0)
    # Raw linked bytes remain authoritative, but retain asmdiff's aligned-line
    # count as a structural diagnostic.  A local rewrite can shorten the total
    # byte diff while replacing a correct signed instruction sequence with a
    # worse unsigned one elsewhere.  Reporting that non-Pareto movement keeps
    # the greedy result reviewable without overruling a genuine byte match.
    shape = _run_owned([sys.executable, ASMDIFF, name, "-n"],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                       text=True, env=env)
    sm = SUMMARY.search(shape.stdout)
    shape_lines = int(sm.group(1)) if sm else None
    length_delta = abs(int(sm.group(2)) - int(sm.group(3))) if sm else None
    lm = re.search(r"linked (\d+) bytes .*? extent (\d+)", out) or \
        re.search(r"extent (\d+) bytes, but the linker placed (\d+)", out)
    if "LENGTH MISMATCH" in out and lm:
        a, b_ = int(lm.group(1)), int(lm.group(2))
        return (False, 1000 + abs(a - b_), shape_lines, length_delta)
    m = re.search(r"\((\d+) in the whole image\)", out)
    if not m:
        return (False, INVALID, shape_lines, length_delta)
    return (False, int(m.group(1)), shape_lines, length_delta)


def shape_regression_note(before, after, match=False):
    """Human-facing warning for a byte win that worsens aligned local shape."""
    if match or before is None or after is None:
        return ""
    before_lines, before_length = before
    after_lines, after_length = after
    if (before_length is not None and after_length is not None and
            after_length > before_length):
        return f"  LENGTH-SHAPE REGRESSION {before_length}→{after_length}"
    if (before_lines is not None and after_lines is not None and
            before_length == after_length and after_lines > before_lines):
        return f"  LOCAL-SHAPE REGRESSION {before_lines}→{after_lines} lines"
    # On an ALREADY length-mismatched draft, matchdiff's score is the length
    # penalty alone (1000 + |delta|), so ANY edit that shortens the function
    # toward the target scores better -- including one that shortens it by
    # DELETING instructions the target has and the draft already matched.
    # ChasetoTarget adopted `deg: short->s32` this way (1012->1008): it removed
    # the target's own sll/sra sign-extension pair from a region that was
    # already byte-correct, and PSX.SYM independently records `deg` as a short.
    # A length win that costs aligned lines is that signature; report it rather
    # than refuse, since a genuine length fix can also perturb alignment.
    if (before_lines is not None and after_lines is not None and
            before_length is not None and after_length is not None and
            after_length < before_length and after_lines > before_lines):
        return (f"  LENGTH-WIN SHAPE REGRESSION {before_lines}→{after_lines} "
                f"lines while length {before_length}→{after_length} — verify it "
                f"did not delete instructions the target has")
    return ""


def shape_candidate_allowed(before, after, match=False):
    """Do not trade an exact-length state for a wrong-length byte-score mirage.

    matchdiff penalizes a length mismatch, but a large exact-length residual can
    still score above that penalty (StageEndScreen was 4273 bytes different, so
    a one-instruction-short candidate scored 1004).  Once a search state has
    zero length delta, a non-matching child with a known nonzero delta cannot be
    an improvement: all following aligned bytes have shifted.  Reject it before
    greedy adoption or beam ranking.  Unknown shape diagnostics remain eligible,
    and a true byte match is always authoritative.
    """
    if match or before is None or after is None:
        return True
    before_length = before[1]
    after_length = after[1]
    return not (before_length == 0 and after_length is not None and
                after_length != 0)


def write(path, lines):
    _write_text(path, "\n".join(lines))


def _candidates(text, name, partial, rules):
    span = region_span(text, partial)  # edits can move a partial's closing #endif
    seen = set()
    for key, _desc, gen in rules:
        for label, new_text in gen(text, name, span):
            if new_text == text:
                continue
            digest = hashlib.sha1(new_text.encode()).digest()
            if digest in seen:
                continue
            seen.add(digest)
            yield key, label, new_text


def greedy_search(path, original, name, partial, rules, base, once=False,
                  shape=(None, None)):
    """Search using `path` as a private staged source, never the live file."""
    cur_text = original
    applied = []
    match = False
    while True:
        best = None  # (score, label, new_text, match)
        tried = 0
        for _key, label, new_text in _candidates(cur_text, name, partial, rules):
            write(path, new_text.split("\n"))
            m, sc, l1, l2 = score(name, partial, path)
            write(path, cur_text.split("\n"))
            tried += 1
            ds = "  invalid" if sc == INVALID else f"  {base}→{sc}"
            warning = (shape_regression_note(shape, (l1, l2), m)
                       if sc < base else "")
            allowed = shape_candidate_allowed(shape, (l1, l2), m)
            improves = sc < base and allowed
            print(f"  {label:34} {ds}{'  ✓' if improves else ''}"
                  f"{'  MATCH' if m else ''}{warning}")
            if improves and (best is None or sc < best[0]):
                best = (sc, label, new_text, m, l1, l2)
        if best is None:
            print(f"  (no improving edit among {tried} candidates)")
            break
        sc, label, new_text, m, l1, l2 = best
        applied.append((label, base, sc))
        print(f"  → adopt [{label}]  {base}→{sc}")
        cur_text = new_text
        write(path, cur_text.split("\n"))
        base = sc
        shape = (l1, l2)
        if m or once:
            match = m
            break
    return cur_text, base, match, applied


def beam_search(path, original, name, partial, rules, base, width, depth,
                allow_regress, budget, shape=(None, None)):
    """Bounded multi-edit search that retains neutral enabling transformations.

    The old greedy loop cannot discover `loop-fence A` (same byte score)
    followed by `split B` (match). A small beam is enough for the cookbook's known
    enabling sequences and remains explainable: every path is printed.
    """
    start = base
    initial = dict(text=original, score=base, shape=shape, match=False, path=[])
    frontier = [initial]
    best = initial
    seen = {hashlib.sha1(original.encode()).digest()}
    cache = {}
    tried = 0
    exhausted = False
    for level in range(1, depth + 1):
        children = []
        print(f"  -- beam depth {level}/{depth}, {len(frontier)} state(s) --")
        for state in frontier:
            for _key, label, new_text in _candidates(state["text"], name, partial, rules):
                digest = hashlib.sha1(new_text.encode()).digest()
                if digest in seen:
                    continue
                seen.add(digest)
                if tried >= budget:
                    exhausted = True
                    break
                write(path, new_text.split("\n"))
                if digest in cache:
                    m, sc, l1, l2 = cache[digest]
                else:
                    m, sc, l1, l2 = score(name, partial, path)
                    cache[digest] = (m, sc, l1, l2)
                    tried += 1
                write(path, state["text"].split("\n"))
                ds = "invalid" if sc == INVALID else str(sc)
                warning = (shape_regression_note(state["shape"], (l1, l2), m)
                           if sc < state["score"] else "")
                allowed = shape_candidate_allowed(state["shape"], (l1, l2), m)
                improves_best = allowed and sc < best["score"]
                print(f"  d{level} {label:31} {state['score']}→{ds}"
                      f"{'  ✓' if improves_best else ''}{'  MATCH' if m else ''}"
                      f"{warning}")
                if (sc == INVALID or not allowed or
                        sc > start + allow_regress):
                    continue
                child = dict(
                    text=new_text, score=sc, shape=(l1, l2), match=m,
                    path=state["path"] + [(label, state["score"], sc)],
                )
                children.append(child)
                if m or sc < best["score"]:
                    best = child
                if m:
                    exhausted = True
                    break
            if exhausted:
                break
        if best["match"] or exhausted or not children:
            break
        # Deterministic tie order plus path diversity; a neutral pure-C edit
        # survives alongside immediate byte wins instead of being discarded.
        children.sort(key=lambda s: (
            s["score"], len(s["path"]),
            hashlib.sha1(s["text"].encode()).hexdigest(),
        ))
        frontier = children[:width]
    if tried >= budget:
        print(f"  (candidate budget {budget} reached)")
    if best["score"] >= start and not best["match"]:
        print(f"  (no improving path among {tried} compiled candidates)")
        return original, start, False, []
    write(path, best["text"].split("\n"))
    return best["text"], best["score"], best["match"], best["path"]


def main():
    # Tool frontends often capture stdout through a pipe.  Stream each scored
    # candidate immediately so a long guided run remains observable.
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(line_buffering=True)
    ap = argparse.ArgumentParser()
    ap.add_argument("name", nargs="?")
    ap.add_argument("--dry", action="store_true", help="report only; don't modify the file")
    ap.add_argument("--once", action="store_true", help="one pass, no greedy re-sweep")
    ap.add_argument("--list", action="store_true", help="list the registered rules")
    ap.add_argument("--guided", action="store_true",
                    help="use target asm + RTL line map to choose advanced rules")
    ap.add_argument("--aggressive", action="store_true",
                    help="also sweep pure-C loop-note and CFG rules blindly")
    ap.add_argument("--rules", help="comma-separated rule keys (overrides the default set)")
    ap.add_argument("--beam", type=int, metavar="WIDTH",
                    help="bounded multi-edit beam search (guided default: 4)")
    ap.add_argument("--depth", type=int, default=2, help="beam edit depth (default: 2)")
    ap.add_argument("--allow-regress", type=int,
                    help="bytes a beam state may regress (guided default: 4)")
    ap.add_argument("--budget", type=int, default=160,
                    help="maximum candidate builds in beam mode (default: 160)")
    args = ap.parse_args()

    if args.list or not args.name:
        ast = "available" if _TS is not None else "MISSING (AST rules disabled)"
        print(f"registered mechanical rules  [tree-sitter: {ast}]:")
        for key, desc, gen in RULES:
            tag = " (AST)" if gen in AST_RULES else ""
            print(f"  {key:12} {desc}{tag}")
        print("guided/advanced rules (opt-in):")
        for key, desc, _gen in AGGRESSIVE_RULES:
            print(f"  {key:18} {desc} (AST)")
        if not args.name:
            print("\ngive a <Name> to sweep it.")
        return 0

    name = args.name
    if _TS is None:
        disabled = ", ".join(key for key, _desc, gen in RULES + AGGRESSIVE_RULES
                             if gen in AST_RULES)
        print(f"autorules: tree-sitter unavailable — AST rules ({disabled}) "
              "are DISABLED; running line-based rules only. "
              "Run inside `nix develop` for the full set.", file=sys.stderr)
    path, original, partial = load(name)
    match, base, lo, lt = score(name, partial)
    if base == INVALID:
        sys.exit(f"autorules: {name} does not build as-is — fix it first "
                 f"({'set NON_MATCHING and ' if partial else ''}run ./Build).")
    print(f"autorules {name} — baseline MATCH" if match
          else f"autorules {name} — baseline {base} differing bytes (whole image)")
    if match:
        print("already byte-identical — nothing to do.")
        return 0

    global GUIDED_LINES, GUIDED_VARIABLES
    guide = None
    if args.guided:
        try:
            import rtlguide
            guide = rtlguide.diagnose(name, build=False, rtl=True)
        except (FileNotFoundError, ValueError, RuntimeError) as e:
            sys.exit(f"autorules: rtlguide failed: {e}")
        GUIDED_LINES = set(guide.get("source_lines", []))
        GUIDED_VARIABLES = {
            variable.encode()
            for substitution in guide.get("register_substitutions", [])
            for variable in substitution.get("variables", [])
            if re.fullmatch(r"[A-Za-z_]\w*", variable)
        }
        print(f"guided by {guide['primary']}: lines "
               f"{','.join(map(str, sorted(GUIDED_LINES))) or '?'}; "
              f"locals {','.join(x.decode() for x in sorted(GUIDED_VARIABLES)) or '?'}; "
              f"rules {','.join(guide['rules']) or '(none)'}")

    requested = [x.strip() for x in (args.rules or "").split(",") if x.strip()]
    available = RULES + AGGRESSIVE_RULES
    known = {x[0] for x in available}
    bad = [x for x in requested if x not in known]
    if bad:
        sys.exit(f"autorules: unknown rule(s) {bad}; use --list")
    rules = select_rules(
        requested=requested,
        guided=guide["rules"] if args.guided else None,
        aggressive=args.aggressive,
    )
    if not rules:
        sys.exit("autorules: no runnable rules selected")
    print("selected: " + ", ".join(x[0] for x in rules))

    beam = args.beam
    allow_regress = args.allow_regress
    if args.guided and beam is None:
        beam = 4
    if allow_regress is None:
        allow_regress = 4 if args.guided else 0
    if args.once and beam:
        args.depth = 1

    # Every speculative rewrite lives under .shake and reaches the compiler via
    # TENCHU_MATCH_SOURCE_<Name>. The checked-in path is published only once, if
    # an improving result is accepted below.
    with staged_candidate(name, original) as candidate_path:
        if beam:
            cur_text, base, match, applied = beam_search(
                candidate_path, original, name, partial, rules, base, beam, args.depth,
                allow_regress, args.budget, (lo, lt),
            )
        else:
            cur_text, base, match, applied = greedy_search(
                candidate_path, original, name, partial, rules, base, args.once,
                (lo, lt),
            )

    print()
    if applied:
        chain = "  ".join(f"[{l}: {a}→{b}]" for l, a, b in applied)
        print(f"applied: {chain}")
    print("result: MATCH ✓" if match else f"result: {base} differing (residual)")

    if args.dry or not applied:
        if applied:
            print(f"(--dry: left {path} unchanged)")
    else:
        atomic_write(path, cur_text)
        print(f"wrote {path}")
    return 0 if match else 1


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with interruption_handlers():
            arm_parent_death_signal()
            with matching_tool_lock("autorules", target):
                sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"autorules: {e}")
    except InterruptedError as e:
        sys.exit(f"autorules: {e}; live source unchanged")
    except KeyboardInterrupt:
        sys.exit("autorules: interrupted; live source unchanged")
