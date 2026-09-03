#!/usr/bin/env python3
"""Enforce one name per address in config/symbols.main.exe.txt.

splat >= 0.4x rejects a symbol_addrs file where two names map to the same vram
unless each is disambiguated with a segment or rom address — which our file
CANNOT express, because it doubles as an `ld` linker script and therefore has no
comment syntax (`//` is a hard parse error there).

Nearly every duplicate is a Ghidra `switchD_<a>__caseD_<n>` alias: several switch
cases branching to one shared block, so Ghidra names the block once per case. A
handful are SDK objects co-located with a case label. Dropping the aliases is
safe as long as nothing references them, so we keep whichever name our sources
actually use.

`tools/import_symbols.py` only RENAMES symbols, never adds, so this stays fixed
once applied. Re-run it after any fresh Ghidra symbol import.

  tools/dedupe-symbols.py            rewrite the file, report what was dropped
  tools/dedupe-symbols.py --check    exit 1 if any address has two names
"""
import argparse, collections, glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYMS = os.path.join(ROOT, "config", "symbols.main.exe.txt")
SYM = re.compile(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")


def referenced_names():
    """Every identifier appearing in our sources — a name used there must stay."""
    out = set()
    for pat in ("src/main.exe/*.c", "src/main.exe/*.C", "src/main.exe/*.h"):
        for f in glob.glob(os.path.join(ROOT, pat)):
            out |= set(re.findall(r"[A-Za-z_$][\w$]*", open(f).read()))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="don't rewrite; exit 1 if duplicates remain")
    args = ap.parse_args()

    lines = open(SYMS).read().splitlines(keepends=True)
    byaddr = collections.defaultdict(list)
    for i, line in enumerate(lines):
        m = SYM.match(line.strip())
        if m:
            byaddr[int(m.group(2), 16)].append((i, m.group(1)))

    dup = {a: e for a, e in byaddr.items() if len(e) > 1}
    if args.check:
        if dup:
            print(f"dedupe-symbols: {len(dup)} address(es) still have >1 name "
                  f"({sum(len(e) - 1 for e in dup.values())} extra); splat will "
                  f"refuse the file. Run tools/dedupe-symbols.py.", file=sys.stderr)
            return 1
        print(f"dedupe-symbols: OK — {len(byaddr)} addresses, all unique")
        return 0

    if not dup:
        print("dedupe-symbols: nothing to do")
        return 0

    used = referenced_names()
    drop, kept_used = set(), []
    for a, entries in dup.items():
        keep = next((e for e in entries if e[1] in used), entries[0])
        if keep[1] in used:
            kept_used.append(keep[1])
        drop |= {i for i, _ in entries if i != keep[0]}

    open(SYMS, "w").writelines(l for i, l in enumerate(lines) if i not in drop)
    print(f"dedupe-symbols: {len(dup)} duplicated address(es); dropped {len(drop)} "
          f"alias name(s); kept {len(kept_used)} that our sources reference "
          f"({', '.join(sorted(kept_used)) or 'none'})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
