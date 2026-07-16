#!/usr/bin/env python3
"""Register-mention histogram: target vs our build, per register.

The recommended FIRST move on any big Ghidra-derived function. Three lanes
hand-rolled this with ad-hoc greps before it became a tool; it answers two
questions in one second that otherwise cost a round each:

* **Is there a mega-pseudo?** Ghidra reuses one variable for a dozen unrelated
  jobs. Each C local is ONE pseudo and one pseudo gets ONE hard register for ALL
  its fragments, so a conflict anywhere exiles every use. A caller-saved register
  the DRAFT mentions far more than the target is that tell — splitting the
  variable per site was worth 140 bytes on FUN_80057b80 (`iVar3`: 70 mentions vs
  the target's 5).
* **Is the decomposition already exhausted?** Read the delta SUM. Deltas that sum
  to ZERO and sit only in the argument registers are the signature of pure
  renames of identical instructions: no splitting lever remains, and the residual
  is allocation/scheduling (StageEndScreen: s0-s8 and v0 exact, args +5/+3/+2/-4/-6,
  sum 0). A non-zero sum means real structural divergence — go find it.

Also useful: a register the target mentions N times has N-2 body refs (the
prologue `sw` and epilogue `lw` are the other two), which is the cheapest
possible check on any "the original's X carries K refs" claim.

  tools/reghist.py <Name>          histogram, differences first
  tools/reghist.py <Name> --all    every register, including the equal ones
  tools/reghist.py <Name> -n       skip ./Build (reads the image on disk)

The candidate extent comes from the link map, so a stale Ghidra size cannot
truncate the view. Run inside the nix devShell.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

import asmdiff
import matchdiff
from matchlock import MatchToolBusy, matching_tool_lock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

# $at/$k0/$k1/$gp/$sp/$zero are not allocated by the register allocator, so a
# difference there is never a decomposition lever; keep them out of the sum.
ALLOCATABLE = (
    ["v0", "v1"]
    + [f"a{i}" for i in range(4)]
    + [f"t{i}" for i in range(10)]
    + [f"s{i}" for i in range(9)]
    + ["ra", "fp"]
)
# objdump prints MIPS registers WITHOUT the `$` (`lw v0,-2536(v0)`), while
# hand-written asm and RTL dumps use it. Accept both, and require word
# boundaries so a hex literal (`0x800a0`) cannot register as an `a0` mention.
REG_RE = re.compile(r"\b\$?(" + "|".join(ALLOCATABLE) + r")\b")


def mentions(disassembly):
    """Count register mentions across an instruction listing."""
    counts = collections.Counter()
    for _addr, text in disassembly:
        # Strip the objdump comment tail so a symbolic branch target's name
        # cannot be miscounted as a register mention.
        body = text.split(";")[0].split("<")[0]
        for reg in REG_RE.findall(body):
            counts[reg] += 1
    return counts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("-n", "--no-build", action="store_true")
    ap.add_argument("--all", action="store_true",
                    help="show every register, not just the differing ones")
    args = ap.parse_args()
    name = args.name

    if not args.no_build:
        if subprocess.run(["./Build"]).returncode:
            sys.exit("reghist: build failed")
    blocker = asmdiff.candidate_artifact_error(name)
    if blocker:
        sys.exit(blocker)

    addr, size = matchdiff.symbol_slot(name)
    target = asmdiff.dis(asmdiff.ORIG, addr, size)
    candidate = asmdiff.candidate_disassembly(name, addr, size)

    t_counts, o_counts = mentions(target), mentions(candidate)
    rows = []
    for reg in ALLOCATABLE:
        t, o = t_counts.get(reg, 0), o_counts.get(reg, 0)
        if t or o:
            rows.append((reg, t, o, o - t))

    shown = [r for r in rows if r[3]] or []
    total = sum(r[3] for r in rows)
    print(f"{name} @ {addr:#x}: register mentions "
          f"(target {len(target)} insns, ours {len(candidate)})")
    print(f"{'reg':>5} {'target':>7} {'ours':>7} {'delta':>7}")
    for reg, t, o, d in (rows if args.all else (shown or rows)):
        flag = "" if not d else ("  <-- draft-heavy" if d > 0 else "  <-- target-heavy")
        print(f"{reg:>5} {t:>7} {o:>7} {d:>+7}{flag}")

    print()
    if not shown:
        print("reghist: every allocatable register matches the target exactly.")
        print("         No mega-pseudo and no splitting lever — the residual is "
              "allocation/scheduling, not decomposition.")
        return 0
    print(f"reghist: {len(shown)} register(s) differ; delta sum {total:+d}")
    if total == 0:
        print("         Sum ZERO = pure renames of identical instructions: the "
              "variable decomposition already matches the target.")
        print("         Do not hunt a mega-pseudo here; the residual is "
              "allocation/scheduling.")
    else:
        print("         Non-zero sum = real structural divergence (instructions "
              "our draft has and the target does not, or vice versa).")
    worst = max(shown, key=lambda r: r[3])
    if worst[3] >= 10:
        print(f"         ${worst[0]} is draft-heavy by {worst[3]} — suspect a "
              f"MEGA-PSEUDO (a Ghidra variable doing several unrelated jobs);")
        print("         split it per site. See docs/matching-cookbook.md.")
    return 0


if __name__ == "__main__":
    target = next((x for x in sys.argv[1:] if not x.startswith("-")), "-")
    try:
        with matching_tool_lock("reghist", target):
            sys.exit(main())
    except MatchToolBusy as e:
        sys.exit(f"reghist: {e}")
