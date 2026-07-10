#!/usr/bin/env python3
"""Find executable bytes that no known function claims.

The Ghidra export (`functions.tsv`) is our function inventory, and `reverse.py`
carves each function at the SIZE that file reports. If Ghidra under-sized a
function, its tail becomes a hole: code that belongs to a real function but is
attributed to nothing. Carving that function then yields a `.c` that can NEVER
byte-match, because the true body is longer than the carve.

Careful: you cannot ask splat which bytes are code. Un-carved text is linked as
a raw blob and typed `data` in `splat.main.exe.yaml`, so only the ~200 already
carved functions live in `c` subsegments. Instead we take the text extent to
start at the first known function, subtract every function's extent, and use the
Ghidra symbol table to recognise the leftovers that are genuinely data.

For each remaining hole it says WHY it is probably a hole:

  * `prev fn <N> jumps in`  — the preceding function branches into the hole, so
                              the hole is that function's own tail
  * `ends with jr ra`       — the hole terminates in a return, likewise a tail
  * `starts w/ prologue`    — looks like a real, unlabelled function

  tools/coverage.py            report holes in the game region
  tools/coverage.py --all      include the SDK region (> 0x80060000)

Run inside the nix devShell.
"""
import argparse, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import triage  # noqa: reuse its objdump pass + function table

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYMS_TSV = os.path.join(ROOT, ".shake", "ghidra-export", "symbols.tsv")
ALIGN = 8  # holes this small are inter-function padding


def data_syms():
    out = set()
    if not os.path.exists(SYMS_TSV):
        return out
    for line in open(SYMS_TSV):
        p = line.split("\t")
        if len(p) >= 2 and p[1].strip() == "DATA":
            try:
                out.add(int(p[0], 16))
            except ValueError:
                pass
    return out


def classify(a, end, funcs, insns):
    reasons = []
    prev = [f for f in funcs if f[0] + f[1] <= a]
    if prev:
        pa, ps, pn = prev[-1]
        for ia in range(pa, pa + ps, 4):
            mn, ops = insns.get(ia, ("?", ""))
            m = re.search(r"0x([0-9a-f]+)", ops)
            if m and mn[0] in "jb" and a <= int(m.group(1), 16) < end:
                reasons.append(f"prev fn {pn} jumps in")
                break
    mn, ops = insns.get(end - 8, ("?", ""))
    if mn == "jr" and "ra" in ops:
        reasons.append("ends with jr ra")
    mn, ops = insns.get(a, ("?", ""))
    if mn == "addiu" and ops.startswith("sp,sp,-"):
        reasons.append("starts w/ prologue")
    return reasons, (prev[-1] if prev else None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true", help="include the SDK region")
    args = ap.parse_args()
    c = triage.load()
    insns = c["insns"]
    hi = triage.TEXT_END if args.all else triage.SDK_START
    funcs = sorted(f for f in c["funcs"] if triage.TEXT_START <= f[0] < hi)
    if not funcs:
        sys.exit("coverage: no functions in range")
    dsym = data_syms()

    # Text starts at the first known function: everything below it is the
    # leading .rodata/.data block (strings), which splat already types.
    lo = funcs[0][0]
    holes, cur = [], lo
    for a, s, _ in funcs:
        if a > cur:
            holes.append((cur, a - cur))
        cur = max(cur, a + s)
    if cur < hi:
        holes.append((cur, hi - cur))

    print(f"text considered: 0x{lo:08x}..0x{hi:08x} ({len(funcs)} functions)\n")
    print(f"{'addr':<12} {'bytes':>6}  after                        why")
    total = 0
    for a, ln in sorted(holes, key=lambda h: -h[1]):
        if ln <= ALIGN:
            continue
        if any(x in dsym for x in range(a, a + ln, 4)):
            continue  # named DATA lives here — not code
        total += ln
        reasons, prev = classify(a, a + ln, funcs, insns)
        pn = prev[2] if prev else "-"
        print(f"0x{a:08x} {ln:>6}  {pn:<28} {', '.join(reasons) or 'unknown — inspect'}")
    print(f"\n{total} bytes of code claimed by no function.")
    if total:
        print("A hole whose reason names the previous function is an UNDER-SIZED\n"
              "functions.tsv entry. Carve that function with its TRUE size\n"
              "(`reverse.py <Name> --size 0x<n>`) and fix it in Ghidra, or its\n"
              "`.c` can never byte-match.")


if __name__ == "__main__":
    main()
