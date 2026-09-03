#!/usr/bin/env python3
"""Just SHOW me the disassembly — target, ours, or both side by side.

Every other tool here answers a question. This one answers none: it prints the
instructions. That turned out to be a real gap — `matchdiff` and `asmdiff` only
emit DIFFERING lines, so a cluster that is not currently differing (or sits outside
the diff's window) has no tool that will show it at all. A lane reasoning about
AddEnemy's cluster at 0x8005b8dc had to hand-derive the vaddr->file-offset delta
from the carved `.s` header and drive `dd` + `objdump -b binary --adjust-vma` by
hand. That is this tool.

  tools/tdis.py <Name>                  the TARGET's disassembly
  tools/tdis.py <Name> --ours           our build's (needs a current ./Build)
  tools/tdis.py <Name> --both           side by side, address-aligned
  tools/tdis.py <Name> --range 0x8005b8c0:0x8005b900     just that window
  tools/tdis.py <Name> --context 0x8005b8dc:6            6 insns either side
  tools/tdis.py <Name> -n               skip ./Build (with --ours/--both)

Note plain `objdump` on PATH is the HOST (x86) one — this uses the MIPS
cross-objdump, like the rest of the tooling. Run inside the nix devShell.
"""
import argparse
import os
import sys

import asmdiff
import matchdiff
from matchlock import MatchToolBusy, matching_tool_lock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)


def parse_window(spec, addr, size):
    """Parse --range LO:HI or --context ADDR:N into an (lo, hi) address pair."""
    lo, _, hi = spec.partition(":")
    if not hi:
        raise ValueError("expected LO:HI (--range) or ADDR:N (--context)")
    return int(lo, 0), int(hi, 0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--ours", action="store_true", help="our build instead of the target")
    ap.add_argument("--both", action="store_true", help="target and ours side by side")
    ap.add_argument("--range", help="LO:HI — only this address window")
    ap.add_argument("--context", help="ADDR:N — N instructions either side of ADDR")
    ap.add_argument("-n", "--no-build", action="store_true")
    args = ap.parse_args()

    need_ours = args.ours or args.both
    if need_ours and not args.no_build:
        env = dict(os.environ)
        srcp = matchdiff.source_path(args.name)
        if os.path.exists(srcp) and "ifndef NON_MATCHING" in open(srcp).read():
            env["NON_MATCHING"] = args.name
        import subprocess
        r = subprocess.run(["./Build"], stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, env=env)
        if r.returncode:
            sys.exit(f"tdis: ./Build failed (rc={r.returncode})")

    addr, size = matchdiff.symbol_slot(args.name)
    target = asmdiff.dis(asmdiff.ORIG, addr, size)
    ours = asmdiff.candidate_disassembly(args.name, addr, size) if need_ours else []

    lo, hi = addr, addr + size
    if args.range:
        lo, hi = parse_window(args.range, addr, size)
    elif args.context:
        centre, _, n = args.context.partition(":")
        centre, n = int(centre, 0), int(n or 6, 0)
        lo, hi = centre - n * 4, centre + (n + 1) * 4

    def window(rows):
        return [(a, t) for a, t in rows if lo <= a < hi]

    t_rows, o_rows = window(target), window(ours)
    print(f"{args.name} @ {addr:#x}, {size} bytes"
          f"{f' — window {lo:#x}..{hi:#x}' if (args.range or args.context) else ''}")

    if args.both:
        # Address-aligned: our build lands each insn at the same vaddr as the
        # target unless the length differs, so a plain address join is honest and
        # a gap is itself information.
        by_addr = dict(o_rows)
        print(f"{'addr':>10}  {'TARGET':<30} {'OURS':<30}")
        for a, t in t_rows:
            o = by_addr.get(a, "")
            mark = " " if o == t else "|"
            print(f"{a:#010x} {mark} {t:<30} {o:<30}")
        extra = [(a, o) for a, o in o_rows if a not in dict(t_rows)]
        for a, o in extra:
            print(f"{a:#010x} > {'':<30} {o:<30}")
        return 0

    for a, t in (o_rows if args.ours else t_rows):
        print(f"{a:#010x}  {t}")
    return 0


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("tdis", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"tdis: {e}")
    except (ValueError, KeyError) as e:
        sys.exit(f"tdis: {e}")
