#!/usr/bin/env python3
"""Decomp progress report: how much is matched, split game-code vs SDK.

  tools/progress.py            human-readable report
  tools/progress.py --json     machine-readable (frogress-style categories)

"Matched" = a function whose src/main.exe/<Name>.c exists and contains no
INCLUDE_ASM (same definition findsimilar/coddog-elf use). Function inventory
comes from the Ghidra export. The game/SDK boundary is a provisional constant
(SDK_START): everything above it is statically-linked PSY-Q library code
(LIBSND/LIBSPU/LIBCD/LIBCARD... — _SsVm*, note2pitch, etc. start at
0x80061000); the goal metric is GAME-code progress. Refine the boundary (or
replace it with per-function classification via coddog compare-raw against
converted PSY-Q .OBJs) as library identification improves.
"""
import argparse, json, os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
SDK_START = 0x80061000  # provisional game/SDK boundary — see module docstring
TSV = ".shake/ghidra-export/functions.tsv"
SRC = "src/main.exe"
SYMBOLS = "config/symbols.main.exe.txt"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    funcs = []
    for line in open(TSV):
        p = line.rstrip("\n").split("\t")
        if len(p) == 3 and re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", p[2]):
            a, s, n = int(p[0], 16), int(p[1]), p[2]
            if TEXT_START <= a < TEXT_END:
                funcs.append((a, s, n))

    # matched .c filenames -> addresses via config/symbols (Ghidra names for the
    # same function can differ, e.g. initialise_font vs its FUN_ name)
    sym_addr = {}
    for line in open(SYMBOLS):
        m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            sym_addr[m.group(1)] = int(m.group(2), 16)
    matched_addrs = set()
    matched = set()
    for f in os.listdir(SRC):
        if f.endswith(".c") and not re.search(r"^\s*INCLUDE_ASM", open(os.path.join(SRC, f)).read(), re.M):
            matched.add(f[:-2])
            if f[:-2] in sym_addr:
                matched_addrs.add(sym_addr[f[:-2]])

    def bucket(pred):
        fs = [(a, s, n) for a, s, n in funcs if pred(a)]
        m = [(a, s, n) for a, s, n in fs if a in matched_addrs]
        return {"fns": len(fs), "fns_matched": len(m),
                "bytes": sum(s for _, s, _ in fs),
                "bytes_matched": sum(s for _, s, _ in m)}

    game = bucket(lambda a: a < SDK_START)
    sdk = bucket(lambda a: a >= SDK_START)
    total = bucket(lambda a: True)

    if args.json:
        print(json.dumps({"game": game, "sdk": sdk, "total": total,
                          "matched_names": sorted(matched)}, indent=2))
        return

    def line(label, b):
        fp = 100.0 * b["fns_matched"] / b["fns"] if b["fns"] else 0
        bp = 100.0 * b["bytes_matched"] / b["bytes"] if b["bytes"] else 0
        print(f"{label:18} {b['fns_matched']:5}/{b['fns']:<5} functions "
              f"({fp:5.2f}%)   {b['bytes_matched']:7}/{b['bytes']:<7} bytes ({bp:5.2f}%)")

    print(f"Tenchu decomp progress (matched = real C, no INCLUDE_ASM)")
    line("game code", game)
    line(f"SDK (>{SDK_START:#x})", sdk)
    line("total", total)
    if matched:
        print("matched:", ", ".join(sorted(matched)))


if __name__ == "__main__":
    main()
