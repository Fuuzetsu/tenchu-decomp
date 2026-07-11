#!/usr/bin/env python3
"""Draft a function from its nearest MATCHED sibling — asm diff + the sibling's C.

The single most decisive move when drafting a function in a family (the ProcItem*,
Act*, Draw*, Think* twins) is: find the already-matched sibling whose asm is the
same shape, read its C, and change only the instructions that differ. Agents were
doing this by hand — `findsimilar` to pick the sibling, then eyeballing two `.s`
files. This does all of it in one shot:

    tools/siblingdiff.py <Target>            # auto-pick the nearest matched sibling
    tools/siblingdiff.py <Target> <Sibling>  # force a specific sibling
    tools/siblingdiff.py <Target> --full     # whole aligned listing, not just diffs
    tools/siblingdiff.py <Target> --no-c     # skip printing the sibling's C

It disassembles BOTH functions straight from the original image (same objdump pass
and function list as findsimilar), normalizes intra-function branch/jump targets to
labels relative to the function start (so the same control-flow shape at two
different addresses ALIGNS instead of reading as all-different), then prints:
  1. the ranked sibling candidates (so you can retry with a different one),
  2. a compact SequenceMatcher diff — `ref:` = sibling, `tgt:` = target — with runs
     of identical instructions collapsed to `… N identical`,
  3. the sibling's matched C (src/main.exe/<Sibling>.c) verbatim, ready to transcribe.

Inter-function `jal`/`j` targets are LEFT ABSOLUTE on purpose: a call to the SAME
callee then reads identical in both (aligns), and a call to a DIFFERENT callee reads
as a real diff (it is one). Only branches landing inside the function window are
relativized. Register allocation still shows through — that's deliberate; a diff in
just the registers usually means the C is right and the residual is an allocation
tie for rtldump/permute, not a source change. Run inside the nix devShell.
"""
from __future__ import annotations

import argparse
import difflib
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
SRC = "src/main.exe"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"
N = 4  # mnemonic n-gram size, matches findsimilar

BRANCH = re.compile(r"^(b|beq|bne|beqz|bnez|bgez|bgtz|blez|bltz|bgezal|bltzal"
                    r"|bc1t|bc1f|j)\w*$")
TEXTADDR = re.compile(r"\b(?:0x)?(80[0-9a-f]{6})\b")  # objdump prefixes targets 0x


def load_functions():
    """[(addr, size, name)] inside text, real-name-preferred (mirrors findsimilar)."""
    symname = {}
    for line in open(SYMBOLS):
        m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            symname[int(m.group(2), 16)] = m.group(1)
    out = []
    for line in open(TSV):
        parts = line.rstrip("\n").split("\t")
        if len(parts) == 3:
            addr, size, name = int(parts[0], 16), int(parts[1]), parts[2]
            if TEXT_START <= addr < TEXT_END and size >= 8:
                out.append((addr, size, symname.get(addr, name)))
    return out


def matched_names():
    """Functions with real C (no INCLUDE_ASM) in src/main.exe/."""
    names = set()
    for f in os.listdir(SRC):
        if f.endswith(".c") and not re.search(
                r"^\s*INCLUDE_ASM", open(os.path.join(SRC, f)).read(), re.M):
            names.add(f[:-2])
    return names


def all_insns():
    """{addr: (mnemonic, operands)} for the whole text segment, one objdump pass."""
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(TEXT_START - FILE_TEXT_OFF), ORIG],
        capture_output=True, text=True).stdout
    insns = {}
    pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \t(\S+)(?:\s+(.*))?$")
    for line in out.splitlines():
        m = pat.match(line)
        if m:
            a = int(m.group(1), 16)
            if a >= TEXT_START:
                insns[a] = (m.group(2), (m.group(3) or "").strip())
    return insns


def ngrams(insns, addr, size):
    seq = [insns.get(a, ("?", ""))[0] for a in range(addr, addr + size, 4)]
    return {tuple(seq[i:i + N]) for i in range(len(seq) - N + 1)}


def jaccard(a, b):
    return len(a & b) / len(a | b) if a and b else 0.0


def norm_line(insns, a, start, end):
    """One normalized instruction string; intra-window targets -> L<off>."""
    mnem, ops = insns.get(a, ("?", ""))

    def repl(m):
        t = int(m.group(1), 16)
        if BRANCH.match(mnem) and start <= t < end:
            return f"L{t - start:x}"          # relative label, aligns across addrs
        return m.group(0)                      # inter-function target: keep absolute

    return f"{mnem} {TEXTADDR.sub(repl, ops)}".rstrip()


def disasm(insns, addr, size):
    end = addr + size
    return [norm_line(insns, a, addr, end) for a in range(addr, end, 4)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target")
    ap.add_argument("sibling", nargs="?", help="force this matched sibling")
    ap.add_argument("--top", type=int, default=5, help="sibling candidates to list")
    ap.add_argument("--full", action="store_true", help="whole aligned listing")
    ap.add_argument("--context", type=int, default=2, help="context insns per hunk")
    ap.add_argument("--no-c", action="store_true", help="don't print the sibling C")
    args = ap.parse_args()

    funcs = load_functions()
    byname = {n: (a, s) for a, s, n in funcs}
    if args.target not in byname:
        sys.exit(f"siblingdiff: {args.target} not in {TSV}")
    matched = matched_names() & set(byname)
    matched.discard(args.target)
    if not matched:
        sys.exit("siblingdiff: no matched functions to compare against")

    insns = all_insns()
    ta, tsz = byname[args.target]
    tg = ngrams(insns, ta, tsz)
    ranked = sorted(((jaccard(tg, ngrams(insns, *byname[m])), m) for m in matched),
                    reverse=True)

    if args.sibling:
        if args.sibling not in matched:
            sys.exit(f"siblingdiff: {args.sibling} is not a matched sibling")
        sib = args.sibling
    else:
        sib = ranked[0][1]

    sa, ssz = byname[sib]
    print(f"target  {args.target}  ({tsz} bytes)")
    print(f"sibling {sib}  ({ssz} bytes)  sim {dict((m, j) for j, m in ranked)[sib]:.2f}"
          f"  -> src/main.exe/{sib}.c")
    print("candidates: " + ", ".join(f"{m}({j:.2f})" for j, m in ranked[:args.top]))
    print()

    ref = disasm(insns, sa, ssz)
    tgt = disasm(insns, ta, tsz)
    sm = difflib.SequenceMatcher(a=ref, b=tgt, autojunk=False)
    same = sum(b - a for tag, a, b, _, _ in sm.get_opcodes() if tag == "equal")
    print(f"asm: {same}/{max(len(ref), len(tgt))} instructions identical after "
          f"normalization  (ratio {sm.ratio():.2f})")
    print("  ref: = sibling   tgt: = target   Lxx = intra-function label\n")

    if args.full:
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == "equal":
                for k in range(i1, i2):
                    print(f"    {ref[k]}")
            else:
                for k in range(i1, i2):
                    print(f"  ref: {ref[k]}")
                for k in range(j1, j2):
                    print(f"  tgt: {tgt[k]}")
    else:
        c = args.context
        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag == "equal":
                n = i2 - i1
                if n > 2 * c + 1:
                    for k in range(i1, i1 + c):
                        print(f"    {ref[k]}")
                    print(f"    … {n - 2 * c} identical")
                    for k in range(i2 - c, i2):
                        print(f"    {ref[k]}")
                else:
                    for k in range(i1, i2):
                        print(f"    {ref[k]}")
            else:
                for k in range(i1, i2):
                    print(f"  ref: {ref[k]}")
                for k in range(j1, j2):
                    print(f"  tgt: {tgt[k]}")

    if not args.no_c:
        p = os.path.join(SRC, sib + ".c")
        if os.path.exists(p):
            print(f"\n===== src/main.exe/{sib}.c (transcribe, then edit the diffs) =====")
            sys.stdout.write(open(p).read())


if __name__ == "__main__":
    main()
