#!/usr/bin/env python3
"""Callers and callees of a function — context for prototypes and externs.

When you pick a target, two things help before drafting: who CALLS it (their
call sites pin the parameter types / count) and what it CALLS (which callees
are already matched, which you'll need `extern` prototypes for). This reports
both, resolved against the ORIGINAL image (so it doesn't depend on our build),
with a matched/unmatched tag from src/main.exe.

  tools/xref.py <Name>          callers + callees
  tools/xref.py <Name> --callers / --callees   just one side

Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
FILE_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
SRC = "src/main.exe"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"


def functions():
    """sorted [(start, end, name)] inside the text segment."""
    out = []
    for line in open(TSV):
        p = line.rstrip("\n").split("\t")
        if len(p) == 3:
            a, s, n = int(p[0], 16), int(p[1]), p[2]
            if TEXT_START <= a < TEXT_END and s >= 8:
                out.append((a, a + s, n))
    out.sort()
    return out


def matched_names():
    names = set()
    for f in os.listdir(SRC):
        if f.endswith(".c") and not re.search(
                r"^\s*INCLUDE_ASM", open(os.path.join(SRC, f)).read(), re.M):
            names.add(f[:-2])
    return names


def all_jals():
    """[(insn_addr, target_addr)] for every `jal` in the text segment."""
    # The EXE has a FILE_OFF (0x800) header before the text, so file offset F is
    # real vram TEXT_START + (F - FILE_OFF): adjust so file 0x800 -> TEXT_START.
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(TEXT_START - FILE_OFF), ORIG],
        capture_output=True, text=True).stdout
    r = []
    pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \tjal\s+0x([0-9a-f]+)")
    for line in out.splitlines():
        m = pat.match(line)
        if m:
            r.append((int(m.group(1), 16), int(m.group(2), 16)))
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--callers", action="store_true")
    ap.add_argument("--callees", action="store_true")
    args = ap.parse_args()
    both = not (args.callers or args.callees)

    funcs = functions()
    byname = {n: (a, e) for a, e, n in funcs}
    if args.name not in byname:
        sys.exit(f"xref: {args.name} not in {TSV}")
    fstart, fend = byname[args.name]
    matched = matched_names()

    def containing(addr):
        for a, e, n in funcs:
            if a <= addr < e:
                return n
        return None

    def tag(n):
        return "matched" if n in matched else "asm"

    jals = all_jals()

    if both or args.callees:
        seen = {}
        for ia, ta in jals:
            if fstart <= ia < fend:
                seen.setdefault(ta, 0)
                seen[ta] += 1
        callee = [(containing(ta) or f"{ta:#x}", ta, c) for ta, c in seen.items()]
        callee.sort(key=lambda x: (x[0] is None, x[0]))
        print(f"{args.name} CALLS ({len(callee)}):")
        for n, ta, c in callee:
            x = f" x{c}" if c > 1 else ""
            print(f"  {n:32} {ta:#010x} [{tag(n)}]{x}")

    if both or args.callers:
        callers = {}
        for ia, ta in jals:
            if ta == fstart:
                cn = containing(ia)
                if cn:
                    callers.setdefault(cn, 0)
                    callers[cn] += 1
        rows = sorted(callers.items())
        print(f"{args.name} CALLED BY ({len(rows)}):")
        if not rows:
            print("  (no direct jal callers — indirect/proc-pointer, or a root)")
        for n, c in rows:
            x = f" x{c}" if c > 1 else ""
            print(f"  {n:32} [{tag(n)}]{x}")


if __name__ == "__main__":
    main()
