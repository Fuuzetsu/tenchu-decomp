#!/usr/bin/env python3
"""Derive (and sync) a function's gp-extern list from its split asm.

ASPSX gp-addresses only the small globals the ORIGINAL TU defined; in our split
`.s` those are exactly the `%gp_rel(SYM)` operands. That set must be listed in
BOTH `shake/src/Build.hs` (maspsxGpExterns) and `tools/permute.py` (GP_EXTERNS),
kept in sync by hand — easy to get wrong or half-update. This reads the pieces
and prints the set; with --write it inserts/updates both files.

  tools/gpsyms.py <Name>            print the %gp_rel symbols
  tools/gpsyms.py <Name> --write    also patch Build.hs + permute.py

Run after the function is split/built (its .s exists under .shake/gen). See
docs/matching-cookbook.md "gp vs absolute globals".
"""
import argparse, glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

ASM = ".shake/gen/main.exe/asm/nonmatchings/{name}"
BUILD_HS = "shake/src/Build.hs"
PERMUTE = "tools/permute.py"


def gp_symbols(name):
    """The %gp_rel(SYM) operands across the function's pieces, first-seen order."""
    d = ASM.format(name=name)
    files = sorted(glob.glob(f"{d}/*.s")) or glob.glob(
        f".shake/gen/main.exe/asm/nonmatchings/*/{name}.s")
    if not files:
        sys.exit(f"gpsyms: no .s for {name} under .shake/gen — split & build it first.")
    seen = []
    for f in files:
        for m in re.finditer(r"%gp_rel\(([A-Za-z_][\w]*)\)", open(f).read()):
            if m.group(1) not in seen:
                seen.append(m.group(1))
    return seen


def patch_build_hs(name, syms):
    s = open(BUILD_HS).read()
    lst = "[" + ", ".join(f'"{x}"' for x in syms) + "]"
    line = f'    syms "{name}" = {lst}'
    pat = re.compile(rf'^    syms "{re.escape(name)}" = \[.*\]$', re.M)
    if pat.search(s):
        s = pat.sub(line, s, count=1)
        action = "updated"
    else:
        # insert right before the `syms _ = []` fallback
        s, n = re.subn(r'^    syms _ = \[\]$', line + "\n    syms _ = []", s,
                       count=1, flags=re.M)
        if n == 0:
            sys.exit("gpsyms: couldn't find `syms _ = []` in Build.hs")
        action = "inserted"
    open(BUILD_HS, "w").write(s)
    return action


def patch_permute(name, syms):
    s = open(PERMUTE).read()
    lst = "[" + ", ".join(f'"{x}"' for x in syms) + "]"
    line = f'    "{name}": {lst},'
    pat = re.compile(rf'^    "{re.escape(name)}": \[.*\],$', re.M)
    if pat.search(s):
        s = pat.sub(line, s, count=1)
        action = "updated"
    else:
        # insert as the last entry of the GP_EXTERNS dict (before its closing })
        m = re.search(r'^GP_EXTERNS = \{$.*?^\}$', s, re.M | re.S)
        if not m:
            sys.exit("gpsyms: couldn't find GP_EXTERNS dict in permute.py")
        block = m.group(0)
        new = block[:-1] + line + "\n}"
        s = s[:m.start()] + new + s[m.end():]
        action = "inserted"
    open(PERMUTE, "w").write(s)
    return action


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--write", action="store_true",
                    help="patch Build.hs maspsxGpExterns + permute.py GP_EXTERNS")
    args = ap.parse_args()

    syms = gp_symbols(args.name)
    if not syms:
        print(f"gpsyms: {args.name} has no %gp_rel symbols — absolute-only, no gp "
              f"entries needed.")
        return
    print(f"gpsyms: {args.name} gp symbols: {', '.join(syms)}")
    if not args.write:
        print("  (run with --write to patch Build.hs + permute.py; verify with "
              "`./Build check` after matching)")
        return
    a1 = patch_build_hs(args.name, syms)
    a2 = patch_permute(args.name, syms)
    print(f"  Build.hs maspsxGpExterns: {a1}")
    print(f"  permute.py GP_EXTERNS:    {a2}")
    # No cache-busting needed: Build.hs exposes the per-file gp flags through a
    # Shake oracle (GpFlags), so editing the list invalidates exactly the
    # affected .s on the next build.
    print("  NOTE: this lists ALL %gp_rel symbols; if any is actually an extern the"
          " original\n  TU only referenced (rare), remove it and keep it absolute — "
          "`./Build check`\n  will tell you (the cookbook's gp section has the rule).")


if __name__ == "__main__":
    main()
