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
# The rule registry. A rule is gen(lines, ranges) -> iterator of
# (label, new_lines). Each yielded candidate is one applied site; the driver
# scores them all and greedily keeps the best. Add mechanical rules here as the
# cookbook grows (structural ones need an AST — see the roadmap in the module
# docstring / docs).
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


def rule_type_width(lines, ranges):
    """Flip a local variable's integer type across width/signedness.

    THE most-cited mechanical lever (cookbook Expressions): a short-returning
    call result that indexes an array wants s32 not s16; a u16 field read
    signed; a terminator's signedness; short-vs-int call results (PauseProc).
    For each local declaration we try each *distinct* (width, signedness)
    canonical type.
    """
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
                yield (f"{m.group(6)}: {cur}→{rep}", nl)


def _swap_decl_type(raw, m, rep):
    """Replace the type token in `raw` at the position it occupies in the
    matched (comment-stripped) code. We re-find the type token followed by the
    same variable name to stay robust to a stripped trailing comment."""
    var = re.escape(m.group(6))
    typ = re.escape(m.group(3))
    pat = re.compile(rf"(^\s*(?:const\s+|volatile\s+)*)({typ})(\s+\**\s*{var}\b)")
    new, n = pat.subn(rf"\g<1>{rep}\g<3>", raw, count=1)
    return new if n else None


RULES = [
    ("type-width", "flip a local's integer type across width/signedness", rule_type_width),
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
                       stderr=subprocess.STDOUT, env=env)
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
        print("registered mechanical rules:")
        for key, desc, _ in RULES:
            print(f"  {key:12} {desc}")
        if not args.name:
            print("\ngive a <Name> to sweep it.")
        return 0

    name = args.name
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
        lines, ranges = body_line_ranges(cur_text, span, name)
        best = None  # (score, label, new_text, match)
        tried = 0
        for _key, _desc, gen in RULES:
            for label, new_lines in gen(lines, ranges):
                new_text = "\n".join(new_lines)
                if new_text == cur_text:
                    continue
                write(path, new_lines)
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
