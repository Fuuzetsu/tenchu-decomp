#!/usr/bin/env python3
"""Tabulate cc1's OWN scheduler trace: priority, ref_count, and what they mean.

**cc1 already prints this and two rounds were lost to not reading it.** The `.sched`
dump carries `;; insn[NNNN]: priority = P, ref_count = R` lines (`sched.c:3686`) —
250 of them on AddEnemy. A round hand-derived a priority from the RTL, read the
`ref_count` column as if it were priority, and concluded an insn "heads a ~7-deep
chain" and therefore sits at the ceiling. It sits at the FLOOR, cc1 said so verbatim,
and the wrong reading was then folded into the cookbook and briefed to the next round.

**The two columns are not the same metric and one is not the other's inverse:**

* `priority` is **DEPTH-FROM-TOP**. `priority()` (`sched.c:1452`) initialises
  `max_priority = 1` and raises it only by walking **LOG_LINKS** — which are
  *producers*. So an insn with `LOG_LINKS (nil)` has priority **exactly 1**,
  unconditionally, and MIPS defines no `ADJUST_PRIORITY`. Priorities that INCREASE
  as you walk DOWN a chain are depth; a height metric would run the other way.
* `ref_count` counts **consumers** (`INSN_DEPEND`). "Heads a long chain" describes
  THIS column. It is not priority and does not feed it.

So "attack the insn's height" is asking for what the code forbids: nothing can lift a
`LOG_LINKS (nil)` insn off priority 1.

It also prints cc1's `;; ready list at T-N: ... , now ...` trace — the scheduler's
actual decisions. **sched is BACKWARD: T-1 is the LAST slot and is filled FIRST, so an
insn PICKED EARLIER LANDS LATER.** FUN_80057b80's entire 8-byte residual is one such
line:

    ;; ready list at T-20: 6 (1) 4 (1), now 6 4     -> picks 6, so 6 lands last

  tools/schedtrace.py <Name>                the sched1 trace, in dump order
  tools/schedtrace.py <Name> --pass sched2  the post-reload scheduler
  tools/schedtrace.py <Name> --sort         ordered by priority, floor first
  tools/schedtrace.py <Name> --floor        only the priority-1 insns (the ties)

**A PROLOGUE or PARM-COPY ordering question lives ONLY in `--pass sched2`.** The
prologue does not exist until after reload, so parm copies never enter sched1's table
at all. This tool was sched1-only and a lane hand-decoded the raw `.i.sched2` instead
— it called that the single biggest cost of its round.

Run inside the nix devShell.
"""
import argparse
import os
import re
import sys

import rtldump

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TRACE = re.compile(r";;\s*insn\[\s*(\d+)\]:\s*priority\s*=\s*(\d+),\s*"
                   r"ref_count\s*=\s*(\d+)")
# `(insn 1460 1459 1461 (set (reg:SI 88) ...` -- enough to name the insn.
INSN_HEAD = re.compile(r"^\((?:insn|call_insn|jump_insn) (\d+) ")


def insn_text(body):
    """uid -> a one-line gist of the insn's RTL, for the table's last column."""
    out, uid, buf = {}, None, []
    for line in body.splitlines():
        m = INSN_HEAD.match(line)
        if m:
            if uid is not None:
                out[uid] = " ".join(" ".join(buf).split())[:60]
            uid, buf = int(m.group(1)), [line]
        elif uid is not None and len(buf) < 4:
            buf.append(line)
    if uid is not None:
        out[uid] = " ".join(" ".join(buf).split())[:60]
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--sort", action="store_true",
                    help="order by priority (floor first) rather than dump order")
    ap.add_argument("--floor", action="store_true",
                    help="only priority-1 insns — the ones tied at the floor")
    ap.add_argument("--pass", dest="which", choices=["sched", "sched2"],
                    default="sched",
                    help="which scheduler. **A PROLOGUE / PARM-COPY ordering question "
                         "lives ONLY in sched2**: the prologue does not exist until "
                         "after reload, so parm copies never enter sched1's table. "
                         "This tool was sched1-only and a lane had to hand-decode the "
                         "raw .i.sched2 — it called that the single biggest cost of "
                         "its round.")
    args = ap.parse_args()

    path = os.path.join("src", "main.exe", args.name + ".c")
    if not os.path.exists(path):
        sys.exit(f"schedtrace: {path} not found")
    with open(path, errors="replace") as stream:
        draft = "ifndef NON_MATCHING" in stream.read()
    try:
        result = rtldump.compile_rtl(args.name, [args.which], draft=draft)
    except (FileNotFoundError, ValueError, RuntimeError) as e:
        sys.exit(f"schedtrace: {e}")

    # Match the suffix EXACTLY: `"sched" in p` also matches `.sched2`, so the
    # sched1/sched2 choice would silently depend on dump order.
    dump = next((p for p in result["dumps"]
                 if p.rsplit(".", 1)[-1] == args.which), None)
    if dump is None:
        sys.exit("schedtrace: cc1 produced no sched dump")
    body = open(dump, errors="replace").read()

    rows = [(int(u), int(p), int(r)) for u, p, r in TRACE.findall(body)]
    if not rows:
        sys.exit("schedtrace: no `;; insn[N]: priority` lines in the dump — "
                 "cc1 emits them from sched.c:3686; check the pass ran")
    gist = insn_text(body)

    print(f"{args.name}: cc1's own {args.which} trace ({len(rows)} insns) — {dump}")
    print()
    if args.which == "sched2":
        print("  sched2 runs AFTER reload, so this is the ONLY pass that sees the "
              "prologue and its")
        print("  parm copies. Its `;; ready list at T-N: ...` lines are the decision "
              "itself.")
    print("  sched is BACKWARD: T-1 is the LAST slot and is filled FIRST, so an insn "
          "PICKED EARLIER")
    print("  LANDS LATER. Ties break priority DESC -> class DESC -> LUID DESC.")
    print("  priority = DEPTH-FROM-TOP (walks LOG_LINKS = PRODUCERS). "
          "LOG_LINKS (nil) => priority 1, the FLOOR.")
    print("  ref_count = CONSUMERS (INSN_DEPEND). 'heads a long chain' is THIS "
          "column, and it is NOT priority.")
    print("  Ties break priority DESC -> class DESC -> LUID DESC; sched is BACKWARD, "
          "so higher LUID is")
    print("  selected first and lands LAST.")
    print()
    print(f"  {'uid':>6} {'priority':>9} {'ref_count':>10}  insn")
    shown = rows
    if args.floor:
        shown = [r for r in rows if r[1] == 1]
    if args.sort:
        shown = sorted(shown, key=lambda r: (r[1], r[0]))
    for uid, pri, ref in shown:
        flag = "  <- FLOOR" if pri == 1 else ""
        print(f"  {uid:>6} {pri:>9} {ref:>10}  {gist.get(uid, '')}{flag}")

    ready = [ln for ln in body.splitlines() if "ready list at T-" in ln]
    if ready:
        print()
        print(f"  --- ready-list trace ({len(ready)} decisions) — the pick is the "
              "last field ---")
        for ln in ready:
            print("  " + ln.strip())

    floor = [r for r in rows if r[1] == 1]
    print()
    print(f"schedtrace: {len(floor)} insn(s) at priority 1 (the floor). Nothing can "
          "lift them:")
    print("  max_priority = 1 is the initialiser and MIPS defines no "
          "ADJUST_PRIORITY, so an insn")
    print("  with LOG_LINKS (nil) is at 1 unconditionally. If two insns you care "
          "about are BOTH")
    print("  at 1, the order is decided by LUID DESC alone — see "
          "docs/matching-cookbook.md,")
    print('  "priority() is DEPTH-FROM-TOP, not height-to-bottom".')
    return 0


if __name__ == "__main__":
    sys.exit(main())
