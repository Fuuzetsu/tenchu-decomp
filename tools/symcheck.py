#!/usr/bin/env python3
"""Catch auto-named data symbols that link at the wrong address.

splat names an unclaimed data symbol after its own address: `D_80097F6C` lives at
0x80097F6C. That invariant is checkable, and it breaks *silently* in one specific
way we have now hit three times:

When the LAST `INCLUDE_ASM` stub referencing such a symbol is converted to real C,
the raw asm that carried splat's auto-label disappears. splat's generated `-T`
script is processed AFTER `config/symbols.<target>.txt`, so the symbol gets rebound
to whatever address the auto-script now computes -- typically a few bytes off,
because another small object slid into the freed slot. The link SUCCEEDS. Nothing
warns. The image is simply wrong, and `matchdiff` shows a `%hi`/`%lo` or gp-offset
immediate off by a small fixed delta at every use.

The same failure appears when a sibling stub file in the same TU family lacks its
`--gp-extern` entry: maspsx synthesises a stray local placeholder and the linker
silently prefers it (first exposed by the CVA family's once-auto-named
0x80097CC0..0x80097CCC cluster, whose symbols shifted by +4/+0xC).

Both are caught by one rule: **a symbol named D_<HEX> must resolve to 0x<HEX>.**

    tools/symcheck.py                 # check main.exe (default)
    tools/symcheck.py --target menu.exe
    tools/symcheck.py --map path/to.map

Exits non-zero on any drift. Run it after `./Build`; it needs the linker map, which
Build.hs already emits to .shake/build/tenchu/<target>.map.

Fix a reported drift by pinning the true address in `config/symbols.<target>.txt`
(`D_80097F6C = 0x80097F6C;`) and, if the symbol is gp-relative, adding it to the
gp-extern list of EVERY sibling file in its TU -- including ones still on
INCLUDE_ASM. See docs/matching-cookbook.md, "gp vs absolute globals".
"""
from __future__ import annotations

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# ` 0x80097f6c   D_80097F6C` (an object's symbol) or
# ` 0x80097f6c   D_80097F6C = 0x80097f6c` (a linker-script assignment)
SYM = re.compile(r"^\s+0x([0-9a-fA-F]+)\s+(D_[0-9A-Fa-f]{8})(?:\s*=|\s*$)")
AUTO = re.compile(r"^\s+0x([0-9a-fA-F]+)\s+(D_[0-9A-Fa-f]{8})\.NON_MATCHING\s*$")


def check(map_path: str) -> int:
    if not os.path.exists(map_path):
        sys.exit(f"symcheck: no {map_path} -- run ./Build first")

    # A symbol can appear several times (object definition + script assignment).
    # Every occurrence must agree with the name.
    bad: list[tuple[str, int, int]] = []
    shadowed: list[tuple[str, int]] = []
    seen = 0
    for line in open(map_path, errors="replace"):
        m = SYM.match(line)
        if m:
            addr, name = int(m.group(1), 16), m.group(2)
            want = int(name[2:], 16)
            seen += 1
            if addr != want:
                bad.append((name, want, addr))
            continue
        m = AUTO.match(line)
        if m:
            addr, name = int(m.group(1), 16), m.group(2)
            want = int(name[2:], 16)
            if addr != want:
                # splat's auto label drifted; harmless only because a pinned
                # symbol of the same name shadows it. Report as informational.
                shadowed.append((name, addr))

    print(f"symcheck: {seen} D_<addr> symbol occurrences in {map_path}")
    if not bad:
        print("  OK: every D_<addr> symbol resolves to its own address")
        return 0

    print(f"\n  {len(bad)} SYMBOL(S) LINKED AT THE WRONG ADDRESS:")
    for name, want, got in sorted(bad)[:10]:
        print(f"    {name}: want {want:#010x}, linked at {got:#010x} "
              f"(delta {got - want:+d})")
    if len(bad) > 10:
        print(f"    ... and {len(bad) - 10} more")
    if shadowed:
        print(f"\n  {len(shadowed)} auto-label(s) also drifted, e.g. "
              f"{sorted(set(shadowed))[0][0]}.NON_MATCHING at "
              f"{sorted(set(shadowed))[0][1]:#010x} -- same root cause.")

    # A whole region shifting at once means one object grew or moved, not that N
    # symbols each went wrong independently. Measured: dropping ONE function's
    # gp-extern entry moved 388 symbols.
    deltas = {got - want for _, want, got in bad}
    if len(deltas) <= 3:
        print(f"\n  All drift by {sorted(deltas)} bytes -- a whole region shifted. The usual")
        print("  cause is ONE file missing its `--gp-extern` entry, not N broken symbols.")
    print("\n  Fix, in order of likelihood:")
    print("    1. A file that references these gp-relative smalls is missing from")
    print("       maspsxGpExterns in shake/src/Build.hs. Derive it with")
    print("       `tools/maspsxflags.py <Name> --write` (run AFTER a build). It")
    print("       mirrors Build.hs and tools/permute.py. This is the load-bearing")
    print("       fix -- verified by")
    print("       removing CVArun's entry, which moved 388 symbols.")
    print("    2. Only if (1) does not settle it: pin the true address in")
    print(f"       config/symbols.<target>.txt, e.g. {bad[0][0]} = {bad[0][1]:#010x};")
    return 1


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", default="main.exe")
    ap.add_argument("--map", help="explicit path to a linker map")
    args = ap.parse_args()
    m = args.map or os.path.join(".shake", "build", "tenchu", args.target + ".map")
    sys.exit(check(m))


if __name__ == "__main__":
    main()
