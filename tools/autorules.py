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
per-worktree lock, and SIGTERM/SIGHUP restore the original source just like
Ctrl-C, so a yielded or interrupted run cannot silently overlap a permuter.
"""
import argparse, glob, hashlib, os, re, signal, subprocess, sys

from matchlock import MatchToolBusy, matching_tool_lock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

SRC = "src/main.exe"
ASMDIFF = "tools/asmdiff.py"
MATCHDIFF = "tools/matchdiff.py"
INVALID = 10**9  # candidate did not compile

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
# temp-inline) parse with tree-sitter (get_parser("c")) and splice byte spans —
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
    for stmt in _find(body, ("if_statement", "for_statement", "while_statement")):
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


RULES = [
    ("type-width", "flip a local's integer type across width/signedness", rule_type_width),
    ("extern-array", "extern T NAME; -> extern T NAME[]; + NAME->NAME[0] (-G8 split)", rule_extern_array),
    ("and-nest", "split/merge if(a && b) <-> nested ifs (no else)", rule_and_nest),
    ("temp-inline", "inline a single-use local temp into its use", rule_temp_inline),
    ("abs-ge", "rewrite (x<0)?-x:x -> (x>=0)?x:-x (the abssi2 fold)", rule_abs_ge),
    ("or-inplace", "rewrite X = X|C -> X |= C (local-alloc lever)", rule_or_inplace),
    ("rand-mod", "split/merge x=rand()%K (call-result allocation lever)", rule_rand_mod_split),
    ("ptr-index-sum", "T *p = base+idx -> (T*)(idx*sizeof(T)+(int)base)", rule_ptr_index_sum),
    ("vector-copy", "four vx/vy/vz/pad field copies -> one struct assignment", rule_vector_copy),
    ("split-chain", "t = (x-C)>>S -> t = x-C; t = t>>S (split fused chain)", rule_split_chain),
    ("min-ternary", "x=a; if(b<x) x=b; -> x = (b<a)?b:a (min-vs-memory)", rule_min_ternary),
    ("cmp-swap", "a>mem -> mem<a (comparison operand-order lever)", rule_cmp_swap),
]

AGGRESSIVE_RULES = [
    ("cmp-polarity", "swap two local comparison operands (regalloc ref-order lever)", rule_cmp_polarity),
    ("shift16-mul", "casted short x<<16 -> x*0x10000 (raw sll lever)", rule_shift16_mul),
    ("plus-group", "enumerate 3-term + grouping/constant placement (fold lever)", rule_plus_group),
    ("add-prefix-temp", "name a signed 2-term seam in a narrowed 3-term sum", rule_add_prefix_temp),
    ("loop-fence", "wrap an if/loop in a zero-code one-shot do loop", rule_loop_fence),
    ("loop-range", "wrap 2-4 adjacent statements in one zero-code do loop", rule_loop_range),
    ("case-fence", "if/else -> two-way switch (hard cross-jump CODE_LABEL)", rule_case_fence),
]

LINE_RULES = {rule_type_width, rule_extern_array}
AST_RULES = {gen for _key, _desc, gen in RULES + AGGRESSIVE_RULES
             if gen not in LINE_RULES}


# ---------------------------------------------------------------------------
# Scoring: build the candidate, then asmdiff -n (reuse that build) for the
# instruction-distance. Separating the build lets us tell "didn't compile"
# (INVALID) from "compiled, scored N" — asmdiff alone conflates them.
# ---------------------------------------------------------------------------

SUMMARY = re.compile(r": (\d+) differing lines in \d+ blocks; "
                     r"length ours (\d+) vs target (\d+)\]")


def score(name, partial):
    env = dict(os.environ)
    if partial:
        env["NON_MATCHING"] = name
    b = subprocess.run(["./Build"], stdout=subprocess.DEVNULL,
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
    r = subprocess.run([sys.executable, MATCHDIFF, name, "-n"],
                       capture_output=True, text=True, env=env)
    out = r.stdout
    if "MATCH!" in out:
        return (True, 0, None, None)
    lm = re.search(r"linked (\d+) bytes .*? extent (\d+)", out) or \
        re.search(r"extent (\d+) bytes, but the linker placed (\d+)", out)
    if "LENGTH MISMATCH" in out and lm:
        a, b_ = int(lm.group(1)), int(lm.group(2))
        return (False, 1000 + abs(a - b_), None, None)
    m = re.search(r"\((\d+) in the whole image\)", out)
    if not m:
        return (False, INVALID, None, None)
    return (False, int(m.group(1)), None, None)


def write(path, lines):
    with open(path, "w") as f:
        f.write("\n".join(lines))


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


def greedy_search(path, original, name, partial, rules, base, once=False):
    cur_text = original
    applied = []
    match = False
    while True:
        best = None  # (score, label, new_text, match)
        tried = 0
        for _key, label, new_text in _candidates(cur_text, name, partial, rules):
            write(path, new_text.split("\n"))
            m, sc, _l1, _l2 = score(name, partial)
            write(path, cur_text.split("\n"))
            tried += 1
            ds = "  invalid" if sc == INVALID else f"  {base}→{sc}"
            print(f"  {label:34} {ds}{'  ✓' if (sc < base) else ''}"
                  f"{'  MATCH' if m else ''}")
            if sc < base and (best is None or sc < best[0]):
                best = (sc, label, new_text, m)
        if best is None:
            print(f"  (no improving edit among {tried} candidates)")
            break
        sc, label, new_text, m = best
        applied.append((label, base, sc))
        print(f"  → adopt [{label}]  {base}→{sc}")
        cur_text = new_text
        write(path, cur_text.split("\n"))
        base = sc
        if m or once:
            match = m
            break
    return cur_text, base, match, applied


def beam_search(path, original, name, partial, rules, base, width, depth,
                allow_regress, budget):
    """Bounded multi-edit search that retains neutral enabling transformations.

    The old greedy loop cannot discover `loop-fence A` (same byte score)
    followed by `split B` (match). A small beam is enough for the cookbook's known
    enabling sequences and remains explainable: every path is printed.
    """
    start = base
    initial = dict(text=original, score=base, match=False, path=[])
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
                    m, sc = cache[digest]
                else:
                    m, sc, _l1, _l2 = score(name, partial)
                    cache[digest] = (m, sc)
                    tried += 1
                write(path, state["text"].split("\n"))
                ds = "invalid" if sc == INVALID else str(sc)
                print(f"  d{level} {label:31} {state['score']}→{ds}"
                      f"{'  ✓' if sc < best['score'] else ''}{'  MATCH' if m else ''}")
                if sc == INVALID or sc > start + allow_regress:
                    continue
                child = dict(
                    text=new_text, score=sc, match=m,
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

    # exec/session frontends commonly terminate a yielded command with SIGTERM
    # or SIGHUP.  Python's default SIGTERM action bypasses the exception handler
    # below and used to leave whichever candidate happened to be under test in
    # the live source file.  Turn those signals into an exception so restoration
    # is just as reliable as Ctrl-C.
    def interrupted(signum, _frame):
        raise InterruptedError(f"autorules interrupted by signal {signum}")

    old_handlers = {}
    for sig in (signal.SIGTERM, getattr(signal, "SIGHUP", None)):
        if sig is not None:
            old_handlers[sig] = signal.signal(sig, interrupted)
    try:
        if beam:
            cur_text, base, match, applied = beam_search(
                path, original, name, partial, rules, base, beam, args.depth,
                allow_regress, args.budget,
            )
        else:
            cur_text, base, match, applied = greedy_search(
                path, original, name, partial, rules, base, args.once,
            )
    except BaseException:
        write(path, original.split("\n"))  # never leave a half-mutated file
        raise
    finally:
        for sig, handler in old_handlers.items():
            signal.signal(sig, handler)

    print()
    if applied:
        chain = "  ".join(f"[{l}: {a}→{b}]" for l, a, b in applied)
        print(f"applied: {chain}")
    print("result: MATCH ✓" if match else f"result: {base} differing (residual)")

    if args.dry or not applied:
        write(path, original.split("\n"))
        if applied:
            print(f"(--dry: reverted {path})")
    else:
        print(f"wrote {path}")
    return 0 if match else 1


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("autorules", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"autorules: {e}")
    except InterruptedError as e:
        sys.exit(f"autorules: {e}; original source restored")
