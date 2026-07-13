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

Safe by construction: byte-identical to the original IS correctness, so a full
MATCH is always valid; any non-zero result is advisory and gets reviewed. A
semantically-wrong rewrite simply fails to improve the score and is discarded.

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
import argparse, ctypes, glob, hashlib, itertools, os, re, signal, subprocess, sys, tempfile
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


def rule_type_width(text, name, span):
    """Flip a local variable's integer type across width/signedness.

    THE most-cited mechanical lever (cookbook Expressions): a short-returning
    call result that indexes an array wants s32 not s16; a u16 field read
    signed; a terminator's signedness; short-vs-int call results (PauseProc);
    an HImode switch guard that must not reuse an SImode case constant
    (ActATTACK).
    For each local declaration we try each *distinct* (width, signedness)
    canonical type. Line-based (robust without the AST); confined to the body.
    """
    lines, ranges = body_line_ranges(text, span, name)
    for rng in ranges:
        for i, raw, code in uncommented(lines, rng):
            m = DECL.match(code)
            if not m:
                continue
            cur = m.group(3)
            w, s = TYPES[cur]
            for (w2, s2) in flip_targets(w, s):
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
    `bspan`. `data` is the utf-8 bytes; returns (root_data, body_node)."""
    if _TS is None:
        return None
    root = _TS.parse(data).root_node
    want = name.encode()
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


def _lone_if(node):
    """`node` is a single if_statement, or a block wrapping exactly one."""
    if node.type == "if_statement":
        return node
    if node.type == "compound_statement":
        stmts = [c for c in node.named_children if c.type != "comment"]
        if len(stmts) == 1 and stmts[0].type == "if_statement":
            return stmts[0]
    return None


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
    authoritative scoring decides among the bounded scalar alternatives.
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
        for next_width, next_signed in flip_targets(width, signed):
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
    """Rewrite a direct ``abs(x)`` call as ``__builtin_abs(x)``.

    The matching build deliberately passes ``-fno-builtin`` to cc1, so an
    ordinary library call can no longer fold to the target's inline
    ``bgez/move/negu`` sequence.  The explicit builtin restores that source
    choice locally without weakening the translation unit's compiler flags.
    Restrict this to a direct, one-argument call and reject a same-named local;
    authoritative scoring decides whether the target wanted the call or the
    inline form.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    if b"abs" in _nonvolatile_local_names(data, body):
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
    """Name an extern-array base before taking one indexed element address.

    ``T *p = &Global[i]`` and ``T *base = Global; T *p = &base[i]`` are
    equivalent, but the latter gives split-addresses/CSE a distinct base pseudo
    and scheduling UID.  Keep this guided and limited to a single pointer
    declarator with an identifier array base.
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


def rule_flag_arm_assign(text, name, span):
    """Move a local 0/1 default and override to the ends of their CFG arms.

    This shortens the flag's live range past the comparisons that precede each
    definition, allowing their hard result register to be reused in the branch
    delay slots.  Only the safe adjacent shape is enumerated: a nonvolatile
    local, no existing else, no use of the flag in the condition/body, and no
    early exit that could bypass the moved override.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
    exits = ("break_statement", "continue_statement", "goto_statement",
             "return_statement")
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
            if (flag not in locals_ or
                    re.search(rb"&\s*" + re.escape(flag) + rb"\b", _txt(data, body)) or
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
            if any(_txt(data, ident) == flag for ident in _find(condition, ("identifier",))):
                continue
            remaining = arm_statements[1:]
            if any(_find(statement, exits) for statement in remaining):
                continue
            if any(_txt(data, ident) == flag
                   for statement in remaining
                   for ident in _find(statement, ("identifier",))):
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


def rule_identical_arm_fence(text, name, span):
    """Duplicate one expression statement into byte-identical if/else arms.

    jump optimization removes the branch, but the temporary CFG can fence
    sched/CSE without assigning loop-depth weight to the statement's operands.
    Conditions come only from nonvolatile locals either used by the statement
    already or unconditionally defined in the three preceding statements; no
    new uninitialised discriminator is invented.
    """
    data = text.encode()
    body = _func_body(data, name, _byte_span(text, span))
    if body is None:
        return
    locals_ = _nonvolatile_local_names(data, body)
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
    ("extern-array", "extern T NAME; -> extern T NAME[]; + NAME->NAME[0] (-G8 split)", rule_extern_array),
    ("and-nest", "split/merge if(a && b) <-> nested ifs (no else)", rule_and_nest),
    ("temp-inline", "inline a single-use local temp into its use", rule_temp_inline),
    ("call-result-split", "split a reused direct-call result into single-definition locals", rule_call_result_split),
    ("late-pointer-direct", "inline a repeated pointer-global assignment in its late region", rule_late_pointer_direct),
    ("call-arg-pair", "inline adjacent same-call temps into one consumer call", rule_call_arg_pair_inline),
    ("abs-ge", "rewrite (x<0)?-x:x -> (x>=0)?x:-x (the abssi2 fold)", rule_abs_ge),
    ("builtin-abs", "abs(x) -> __builtin_abs(x) under cc1 -fno-builtin", rule_builtin_abs),
    ("or-inplace", "rewrite X = X|C -> X |= C (local-alloc lever)", rule_or_inplace),
    ("rand-mod", "split/merge x=rand()%K (call-result allocation lever)", rule_rand_mod_split),
    ("subscript-postinc", "split/merge a[i];i++ <-> a[i++] (working-copy lever)", rule_subscript_postinc),
    ("ptr-index-sum", "T *p = base+idx -> (T*)(idx*sizeof(T)+(int)base)", rule_ptr_index_sum),
    ("vector-copy", "four vx/vy/vz/pad field copies -> one struct assignment", rule_vector_copy),
    ("assignment-chain", "merge/split same-literal field stores", rule_assignment_chain),
    ("vector-copy-adjust", "split/merge a literal-adjusted vx/vy/vz field copy", rule_vector_copy_adjust),
    ("split-chain", "t = (x-C)>>S -> t = x-C; t = t>>S (split fused chain)", rule_split_chain),
    ("min-ternary", "x=a; if(b<x) x=b; -> x = (b<a)?b:a (min-vs-memory)", rule_min_ternary),
    ("cmp-swap", "a>mem -> mem<a (comparison operand-order lever)", rule_cmp_swap),
]

AGGRESSIVE_RULES = [
    ("cmp-polarity", "swap two local comparison operands (regalloc ref-order lever)", rule_cmp_polarity),
    ("eq-literal-swap", "swap ==/!= literal operand order (v0/v1 lever)", rule_eq_literal_swap),
    ("shift16-mul", "casted short x<<16 -> x*0x10000 (raw sll lever)", rule_shift16_mul),
    ("shift-stage", "split x>>N into every two-stage constant shift", rule_shift_stage),
    ("mul-affine-shape", "toggle x*M+C with (x+C/M)*M", rule_mul_affine_shape),
    ("ptr-base-split", "name an extern array base before indexed address", rule_ptr_base_split),
    ("plus-group", "enumerate 3-term + grouping/constant placement (fold lever)", rule_plus_group),
    ("add-prefix-temp", "name a signed 2-term seam in a narrowed 3-term sum", rule_add_prefix_temp),
    ("flag-arm-assign", "move local 0/1 definitions after each arm's comparisons", rule_flag_arm_assign),
    ("shared-tail-assign", "duplicate one shared assignment into both preceding arms", rule_shared_tail_assign),
    ("if-else-invert", "invert a compound if/else to swap physical body layout", rule_if_else_invert),
    ("adjacent-field-store-swap", "swap adjacent literal stores to distinct fields", rule_adjacent_field_store_swap),
    ("switch-cse-evict", "dead-overwrite an entry index before a fresh switch load", rule_switch_cse_evict),
    ("pointee-volatile", "toggle volatile on a local integer pointer's pointee", rule_pointee_volatile),
    ("loop-fence", "wrap an if/loop in a zero-code one-shot do loop", rule_loop_fence),
    ("nested-loop-fence", "atomically add two or three loop weights at one site", rule_nested_loop_fence),
    ("paired-loop-fence", "wrap adjacent groups in two atomic one-shot loops", rule_paired_loop_fence),
    ("loop-range", "wrap 2-4 adjacent statements in one zero-code do loop", rule_loop_range),
    ("loop-boundary-shift", "move the next statement across an existing LOOP_END", rule_loop_boundary_shift),
    ("identical-arm-fence", "duplicate one statement into zero-code identical arms", rule_identical_arm_fence),
    ("case-fence", "if/else -> two-way switch (hard cross-jump CODE_LABEL)", rule_case_fence),
    ("sparse-eq-switch", "three literal equality arms -> sparse switch permutations", rule_sparse_eq_switch),
]

LINE_RULES = {rule_type_width, rule_extern_array, rule_pointee_volatile}
AST_RULES = {gen for _key, _desc, gen in RULES + AGGRESSIVE_RULES
             if gen not in LINE_RULES}


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

_PR_SET_PDEATHSIG = 1
_LIBC = ctypes.CDLL(None, use_errno=True) if sys.platform.startswith("linux") else None


def _source_override_var(name):
    """Shake's per-function staged-source environment oracle."""
    return SOURCE_OVERRIDE_PREFIX + name


def _set_parent_death_signal(sig):
    """Ask Linux to signal this process when its current parent exits."""
    if _LIBC is None:
        return
    if _LIBC.prctl(_PR_SET_PDEATHSIG, sig, 0, 0, 0) != 0:
        err = ctypes.get_errno()
        raise OSError(err, os.strerror(err))


def arm_parent_death_signal():
    """Prevent a command runner's death from orphaning the autorules driver.

    PR_SET_PDEATHSIG has a small setup race. Checking getppid again closes it:
    if the original parent disappeared between the two calls, abort before the
    driver can start a candidate search.
    """
    if _LIBC is None:
        return
    parent = os.getppid()
    _set_parent_death_signal(signal.SIGTERM)
    if os.getppid() != parent:
        raise InterruptedError("autorules parent exited during startup")


@contextmanager
def interruption_handlers():
    """Convert frontend termination into a catchable driver interruption."""
    def interrupted(signum, _frame):
        raise InterruptedError(f"autorules interrupted by signal {signum}")

    old_handlers = {}
    for sig in (signal.SIGTERM, getattr(signal, "SIGHUP", None)):
        if sig is not None:
            old_handlers[sig] = signal.signal(sig, interrupted)
    try:
        yield
    finally:
        for sig, handler in old_handlers.items():
            signal.signal(sig, handler)


def _child_parent_death_setup(expected_parent):
    """Popen child hook: do not leave ./Build alive if autorules is killed."""
    if _LIBC is None:
        return
    # SIGKILL is intentional here. The child has not exec'd yet, so running a
    # Python signal handler in the post-fork/pre-exec window would be unsafe.
    _set_parent_death_signal(signal.SIGKILL)
    if os.getppid() != expected_parent:
        os._exit(128 + signal.SIGKILL)


def _terminate_process_group(proc):
    """Terminate and reap a subprocess plus every descendant in its session."""
    try:
        os.killpg(proc.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        pass
    # The group can outlive its leader: a shell/Shake process may exit on TERM
    # before one of its compiler children does. Probe by sending KILL even when
    # wait() already reaped the immediate child; ESRCH means the group is gone.
    try:
        os.killpg(proc.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    if proc.poll() is None:
        proc.wait()


def _run_owned(args, **kwargs):
    """subprocess.run equivalent whose process group cannot escape an abort.

    `subprocess.run` does not terminate its child when a Python signal handler
    raises during `wait()`. Autorules used to restore the source in that case
    while the interrupted Shake/build tree continued independently. Give each
    command a session, kill the whole group on every exception, and arrange for
    the immediate child to die even if autorules itself receives SIGKILL.
    """
    parent = os.getpid()
    child_setup = ((lambda: _child_parent_death_setup(parent))
                   if _LIBC is not None else None)
    proc = subprocess.Popen(
        args,
        start_new_session=True,
        preexec_fn=child_setup,
        **kwargs,
    )
    try:
        stdout, stderr = proc.communicate()
    except BaseException:
        _terminate_process_group(proc)
        raise
    return subprocess.CompletedProcess(args, proc.returncode, stdout, stderr)


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

    global GUIDED_LINES
    guide = None
    if args.guided:
        try:
            import rtlguide
            guide = rtlguide.diagnose(name, build=False, rtl=True)
        except (FileNotFoundError, ValueError, RuntimeError) as e:
            sys.exit(f"autorules: rtlguide failed: {e}")
        GUIDED_LINES = set(guide.get("source_lines", []))
        print(f"guided by {guide['primary']}: lines "
              f"{','.join(map(str, sorted(GUIDED_LINES))) or '?'}; "
              f"rules {','.join(guide['rules']) or '(none)'}")

    requested = [x.strip() for x in (args.rules or "").split(",") if x.strip()]
    available = RULES + AGGRESSIVE_RULES
    known = {x[0] for x in available}
    bad = [x for x in requested if x not in known]
    if bad:
        sys.exit(f"autorules: unknown rule(s) {bad}; use --list")
    if requested:
        selected_keys = set(requested)
    elif args.guided:
        selected_keys = set(guide["rules"])
    elif args.aggressive:
        selected_keys = {x[0] for x in RULES + AGGRESSIVE_RULES}
    else:
        selected_keys = {x[0] for x in RULES}
    rules = [x for x in available if x[0] in selected_keys]
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
