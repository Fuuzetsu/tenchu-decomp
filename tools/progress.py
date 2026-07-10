#!/usr/bin/env python3
"""Decomp progress report: how much is matched, split game-code vs SDK.

  tools/progress.py            human-readable report
  tools/progress.py --json     machine-readable (frogress-style categories)

"Matched" = a function that is CARVED (has a `c` subsegment, so its .c is
actually linked) AND whose src/main.exe/<Name>.c contains no INCLUDE_ASM. The
carve check matters: an un-carved function's bytes come from a raw data blob, so
a bogus .c sitting next to it changes nothing and used to be counted as matched
(five were). Function inventory
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
SDK_START = 0x80060000  # game/SDK boundary: 0x80060xxx is the CRT/libcd/libapi
# block (Exec/__main/Cd*/PC*/EVENT_OBJ*/DeliverEvent/_SN*), a differently-compiled
# PsyQ SDK object that won't byte-match our cc1 — see module docstring.
TSV = ".shake/ghidra-export/functions.tsv"
SRC = "src/main.exe"
SYMBOLS = "config/symbols.main.exe.txt"
YAML = "config/splat.main.exe.yaml"


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
    # A `.c` only reaches the image if splat carves a `c` subsegment for it.
    carved = set(re.findall(r"^\s+- \[0x[0-9A-Fa-f]+,\s*c,\s*(\S+)\]",
                            open(YAML).read(), re.M))
    matched_addrs = set()
    matched = set()
    orphans = []
    for f in sorted(os.listdir(SRC)):
        if not f.endswith(".c"):
            continue
        if re.search(r"^\s*INCLUDE_ASM", open(os.path.join(SRC, f)).read(), re.M):
            continue
        name = f[:-2]
        if name not in carved:
            orphans.append(name)     # pure C but never linked — NOT matched
            continue
        matched.add(name)
        if name in sym_addr:
            matched_addrs.add(sym_addr[name])

    if orphans:
        print(f"WARNING: {len(orphans)} pure-C file(s) are NOT carved, so they are "
              f"never linked and are NOT counted as matched:")
        for n in orphans:
            print(f"  {n}")
        print()

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

    print(f"Tenchu decomp progress (matched = carved + real C, no INCLUDE_ASM)")
    line("game code", game)
    line(f"SDK (>{SDK_START:#x})", sdk)
    line("total", total)
    if matched:
        print("matched:", ", ".join(sorted(matched)))


if __name__ == "__main__":
    main()
