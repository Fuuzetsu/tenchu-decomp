#!/usr/bin/env python3
"""Tabulate cc1's OWN scheduler trace: priority, ref_count, and what they mean.

**cc1 already prints this and two rounds were lost to not reading it.** The `.sched`
dump carries `;; insn[NNNN]: priority = P, ref_count = R` lines (`sched.c:3686`) —
250 of them on AddEnemy. A round hand-derived a priority from the RTL, read the
`ref_count` column as if it were priority, and concluded an insn "heads a ~7-deep
chain" and therefore sits at the ceiling. It sits at the FLOOR, cc1 said so verbatim,
and the wrong reading was then folded into the cookbook and briefed to the next round.

**The two columns are not the same metric and one is not the other's inverse:**

* `priority` is NOT depth, and **a flat column of 1 means UNIT LATENCY, not "no
  producers"**. `priority()` (`sched.c:1452`) accumulates
  `priority(x) + insn_cost(x, prev, insn) - 1` over **LOG_LINKS** (producers). gcc's
  own comment on that `- 1` says it outright: *"When all instructions have a latency
  of 1 ... Subtracting one here ensures that in such cases all instructions will end
  up with a priority of one, and hence no scheduling will be done."* So on a
  unit-latency machine a whole dependence chain floors at 1. **An insn printing
  priority 1 while DEPENDING on another priority-1 insn is correct, not a bug** — and
  "priority 1 ⟹ it has no producers" is a false inference that has cost real rounds.
* `ref_count` counts **consumers** (`INSN_DEPEND`). "Heads a long chain" describes
  THIS column. It is not priority and does not feed it.

**Priority 1 is not a floor you are stuck on.** This tool used to assert "nothing can
lift a `LOG_LINKS (nil)` insn off priority 1, and MIPS defines no `ADJUST_PRIORITY`".
The macro claim is true (0 hits in `config/mips/mips.h`) and **irrelevant** — the bump
lives in the FUNCTION `adjust_priority` (`sched.c:2535`), not the macro, so the macro's
absence disables nothing. The inference was false and it misled a whole round.

**What `adjust_priority` actually does, in this compiler: exactly one thing.** It is
gated on `reload_completed == 0` (**sched1 only, never sched2**) and switches on
`n_deaths` — but gcc's own comment there reads *"??? This code has no effect, because
REG_DEAD notes are removed before we ever get here."* So `n_deaths` is always 0, the
`default:`/`3`/`2`/`1` shift-down arms are DEAD, and only `case 0` runs: if
`birthing_insn_p(PATTERN(prev))`, set `INSN_PRIORITY(prev) = max_priority`. And
`max_priority` (`sched.c:2605`) is `MAX(INSN_PRIORITY(ready[0]), INSN_PRIORITY(insn))`
where `insn` was set to `LAUNCH_PRIORITY` (0x7f000001) at `sched.c:3923` immediately
before the `schedule_insn` call — **which is why a bumped birthing insn shows up as
`(7f000001)` in a later ready list.** `birthing_insn_p` (2499) fires iff the pattern is
`(set (REG) ...)` with a live dest and **`REG_N_SETS == 1`**.

**Anatomy of a `ready list` line — verified against the source, not inferred:**

    ;; ready list at T-20: 6 (1) 4 (1), now 6 4
                           ^^^^^^^^^^^       ^^^
                           UNSORTED,         SORTED (after SCHED_SORT)
                           priorities in HEX

`sched.c:3756` prints `UID (INSN_PRIORITY in hex)` for each ready insn **before**
`SCHED_SORT`; `sched.c:3793` prints the UIDs again **after** the sort, and
`last_scheduled_insn = insn = ready[0]` — **so the first UID after `now` IS THE PICK.**
Because the print precedes this iteration's `LAUNCH_PRIORITY` store, a `(7f000001)` in
the printed list is a *durable* birthing bump from an earlier iteration, not the
transient marker.

**`rank_for_schedule` ties break priority DESC → class DESC → LUID DESC**, and **class
is a CEILING, not a lever.** An insn `link == 0` (independent of `last_scheduled_insn`)
*or* with `insn_cost == 1` gets class **3** — the maximum — unconditionally; only a
dependence with cost ≠ 1 drops you to class 1 (data) or 2 (anti/output). Since the sort
is descending, **perturbing a dependence's cost can only sort the DEPENDENT insn LATER.
Never propose "change the dep cost" as a lever to promote a dependent insn** (that
"open lever" was proposed once, and it was backwards).

**For the ready-list trace, prefer `tools/sched-deps.py`.** It reconstructs the schedule
FORWARD and self-validates against cc1's own post-pass insn chain (78 runs, 3,464 blocks,
zero divergences; fault-injected twice). This tool printed two false readings of the raw
lines, both since corrected here, and both worth knowing if you read a raw dump:

* **A hazard cycle prints `, now` TWICE, and the FIRST one's head is the LOSER.**
  `schedule_select` (`sched.c:2713`) prints the ready list *before* the swap and *only*
  when it swaps (`if (best_insn != 0)`), then names the winner; `schedule_block` (3793)
  prints it again after the pick. On a quiet cycle only the second prints — which is why
  "the pick is the first insn of the `now` list" looks right and was "verified across 11
  consecutive insns". Those 11 were FUN_80057b80 block 0's **11 hazard swaps**, i.e. the
  reading that only works if you take the LAST `, now`. Reconstructing with the first
  fails validation on 8 blocks; with the last it validates 55/55 and matches cc1's `-dp`
  asm instruction-for-instruction. Example, verbatim:

      ;; ready list at T-8: 18 (1) 6 (1) ... 1950 (1), now 18 1950 8 6 2014 ...
      ;; insn 1950 has a greater potential hazard, now 1950 18 8 6 2014 ...
                  ^^^^ the PICK — not the 18 heading the first `, now`

* **T is not an address index.** `clock += stalls` (3747) skips T values outright, so
  "each T decrement = +4 bytes" is false. T is monotonic but not evenly spaced.

**sched is BACKWARD: T-1 is the LAST slot and is filled FIRST, so an insn PICKED EARLIER
LANDS LATER.**

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
    ap.add_argument("--pass", dest="which", choices=["sched", "sched1", "sched2"],
                    default="sched",
                    help="which scheduler. **A PROLOGUE / PARM-COPY ordering question "
                         "lives ONLY in sched2**: the prologue does not exist until "
                         "after reload, so parm copies never enter sched1's table. "
                         "This tool was sched1-only and a lane had to hand-decode the "
                         "raw .i.sched2 — it called that the single biggest cost of "
                         "its round.")
    args = ap.parse_args()
    # cc1 names the dump `.sched`, but everyone (and my own docs) says "sched1".
    # Two lanes hit `invalid choice: 'sched1'` before this alias existed.
    if args.which == "sched1":
        args.which = "sched"

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
    print("  ref_count = CONSUMERS (INSN_DEPEND). 'heads a long chain' is THIS "
          "column, NOT priority.")
    # The pre-bump caveat is SCHED1-ONLY: adjust_priority (sched.c:2535) is gated on
    # `reload_completed == 0`, so nothing bumps in sched2 and the column there really
    # is the scheduler's first sort key. Printing the sched1 caveat under a sched2
    # heading told a lane its column was untrustworthy when it was exact.
    if args.which == "sched2":
        print("  This is sched2, so NOTHING BUMPS: `adjust_priority` is gated on "
              "`reload_completed == 0`")
        print("  and does not run here. **This column IS the scheduler's first sort "
              "key** (ties then")
        print("  break class DESC -> LUID DESC). That is not true in sched1 — do not "
              "carry this")
        print("  reading across.")
    else:
        print("  *** The `priority` COLUMN BELOW IS NOT WHAT THE SCHEDULER USED. *** "
              "sched1's")
        print("  `adjust_priority` bumps a BIRTHING insn to max_priority (~"
              "LAUNCH_PRIORITY 7f000001)")
        print("  at launch time, so the table is the PRE-BUMP value. **Read the "
              "`ready list at T-k:`")
        print("  lines.**")
    print("  A priority of 1 means UNIT LATENCY, not 'no producers': priority() "
          "accumulates")
    print("  `priority(x) + insn_cost(...) - 1`, and gcc's own comment says the -1 "
          "exists so that")
    print("  an all-latency-1 chain floors at 1. An insn printing 1 while DEPENDING "
          "on another")
    print("  priority-1 insn is CORRECT, not a bug.")
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
        print(f"  --- ready-list trace ({len(ready)} decisions) ---")
        print(f"  *** Prefer `tools/sched-deps.py {args.name} --pass {args.which}`: it "
              "reconstructs this")
        print("  trace FORWARD and self-validates against cc1's own post-pass insn "
              "chain (78 runs,")
        print("  3,464 blocks, zero divergences). The raw lines below have two traps "
              "this tool")
        print("  used to get wrong:")
        print("  1. A hazard cycle prints `, now` TWICE. `schedule_select` "
              "(sched.c:2713) prints the")
        print("     list BEFORE the swap and only when it swaps, then names the winner; "
              "`schedule_block`")
        print("     (3793) prints it after. **The PICK is the head of the LAST `, now` "
              "on the line** —")
        print("     on a hazard cycle the FIRST one's head is the LOSER.")
        print("  2. T is NOT an address index: `clock += stalls` (3747) SKIPS T values "
              "outright, so")
        print("     'each T decrement = +4 bytes' is false. T is monotonic, not evenly "
              "spaced.")
        for ln in ready:
            mark = "   <-- BUMPED" if "7f000001" in ln else ""
            print("  " + ln.strip() + mark)

    # The TABLE is NOT what the scheduler used. sched1's adjust_priority bumps a
    # BIRTHING insn to LAUNCH_PRIORITY (0x7f000001) at LAUNCH TIME, so an insn the
    # table lists at 1 can be picked first. This tool used to assert the opposite
    # ("nothing can lift them; MIPS defines no ADJUST_PRIORITY") -- refuted by the
    # ready lists THIS SAME TOOL PRINTS, and it misled a whole round. The macro was
    # never the gate: sched.c's adjust_priority is gated on `reload_completed == 0`,
    # and birthing_insn_p returns `REG_N_SETS(i) == 1` for a live (set (REG) ...).
    bumped = sorted({int(m) for ln in ready
                     for m in re.findall(r"(\d+) \(7f000001\)", ln)})
    floor = [r for r in rows if r[1] == 1]
    print()
    # SCHED2 CANNOT BUMP. Printing the sched1 bump story under a sched2 heading told a
    # lane its column was untrustworthy when it was exact -- and "(no insn was bumped)"
    # reads as a measurement when it is a tautology here.
    if args.which == "sched2":
        print(f"schedtrace: {len(floor)} insn(s) show priority 1 in the table. In "
              "SCHED2 that table is")
        print("  TRUSTWORTHY: `adjust_priority` is gated on `reload_completed == 0` "
              "and does not run")
        print("  after reload, so nothing bumps and priority is the first sort key. "
              "A 1 here means")
        print("  UNIT LATENCY (the `- 1` in priority()), NOT 'no producers' — so a "
              "table full of 1s")
        print("  is a genuine LUID-ordered tie, and **LUID (source/emission order) is "
              "your lever**.")
        print("  Ties break priority DESC -> class DESC -> LUID DESC, and class is a "
              "CEILING: an insn")
        print("  independent of `last_scheduled_insn` (or with insn_cost 1) is class 3 "
              "already, so")
        print("  perturbing a dependence's cost can only sort the DEPENDENT insn "
              "LATER, never earlier.")
        return 0
    print(f"schedtrace: {len(floor)} insn(s) show priority 1 in the TABLE — but the "
          "table is NOT")
    print("  what the scheduler used. sched1's `adjust_priority` bumps a BIRTHING "
          "insn to")
    print("  max_priority (~LAUNCH_PRIORITY 0x7f000001) at LAUNCH TIME. **Read the "
          "priority in")
    print("  the `ready list at T-k:` lines above, not this table** — that "
          "distinction cost a")
    print("  full round.")
    if bumped:
        print(f"  BUMPED here: {' '.join('insn ' + str(b) for b in bumped[:12])}")
        print("  Those never compete at the floor, so 'ordered by LUID' is false for "
              "them.")
    else:
        print("  (no insn was bumped in this function's ready lists)")
    print("  `birthing_insn_p` (sched.c:2499) fires iff the pattern is "
          "`(set (REG) ...)` with a")
    print("  LIVE dest and **REG_N_SETS == 1** — set exactly once in the whole "
          "function. A")
    print("  `(set (SUBREG ...) ...)` dest — what a compound assignment to a `short` "
          "local")
    print("  produces — is NEVER birthing. sched walks BACKWARD, so a bumped insn is "
          "picked")
    print("  FIRST and lands as LATE as possible (cc1 shortening the new live range).")
    print("  It is gated on `reload_completed == 0`, NOT on the target defining "
          "ADJUST_PRIORITY.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
