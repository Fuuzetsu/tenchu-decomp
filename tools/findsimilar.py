#!/usr/bin/env python3
"""Rank functions by assembly similarity — pick references and next targets.

Two uses (docs/matching-cookbook.md):

  tools/findsimilar.py <FuncName>     which MATCHED functions look most like
                                      this one — read those first, they're the
                                      worked examples to imitate
  tools/findsimilar.py --targets      which UNMATCHED functions are closest to
                                      the matched corpus — easy next targets
                                      (sorted best-first, smallest-first)

Similarity = Jaccard overlap of mnemonic 4-grams (operands ignored — register
allocation noise would swamp the structure). The function list comes from the
Ghidra export (.shake/ghidra-export/functions.tsv); bytes from the original
image, disassembled in one objdump pass. Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
FILE_TEXT_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
SRC = "src/main.exe"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"
N = 4  # n-gram size


def load_functions():
    """[(addr, size, name)] for functions inside the text segment."""
    out = []
    for line in open(TSV):
        parts = line.rstrip("\n").split("\t")
        if len(parts) == 3:
            addr, size, name = int(parts[0], 16), int(parts[1]), parts[2]
            if TEXT_START <= addr < TEXT_END and size >= 8:
                out.append((addr, size, name))
    return out


def matched_names():
    """Functions with real C (no INCLUDE_ASM) in src/main.exe/."""
    names = set()
    for f in os.listdir(SRC):
        if f.endswith(".c"):
            if not re.search(r"^\s*INCLUDE_ASM", open(os.path.join(SRC, f)).read(), re.M):
                names.add(f[:-2])
    return names


def all_mnemonics():
    """{addr: mnemonic} for the whole text segment, one objdump pass."""
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(TEXT_START), ORIG],
        capture_output=True, text=True).stdout
    mn = {}
    pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \t(\S+)")
    for line in out.splitlines():
        m = pat.match(line)
        if m:
            a = int(m.group(1), 16)
            if a >= TEXT_START:
                mn[a] = m.group(2)
    return mn


def ngrams(mn, addr, size):
    seq = [mn.get(a, "?") for a in range(addr, addr + size, 4)]
    return {tuple(seq[i:i + N]) for i in range(len(seq) - N + 1)}


def jaccard(a, b):
    if not a or not b:
        return 0.0
    return len(a & b) / len(a | b)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name", nargs="?")
    ap.add_argument("--targets", action="store_true",
                    help="rank unmatched functions by closeness to the matched corpus")
    ap.add_argument("--top", type=int, default=15)
    ap.add_argument("--max-size", type=int, default=0x800,
                    help="--targets: ignore functions bigger than this (bytes)")
    args = ap.parse_args()

    funcs = load_functions()
    byname = {n: (a, s) for a, s, n in funcs}
    matched = matched_names() & set(byname)
    if not matched:
        sys.exit("findsimilar: no matched functions found in src/main.exe")
    mn = all_mnemonics()
    mgrams = {m: ngrams(mn, *byname[m]) for m in matched}

    if args.targets:
        rows = []
        for a, s, n in funcs:
            if n in matched or s > args.max_size:
                continue
            g = ngrams(mn, a, s)
            best, bestm = 0.0, ""
            for m, gm in mgrams.items():
                j = jaccard(g, gm)
                if j > best:
                    best, bestm = j, m
            rows.append((best, s, n, bestm))
        rows.sort(key=lambda r: (-r[0], r[1]))
        print(f"{'sim':>5} {'size':>6}  {'function':32} nearest matched")
        for best, s, n, bestm in rows[:args.top]:
            print(f"{best:5.2f} {s:6}  {n:32} {bestm}")
        return

    if not args.name:
        sys.exit("findsimilar: give a function name or --targets")
    if args.name not in byname:
        sys.exit(f"findsimilar: {args.name} not in {TSV}")
    g = ngrams(mn, *byname[args.name])
    rows = sorted(((jaccard(g, gm), m) for m, gm in mgrams.items()), reverse=True)
    print(f"{args.name} ({byname[args.name][1]} bytes) vs matched corpus:")
    for j, m in rows[:args.top]:
        print(f"{j:5.2f}  {m} ({byname[m][1]} bytes) — src/main.exe/{m}.c")


if __name__ == "__main__":
    main()
