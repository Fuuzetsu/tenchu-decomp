#!/usr/bin/env python3
"""Mechanically apply the cookbook's *local* matching rules and keep what helps.

Most cookbook rules are RECOGNITION rules: you must read the asm diff to know
which one applies and where (loop shape, switch-vs-ladder, union-offset casts,
two-regs-same-value...). A blind sweep can't place those. But a handful are
purely MECHANICAL — a token/expression rewrite at an enumerable site whose only
open question is "does it move the bytes the right way?". Nobody (agent or human)
should hand-apply those every time.

This tool enumerates every applicable site of each mechanical rule, tries it,
rebuilds, and rescoring with tools/asmdiff.py — then GREEDILY keeps the single
best improving edit and repeats until nothing helps (or it MATCHES). Greedy
re-sweeping handles order-dependence for free: independent wins compose, and an
edit that only helps *after* another is found on the next pass.

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

Run inside the nix devShell. Each candidate costs one ./Build (incremental), so
a run is a few dozen builds — a minute or two, unattended.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

SRC = "src/main.exe"
ASMDIFF = "tools/asmdiff.py"
INVALID = 10**9  # candidate did not compile


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
    signed; a terminator's signedness; short-vs-int call results (PauseProc).
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


RULES = [
    ("type-width", "flip a local's integer type across width/signedness", rule_type_width),
    ("and-nest", "split/merge if(a && b) <-> nested ifs (no else)", rule_and_nest),
    ("temp-inline", "inline a single-use local temp into its use", rule_temp_inline),
    ("abs-ge", "rewrite (x<0)?-x:x -> (x>=0)?x:-x (the abssi2 fold)", rule_abs_ge),
    ("or-inplace", "rewrite X = X|C -> X |= C (local-alloc lever)", rule_or_inplace),
    ("ptr-index-sum", "T *p = base+idx -> (T*)(idx*sizeof(T)+(int)base)", rule_ptr_index_sum),
]


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
    r = subprocess.run([sys.executable, ASMDIFF, name, "-n"],
                       capture_output=True, text=True, env=env)
    m = SUMMARY.search(r.stdout)
    if not m:
        return (False, INVALID, None, None)
    nd, lo, lt = int(m.group(1)), int(m.group(2)), int(m.group(3))
    match = (r.returncode == 0)
    return (match, nd + 3 * abs(lo - lt), lo, lt)


def write(path, lines):
    open(path, "w").write("\n".join(lines))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name", nargs="?")
    ap.add_argument("--dry", action="store_true", help="report only; don't modify the file")
    ap.add_argument("--once", action="store_true", help="one pass, no greedy re-sweep")
    ap.add_argument("--list", action="store_true", help="list the registered rules")
    args = ap.parse_args()

    if args.list or not args.name:
        ast = "available" if _TS is not None else "MISSING (AST rules disabled)"
        print(f"registered mechanical rules  [tree-sitter: {ast}]:")
        for key, desc, gen in RULES:
            uses_ast = gen in (rule_and_nest, rule_temp_inline)
            tag = " (AST)" if uses_ast else ""
            print(f"  {key:12} {desc}{tag}")
        if not args.name:
            print("\ngive a <Name> to sweep it.")
        return 0

    name = args.name
    if _TS is None:
        print("autorules: tree-sitter unavailable — AST rules (&&-nest, "
              "temp-inline) are DISABLED; running line-based rules only. "
              "Run inside `nix develop` for the full set.", file=sys.stderr)
    path, original, partial = load(name)
    span = region_span(original, partial)

    match, base, lo, lt = score(name, partial)
    if base == INVALID:
        sys.exit(f"autorules: {name} does not build as-is — fix it first "
                 f"({'set NON_MATCHING and ' if partial else ''}run ./Build).")
    tag = "MATCH" if match else f"{base} differing"
    print(f"autorules {name} — baseline {tag} (len {lo}=={lt})" if match
          else f"autorules {name} — baseline {base} differing lines (len {lo} vs {lt})")
    if match:
        print("already byte-identical — nothing to do.")
        return 0

    cur_text = original
    applied = []
    passes = 0
    try:
      while True:
        passes += 1
        best = None  # (score, label, new_text, match)
        tried = 0
        for _key, _desc, gen in RULES:
            for label, new_text in gen(cur_text, name, span):
                if new_text == cur_text:
                    continue
                write(path, new_text.split("\n"))
                m, sc, l1, l2 = score(name, partial)
                write(path, cur_text.split("\n"))  # restore between candidates
                tried += 1
                ds = "  invalid" if sc == INVALID else f"  {base}→{sc}"
                print(f"  {label:28} {ds}{'  ✓' if (sc < base) else ''}"
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
        if m or args.once:
            match = m
            break
    except BaseException:
        write(path, original.split("\n"))  # never leave a half-mutated file
        raise

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
    sys.exit(main())
