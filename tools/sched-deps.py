#!/usr/bin/env python3
"""Re-render cc1's scheduler trace FORWARD, and read the decisions it prints.

`schedtrace.py` tabulates the scheduler's columns.  This tool answers the
questions lanes actually ask of them, and it answers them from cc1's own printed
lines rather than from a model:

    tools/sched-deps.py <Name>                  forward schedule, sched1
    tools/sched-deps.py <Name> --pass sched2    ... after reload (prologue lives here)
    tools/sched-deps.py <Name> --block 0        just one basic block
    tools/sched-deps.py <Name> --uid 1950       one insn: its cycle and its block
    tools/sched-deps.py <Name> --asm            + where each UID landed in the FINAL asm
    tools/sched-deps.py <Name> --verdicts       the §2848/§6559/§6872 verdicts

**THE TRAP THIS TOOL EXISTS TO CLOSE.** cc1 prints *two different* `, now` lists
and they mean opposite things:

    ;; ready list at T-8: 18 (1) 6 (1) ... 1950 (1), now 18 1950 8 6 ...
    ;; insn 1950 has a greater potential hazard, now 1950 18 8 6 ...

The first `, now` is `schedule_select`'s (sched.c:2713), printed BEFORE the
hazard swap it is about to perform -- its head (18) is NOT the pick.  The second
is `schedule_block`'s (sched.c:3793), printed after `insn = ready[0]` -- its head
(1950) IS the pick.  `schedule_select` prints its list ONLY when it swaps
(`if (best_insn != 0)`), so on a non-swap cycle the sole `, now` on the ready-list
line *is* the pick and the rule "the pick is the head of the `now` list" appears
to hold.  **On FUN_80057b80 it fails on 11 of 24 cycles in block 0 alone.**  The
rule is: the pick is the head of the LAST `, now` of the cycle.  This tool never
guesses -- it reconstructs the whole schedule and checks it against the
post-pass RTL cc1 prints in the same file (see `--validate`).

Facts this tool relies on, each read out of sched.c rather than recalled:

* The `;; insn[N]: priority = P, ref_count = R` table is printed at sched.c:3686
  BEFORE the scheduling loop (3710) and only for `INSN_PRIORITY > 0`.  **In
  sched1 it is a PRE-BUMP value and is NOT what the scheduler used; in sched2
  nothing can bump, so it IS the scheduler's first sort key.**  This tool says
  which, per pass -- printing the sched1 caveat over sched2 data tells a lane its
  exact data is untrustworthy.
* **PRIORITY IS NOT DEPTH, AND priority 1 DOES NOT MEAN "no producers".**
  priority() (1452) accumulates `priority(x) + insn_cost(x, prev, insn) - 1` over
  LOG_LINKS, floored at 1.  gcc's own comment on that `- 1`: it exists so that
  "when all instructions have a latency of 1 ... all instructions will end up
  with a priority of one, and hence no scheduling will be done."  So a cost-1
  dependence propagates priority UNCHANGED, and an insn printing 1 while
  depending on another printing 1 is CORRECT.  A flat column means unit latency,
  not an absent chain.  Priority is the longest STALL-weighted path above the
  insn.  Two probes were lost to the depth reading; this tool never infers a
  chain from the column.
* `adjust_priority` (2535) does exactly ONE thing in this compiler.  Its `n_deaths`
  switch is dead -- gcc's own comment: "??? This code has no effect, because
  REG_DEAD notes are removed before we ever get here" -- so n_deaths is always 0
  and only `case 0` runs: if `birthing_insn_p`, `INSN_PRIORITY(prev) =
  max_priority` (2572).  It does NOT assign LAUNCH_PRIORITY.  `max_priority`
  (2605) is `MAX(INSN_PRIORITY(ready[0]), INSN_PRIORITY(insn))` and `insn` holds
  LAUNCH_PRIORITY from 3923 at that moment, which is WHY a bump surfaces as
  (7f000001).  `schedule_insn` early-returns `if (LOG_LINKS(insn) == 0)` (2600),
  so a producerless scheduled insn bumps nothing at all.
* `adjust_priority` is gated on `reload_completed == 0`, so **it is sched1-only**.
  It is NOT gated on the target defining ADJUST_PRIORITY.
* A `(7f000001)` in a printed ready list is a DURABLE bump from an earlier
  iteration, not the transient marker: 3756 prints before this iteration's 3923
  store.
* **CLASS IS A CEILING, NOT A LEVER** (`rank_for_schedule`, 2415).  An insn
  independent of `last_scheduled_insn` (`link == 0`) OR reached with
  `insn_cost == 1` gets class 3 -- the maximum -- unconditionally; only a
  dependence with cost != 1 drops it to 1 (data) or 2 (anti/output).  The sort is
  class DESC.  So perturbing a dependence's cost can only sort the dependent insn
  LATER, never earlier.  "Make the dep cost != 1 to promote an insn" is exactly
  backwards and a round was spent on it.
* sched is BACKWARD: the first pick lands LAST.  Higher T = earlier address.
  **T is NOT an address index**: a stall does `clock += stalls` (3747) and skips
  T values outright -- FUN_80057b80 block 26 goes T-2 -> T-4 with no T-3.  The
  PICK ORDER maps to the address, reversed; T does not.  This tool prints the
  index, not the T.
* Ties break priority DESC -> class DESC -> LUID DESC (`rank_for_schedule`, 2415).
* The block-tail insn's priority is `TAIL_PRIORITY - i` (3338) where `i` is a
  STALE loop variable -- the initialiser was `#if 0`'d out at 3365.  That is why
  the tail prints 2147482912 and not a round number.  It is not a depth.
* A prologue / parm-copy question lives ONLY in sched2.

NOT BUILT, on purpose: the §6399 remat-split verdict (a UID rewritten in place to
a `const_int` set while its store reappears at a fresh UID => "reload split -- not
a sched tie, not fence-fixable").  The shape is real and the detector is ~15
lines, but there is no known instance in this tree to test it against, and an
untested detector for a subtle shape is exactly how a confidently wrong verdict
gets into the cookbook.  Build it WITH a case in hand.

Run inside the nix devShell.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import re
import subprocess
import sys

import rtldump

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

BLOCK_HDR = re.compile(r";;\s+--\s+basic block number (\d+) from (\d+) to (\d+) --")
TABLE = re.compile(r";;\s*insn\[\s*(\d+)\]:\s*priority\s*=\s*(-?\d+),\s*"
                   r"ref_count\s*=\s*(-?\d+)")
READY = re.compile(r";;\s*ready list at T-(\d+):")
READY_ITEM = re.compile(r"(\d+) \(([0-9a-f]+)\)")
NOW = re.compile(r", now((?: \d+)*)")
HAZARD = re.compile(r";;\s*insn (\d+) has a greater potential hazard")
BLOCKING = re.compile(r";;\s*blocking insn (\d+) for (\d+) cycles")
LAUNCH = re.compile(r";;\s*launching (\d+) before (\d+) with (?:no|(\d+)) stalls "
                    r"at T-(\d+)")
TOTAL = re.compile(r";;\s*total time = (\d+)")
# `(insn:HI 15 6 2002 ...)` -- an insn head carries BOTH `/flag` suffixes and a
# MODE (`:HI`).  Matching only `/\w+` silently dropped every moded insn from the
# post-pass chain, which made the self-validation report a false divergence
# (insn 15 "absent from the RTL chain" while sitting in it, and in the asm).
# A validator that cries wolf gets switched off, so this regex is load-bearing.
RTL_HEAD = re.compile(r"^\((insn|jump_insn|call_insn)(?:[:/]\w+)* (\d+) (\d+) (\d+) ")
ANY_RTL = re.compile(r"^\((?:insn|jump_insn|call_insn|note|code_label|barrier)\b")
# `-dp` (final.c:2549) -> ` # <uid> <pattern_name>[/alt]` on the FIRST asm insn
# of each RTL insn.  ASM_COMMENT_START is `#` on MIPS.
DP = re.compile(r"#\s+(\d+)\s+([A-Za-z_][\w]*)(?:/(\d+))?\s*$")



class TRecord:
    """One iteration of schedule_block's `while (sched_n_insns < n_insns)` loop."""

    def __init__(self, t):
        self.t = t
        self.ready = []        # [(uid, priority)] -- PRE-SORT (printed at 3756)
        self.nows = []         # every `, now` list, in print order
        self.hazard = None     # uid named by `;; insn N has a greater potential hazard`
        self.blocked = []      # [(uid, cycles)] queued by actual_hazard
        self.launched = []     # [(uid, before, stalls, t)] from the preceding lines

    @property
    def pick(self):
        """The insn scheduled this cycle, or None if every ready insn was queued.

        The head of the LAST `, now`.  schedule_block prints its list (3793)
        after `insn = ready[0]`, so it is always last and always post-swap.  A
        cycle with no `, now` at all is schedule_select returning 0 -- everything
        was blocked, the loop `continue`s (3781) and nothing is scheduled.
        """
        if not self.nows or not self.nows[-1]:
            return None
        return self.nows[-1][0]

    @property
    def swapped(self):
        """True if potential_hazard overrode the sort order this cycle.

        schedule_select only prints (and only swaps) when `best_insn != 0`
        (2709), i.e. when the hazard winner was not already at the front.
        """
        return self.hazard is not None

    @property
    def displaced(self):
        """The insn the sort put first, which the hazard swap then demoted.

        Only meaningful when `swapped`: nows[-2] is schedule_select's pre-swap
        list.  This is the insn that WOULD have been picked on priority/class/LUID
        alone -- the §2848 'symptom' half.
        """
        if not self.swapped or len(self.nows) < 2 or not self.nows[-2]:
            return None
        return self.nows[-2][0]


class Block:
    def __init__(self, num, first, last):
        self.num, self.first, self.last = num, first, last
        self.table = {}        # uid -> (priority, ref_count), cc1's own line
        self.records = []
        self.total_time = None

    @property
    def picks(self):
        """Pick order: the order cc1 chose insns.  First pick lands LAST."""
        return [r.pick for r in self.records if r.pick is not None]

    @property
    def emission(self):
        """Forward order: the order the insns are emitted.  sched is BACKWARD."""
        return list(reversed(self.picks))


def split_dump(body):
    """(trace_text, rtl_text).  The RTL listing starts at the first `(` line.

    cc1 writes the pass trace while scheduling and then `print_rtl_with_bb`s the
    whole function into the same file (toplev.c:3435).  That listing is the
    post-pass insn chain -- i.e. cc1's own answer to 'what order did you pick?',
    which is what makes `validate()` possible.
    """
    lines = body.splitlines()
    for i, line in enumerate(lines):
        if ANY_RTL.match(line):
            return "\n".join(lines[:i]), "\n".join(lines[i:])
    return body, ""


def parse_blocks(trace_text):
    """Parse the trace into per-block records.  Structure is per sched.c:

        ;;  -- basic block number N from A to B --
        ;; ready list initially: ...
        ;; insn[N]: priority = P, ref_count = R     (3686, pre-loop, PRE-BUMP)
        ;; launching N before M with no stalls at T-k    (3719, own line)
        ;; ready list at T-k: <uid (pri)>*                (3756, no newline!)
          [ , now <uids> + "\\n;; insn N has a greater potential hazard" ]  (2713)
          [ , now <uids> ]                                                 (3793)
        ;; total time = N
    """
    blocks, cur, rec, pending = [], None, None, []
    for line in trace_text.splitlines():
        m = BLOCK_HDR.search(line)
        if m:
            cur = Block(int(m.group(1)), int(m.group(2)), int(m.group(3)))
            blocks.append(cur)
            rec, pending = None, []
            continue
        if cur is None:
            continue
        m = TABLE.search(line)
        if m:
            cur.table[int(m.group(1))] = (int(m.group(2)), int(m.group(3)))
            continue
        # A launching line is printed complete-with-newline BEFORE the ready
        # list it feeds, so it can carry no `, now` and belongs to the next
        # record.
        m = LAUNCH.search(line)
        if m:
            pending.append((int(m.group(1)), int(m.group(2)),
                            int(m.group(3) or 0), int(m.group(4))))
            continue
        m = READY.search(line)
        if m:
            rec = TRecord(int(m.group(1)))
            rec.launched, pending = pending, []
            seg = line[m.end():]
            cut = seg.find(", now")
            rec.ready = [(int(u), int(p, 16))
                         for u, p in READY_ITEM.findall(seg if cut < 0 else seg[:cut])]
            cur.records.append(rec)
        # NOT `continue`: `;; blocking insn ...` has no trailing newline, so a
        # `, now` from schedule_select can share its line.
        m = BLOCKING.search(line)
        if m and rec is not None:
            rec.blocked.append((int(m.group(1)), int(m.group(2))))
        m = HAZARD.search(line)
        if m and rec is not None:
            rec.hazard = int(m.group(1))
        if rec is not None:
            for nm in NOW.finditer(line):
                rec.nows.append([int(x) for x in nm.group(1).split()])
        m = TOTAL.search(line)
        if m:
            cur.total_time = int(m.group(1))
            rec = None
    return blocks


def rtl_order(rtl_text):
    """UIDs of the real insns in the post-pass chain, in emitted order."""
    return [int(m.group(2)) for line in rtl_text.splitlines()
            if (m := RTL_HEAD.match(line))]


def rtl_bodies(rtl_text):
    """uid -> flattened RTL text, for the insn column."""
    out, uid, buf = {}, None, []
    for line in rtl_text.splitlines():
        m = RTL_HEAD.match(line)
        if m:
            if uid is not None:
                out[uid] = " ".join(" ".join(buf).split())
            uid, buf = int(m.group(2)), [line]
        elif uid is not None:
            if ANY_RTL.match(line):
                out[uid] = " ".join(" ".join(buf).split())
                uid, buf = None, []
            else:
                buf.append(line)
    if uid is not None:
        out[uid] = " ".join(" ".join(buf).split())
    return out


def validate(blocks, order):
    """Check the reconstruction against cc1's own post-pass insn chain.

    This is the whole reason to trust anything below.  If `pick` picked the
    wrong `, now` -- the exact error the folklore rule makes on every hazard
    swap -- the reconstructed order stops matching the chain cc1 printed, and we
    refuse to print rather than emit a confident lie.

    Returns [] when the model agrees, else a list of complaint strings.
    """
    if not order:
        return ["no post-pass RTL listing in the dump — cannot self-validate"]
    rank = {u: i for i, u in enumerate(order)}
    bad = []
    for b in blocks:
        emission = b.emission
        unknown = [u for u in emission if u not in rank]
        if unknown:
            bad.append(f"block {b.num}: picked insn(s) {unknown} are absent from "
                       f"the post-pass RTL chain")
            continue
        actual = [u for u in order if u in set(emission)]
        if emission != actual:
            i = next((k for k, (x, y) in enumerate(zip(emission, actual))
                      if x != y), min(len(emission), len(actual)))
            bad.append(
                f"block {b.num}: reconstructed emission order diverges from the "
                f"chain cc1 printed, first at index {i}: "
                f"ours={emission[i:i + 4]} cc1={actual[i:i + 4]}")
    return bad


# --------------------------------------------------------------------------
# UID -> emitted asm.  cc1's own `-dp` annotation, then the real maspsx/as
# pipeline for the address.
# --------------------------------------------------------------------------

def parse_dp(asm_text):
    """[(uid, pattern, text)] for each *real* instruction, in emitted order.

    Only the FIRST asm insn of an RTL insn carries the comment (final.c clears
    `debug_insn`), so an instruction with no comment continues the previous UID.

    **The `#nop` trap.** maspsx emits both `nop # DEBUG: ...` (a real load-delay
    nop -- one instruction) and `#nop # DEBUG: ...` (a commented-out nop it
    decided against -- zero instructions).  A round counted the commented form as
    an instruction and manufactured a phantom '+3 surplus refs' theory from it.
    A line whose first non-blank character is `#` is a comment, full stop.
    """
    out = []
    for raw in asm_text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or line.startswith("."):
            continue
        if line.startswith("$") or line.endswith(":"):
            continue          # label
        m = DP.search(line)
        uid = int(m.group(1)) if m else None
        pat = m.group(2) if m else None
        text = re.sub(r"\s*#.*$", "", line).strip()
        if not text:
            continue
        out.append((uid, pat, text))
    return out


def dp_carry(rows):
    """Fill each instruction's UID down from the last annotated one."""
    out, last = [], None
    for uid, pat, text in rows:
        if uid is not None:
            last = uid
        out.append((last, pat, text))
    return out


def asm_bridge(name, draft):
    """cc1's FINAL emission order, with each instruction's UID, from its own `-dp`.

    Returns [(uid, pattern, text)] per emitted asm instruction, in order.  `-dp`
    (final.c:2549) makes cc1 annotate the FIRST asm insn of every RTL insn with
    `# <uid> <pattern>`; it is comment-only and cannot change codegen, and it is
    the only UID -> asm mapping that is not a hand-rolled correlation.

    **This stops at cc1's asm on purpose -- there is deliberately NO address
    column.**  Two independent attempts to carry it through to an address both
    corrupt the very bytes they would be describing:

    * `-dp`'s trailing comment defeats maspsx's operand parser, so it stops
      gp-addressing: Think1sleep's asm maspsxes to FOUR `%gp_rel` references
      normally and ZERO once `-dp` is on, turning each `lw $2,SYM` back into a
      two-instruction `lui`/`lw`.  The `-dp` asm is then not the build's asm.
    * Moving the UID onto its own comment line restores gp-addressing but breaks
      maspsx's load-delay analysis, which is line-ADJACENCY sensitive: the marked
      input silently loses a real `nop`.  Fewer instructions again.

    So an address derived this way would describe code the build never emits.
    For addresses use `tdis --both`, which reads the linked image.  The order and
    the UIDs below are exact, and they are what §6872 needs.
    """
    res = rtldump.compile_rtl(name, ["sched2"], draft=draft, extra_flags=["-dp"])
    raw = open(res["asm"], errors="replace").read()
    return dp_carry(parse_dp(raw))


# --------------------------------------------------------------------------
# Verdicts
# --------------------------------------------------------------------------

ARGREG = {4: "a0", 5: "a1", 6: "a2", 7: "a3"}
HARDSET = re.compile(r"\(set \(reg:\w+ (\d+) (\w+)\)")


def hazard_groups(block):
    """§2848: every cycle where potential_hazard overrode the sort.

    cc1 NAMES the winner (`;; insn N has a greater potential hazard`) and prints
    the list both before and after the swap, so nothing here is derived.
    """
    out = []
    for rec in block.records:
        if not rec.swapped:
            continue
        pri = dict(rec.ready)
        top = pri.get(rec.hazard)
        group = [u for u, p in rec.ready if p == top]
        out.append(dict(t=rec.t, winner=rec.hazard, displaced=rec.displaced,
                        priority=top, group=group))
    return out


def argmoves(block, bodies):
    """Every set of a hard argument register in this block: [(uid, reg, pri)]."""
    out = []
    for uid, (pri, _ref) in sorted(block.table.items()):
        m = HARDSET.search(bodies.get(uid, ""))
        if m and int(m.group(1)) in ARGREG:
            out.append((uid, ARGREG[int(m.group(1))], pri))
    return out


def argmove_census(blocks, bodies):
    """§6559, run as a FALSIFICATION rather than asserted.

    The cookbook rule is "hard-reg argument move is a PERMANENT floor: argmove +
    priority-1 => unreachable, park".  The conjunction is real but the *rule* is
    not: being an argmove does not imply the floor.  This counts both, so the
    verdict can never be quoted without its own counterexamples.

    Measured on FUN_80057b80 sched2 -- the function the rule is cited for --
    only 7 of 30 argmoves sit at priority 1; the other 23 reach 2, 11, 12, 14,
    25, 28, 51, 52 and 53.  Returns (floor, above, peak) where `peak` is the
    highest-priority argmove, i.e. the rule's strongest counterexample.
    """
    all_moves = [(b, uid, reg, pri) for b in blocks
                 for (uid, reg, pri) in argmoves(b, bodies)]
    floor = [m for m in all_moves if m[3] == 1]
    above = [m for m in all_moves if m[3] != 1]
    peak = max(above, key=lambda m: m[3]) if above else None
    return floor, above, peak


def reordered(block):
    """§6872 half: did sched actually move anything in this block?

    `block.table` is printed by walking the insn chain (sched.c:3684) BEFORE the
    scheduling loop, so its key order IS the pre-sched order -- no second dump
    needed.  Comparing it with the reconstructed emission order says whether the
    scheduler is even a candidate lever for this block.
    """
    before = [u for u in block.table if u in set(block.emission)]
    after = block.emission
    return before, after, before != after


# --------------------------------------------------------------------------
# Output
# --------------------------------------------------------------------------

BLIND = {
    "sched1": "sched1 runs BEFORE reload, so every register below is a PSEUDO and "
              "the prologue does not exist yet.",
    "sched2": "sched2 runs AFTER reload: hard registers, prologue present. "
              "adjust_priority cannot fire here (gated reload_completed == 0).",
}


def provenance(name, src, dump, which, draft):
    h = hashlib.sha1(open(src, "rb").read()).hexdigest()[:10]
    print(f"sched-deps: {name} — cc1's own {which} trace, re-rendered FORWARD")
    print(f"  source: {src} (sha1 {h}){'  [DRAFT: the #else NON_MATCHING body]' if draft else ''}")
    print(f"  dump:   {dump}")
    print(f"  blind spot: {BLIND[which if which == 'sched2' else 'sched1']}")


def print_schedule(blocks, bodies, addrs, only_block, which="sched", width=58):
    print()
    print("  Forward order = the order these insns are EMITTED. sched picks "
          "BACKWARD (first")
    print("  pick lands last), so this table is the reverse of cc1's pick "
          "sequence. `T` is")
    print("  cc1's clock, printed for reference ONLY: a stall does clock += "
          "stalls and skips")
    print("  T values outright, so T is NOT an address index — the INDEX column "
          "is.")
    if which == "sched2":
        print("  `pri` IS the scheduler's first sort key here: sched2 runs with "
              "reload_completed,")
        print("  and adjust_priority is gated on reload_completed == 0, so nothing "
              "bumps in this")
        print("  pass. The column is exact. (In sched1 it would be a PRE-BUMP "
              "value.)")
    else:
        print("  `pri` is the PRE-BUMP table value (sched.c:3686, printed before "
              "the loop runs).")
        print("  sched1 CAN bump a birthing insn mid-loop, so this column is not "
              "always what the")
        print("  scheduler used; the pick order in this table is. A (7f000001) in "
              "a ready list is")
        print("  such a bump.")
    print("  priority 1 does NOT mean 'no producers' — it means no cost!=1 "
          "producer above it")
    print("  (priority() floors at 1 and a cost-1 dependence propagates it "
          "unchanged).")
    print()
    for b in blocks:
        if only_block is not None and b.num != only_block:
            continue
        swaps = sum(1 for r in b.records if r.swapped)
        print(f"  --- basic block {b.num}: insns {b.first}..{b.last}, "
              f"{len(b.emission)} scheduled, total time {b.total_time}"
              f"{f', {swaps} hazard swap(s)' if swaps else ''} ---")
        bypick = {r.pick: r for r in b.records if r.pick is not None}
        hdr = f"  {'idx':>4} {'asm':>5} {'T':>4} {'uid':>6} {'pri':>10} {'ref':>4}  insn"
        print(hdr)
        for i, uid in enumerate(b.emission):
            rec = bypick[uid]
            pri, ref = b.table.get(uid, ("?", "?"))
            pris = (f"{pri:#x}" if isinstance(pri, int) and pri > 0xffff
                    else str(pri))
            a = addrs.get(uid)
            astr = str(a) if a is not None else "-"
            mark = ""
            if rec.swapped:
                mark = f"  <- HAZARD SWAP: beat insn {rec.displaced}"
            body = bodies.get(uid, "")
            print(f"  {i:>4} {astr:>5} {rec.t:>4} {uid:>6} {pris:>10} {ref:>4}  "
                  f"{body[:width]}{mark}")


def print_verdicts(blocks, bodies, order, which, name):
    print()
    print("  ============================ VERDICTS "
          "============================")

    # ---- §2848 -----------------------------------------------------------
    groups = [(b, g) for b in blocks for g in hazard_groups(b)]
    print()
    print(f"  §2848  equal-priority potential_hazard swap — {len(groups)} "
          f"found")
    if not groups:
        print("    None. No cycle in this function had potential_hazard override "
              "the sort,")
        print("    so no residual here is a hazard-swap symptom.")
    else:
        print("    cc1 NAMES the winner itself (`;; insn N has a greater potential")
        print("    hazard`, sched.c:2717) and prints the list both before and after "
              "the")
        print("    swap. Nothing below is derived.")
        print()
        for b, g in groups[:12]:
            print(f"    block {b.num} T-{g['t']}: insns {g['group']} all tie at "
                  f"priority {g['priority']}.")
            print(f"      cc1's sort put {g['displaced']} first "
                  f"(priority DESC -> class DESC -> LUID DESC);")
            print(f"      potential_hazard overrode it and picked {g['winner']} "
                  f"-> {g['winner']} lands LATER than {g['displaced']}.")
        if len(groups) > 12:
            print(f"    ... {len(groups) - 12} more (use --block N). The count "
                  "above is the total.")
        print()
        print("    VERDICT: the swap is a SYMPTOM, not the lever. potential_hazard "
              "is only")
        print("    consulted because the members TIE on priority; break the tie and "
              "the swap")
        print("    never happens. Priority rises only via a cost!=1 producer "
              "(sched.c:1521), so")
        print("    the lever is what FEEDS the insn — e.g. sourcing it from a load "
              "rather than")
        print("    a register — not the hazard and not the ordering.")
        print("    BLIND SPOT 1: this says which insns tie and who won, NOT which C "
              "change")
        print("    breaks the tie. `potential_hazard` is a function-unit model this "
              "tool does")
        print("    NOT reimplement — it reads cc1's answer. If your edit leaves the "
              "priorities")
        print("    equal, expect the swap back.")
        print("    BLIND SPOT 2 (a round was spent learning this): do NOT try to "
              "promote an")
        print("    insn by making a dependence cost != 1. CLASS IS A CEILING — "
              "rank_for_schedule")
        print("    (2415) gives class 3, the maximum, to anything independent of "
              "the last")
        print("    scheduled insn OR reached at cost 1, and sorts class DESC. A "
              "cost!=1 dep can")
        print("    only drop you to class 1/2 and sort you LATER. That lever runs "
              "backwards.")

    # ---- §6872 -----------------------------------------------------------
    moved = [(b, reordered(b)) for b in blocks]
    stirred = [(b, r) for b, r in moved if r[2]]
    print()
    print(f"  §6872  did {which} reorder anything? — {len(stirred)} of "
          f"{len(blocks)} blocks moved")
    if not stirred:
        print("    NO BLOCK WAS REORDERED. Pre-sched order already equals the "
              "emitted order,")
        print(f"    so {which} is not the lever for any residual in this function: "
              "an out-of-")
        print("    order residual was decided BEFORE this pass (or after it, by "
              "reorg).")
    else:
        for b, (before, after, _) in stirred[:8]:
            i = next((k for k, (x, y) in enumerate(zip(before, after)) if x != y), 0)
            print(f"    block {b.num}: first move at emitted index {i} — "
                  f"pre-sched {before[i:i + 4]} -> emitted {after[i:i + 4]}")
        if len(stirred) > 8:
            print(f"    ... {len(stirred) - 8} more.")
        print("    Blocks NOT listed were left in source order by this pass.")
    print("    BLIND SPOT: 'pre-sched order' is this pass's INPUT chain "
          "(sched.c:3684's own")
    print("    walk), not your C statement order — earlier passes already moved "
          "things.")
    print("    And an EMPTY DELAY SLOT is decided by reorg, AFTER sched2; "
          "compare with")
    print("    --asm to see what reorg did.")

    # ---- §6559 -----------------------------------------------------------
    if which == "sched2":
        floor, above, peak = argmove_census(blocks, bodies)
        print()
        print(f"  §6559  hard-reg argument moves — {len(floor)} at the priority "
              f"floor, {len(above)} ABOVE it")
        if not floor and not above:
            print("    None. No set of a0..a3 in this function.")
        else:
            for b, uid, reg, _p in floor[:8]:
                rec = next((r for r in b.records if r.pick == uid), None)
                idx = b.emission.index(uid) if uid in b.emission else None
                print(f"    block {b.num} insn {uid} -> ${reg}: priority 1 "
                      f"(FLOOR), picked T-{rec.t if rec else '?'}, emitted index "
                      f"{idx}")
            if len(floor) > 8:
                print(f"    ... {len(floor) - 8} more at the floor.")
            print()
            print("    FALSIFICATION RUN (this verdict refuses to be quoted "
                  "without it):")
            if above:
                print(f"    *** The cookbook rule 'a hard-reg argument move is a "
                      f"PERMANENT floor'")
                print(f"    *** IS FALSE HERE. {len(above)} of "
                      f"{len(floor) + len(above)} argument moves in this function "
                      f"are NOT at")
                print(f"    *** the floor; the highest prints priority "
                      f"{peak[3]} (insn {peak[1]} -> ${peak[2]}, block "
                      f"{peak[0].num}).")
                print("    Being an argument move does NOT put an insn at the "
                      "floor. The two halves")
                print("    of that conjunction are independent and only the "
                      "MEASUREMENT counts.")
            else:
                print(f"    Every argument move in this function does sit at the "
                      f"floor — but that is")
                print("    a fact about THIS function, not about argument moves. "
                      "The same census on")
                print("    FUN_80057b80 finds 23 of 30 above the floor, one at "
                      "priority 53.")
            print()
            print("    WHAT THE FLOOR ACTUALLY MEANS: priority 1 = no cost!=1 "
                  "producer above this")
            print("    insn (priority() floors at 1; a cost-1 dependence "
                  "propagates it unchanged,")
            print("    sched.c:1521 and gcc's own comment there). It does NOT mean "
                  "'no producers',")
            print("    and it is reachable: put a multi-cycle producer (a load) in "
                  "the insn's")
            print("    dependency chain and the column moves.")
            print("    In sched2 there is no bump path — adjust_priority is gated "
                  "on")
            print("    reload_completed == 0 — so among the floor insns the order "
                  "is decided by")
            print("    class then LUID, i.e. by the order they ENTER sched2. That "
                  "is a source-")
            print("    order lever, not a priority one.")
            print("    BLIND SPOT: 'class' is a CEILING, not a lever — class 3 is "
                  "given")
            print("    unconditionally to anything independent of the last "
                  "scheduled insn or")
            print("    reached at cost 1, and the sort is class DESC, so making a "
                  "dependence")
            print("    cost != 1 can only push an insn LATER. Do not reach for that "
                  "one.")
    else:
        print()
        print("  §6559  hard-reg argument move — SKIPPED: sched1 runs before "
              "reload, so")
        print("    argument registers do not exist yet and parm copies are not in "
              "this")
        print("    table at all. Re-run with --pass sched2.")


def main():
    ap = argparse.ArgumentParser(
        description="Re-render cc1's scheduler trace forward; read the decisions "
                    "it prints.")
    ap.add_argument("name")
    ap.add_argument("--pass", dest="which", choices=["sched", "sched1", "sched2"],
                    default="sched",
                    help="which scheduler. A PROLOGUE / PARM-COPY question lives "
                         "ONLY in sched2.")
    ap.add_argument("--block", type=int, help="only this basic block")
    ap.add_argument("--uid", type=int, help="only this insn, with its cycle")
    ap.add_argument("--asm", action="store_true",
                    help="add an `asm` column: where each UID landed in cc1's "
                         "FINAL emitted asm, read from cc1's own -dp annotation. "
                         "Because that order is POST-REORG, comparing it with this "
                         "pass's order shows exactly what reorg moved into delay "
                         "slots (§6872). Deliberately not an address — see "
                         "asm_bridge's docstring.")
    ap.add_argument("--verdicts", action="store_true",
                    help="the §2848 / §6559 / §6872 verdicts")
    ap.add_argument("--stub", action="store_true",
                    help="measure the INCLUDE_ASM stub of a guarded function. "
                         "Almost never what you want: the stub's RTL is the "
                         "trampoline, not your C.")
    args = ap.parse_args()
    if args.which == "sched1":
        args.which = "sched"

    src = os.path.join("src", "main.exe", args.name + ".c")
    if not os.path.exists(src):
        sys.exit(f"sched-deps: {src} not found")
    draft = rtldump.is_guarded(args.name, src) and not args.stub
    if args.stub and rtldump.is_guarded(args.name, src):
        print("sched-deps: WARNING: --stub on a guarded function measures the "
              "INCLUDE_ASM trampoline, not your C.", file=sys.stderr)

    try:
        res = rtldump.compile_rtl(args.name, [args.which], draft=draft)
    except (FileNotFoundError, ValueError, RuntimeError) as e:
        sys.exit(f"sched-deps: {e}")
    dump = next((p for p in res["dumps"]
                 if p.rsplit(".", 1)[-1] == args.which), None)
    if dump is None:
        sys.exit(f"sched-deps: cc1 produced no .{args.which} dump")
    body = open(dump, errors="replace").read()
    if not body.strip():
        sys.exit(f"sched-deps: {dump} is EMPTY — that is an error, not a result. "
                 "Did the pass run?")

    trace, rtl = split_dump(body)
    blocks = parse_blocks(trace)
    order = rtl_order(rtl)
    bodies = rtl_bodies(rtl)
    if not blocks:
        sys.exit("sched-deps: no `-- basic block number N from A to B --` headers "
                 "in the dump — cc1 emits them from sched.c:3232. Dump format "
                 "changed — do not trust this tool until it is re-read.")
    if not any(b.table for b in blocks):
        sys.exit("sched-deps: no `;; insn[N]: priority = P, ref_count = R` lines — "
                 "cc1 emits them from sched.c:3686. Dump format changed — do not "
                 "trust.")

    provenance(args.name, src, dump, args.which, draft)

    bad = validate(blocks, order)
    total_cycles = sum(len(b.records) for b in blocks)
    scheduled = sum(len(b.emission) for b in blocks)
    print(f"  totals: {len(blocks)} blocks, {scheduled} insns scheduled over "
          f"{total_cycles} cycles, "
          f"{sum(1 for b in blocks for r in b.records if r.swapped)} hazard swap(s)")
    if bad:
        print()
        print("  *** SELF-VALIDATION FAILED — OUTPUT REFUSED ***")
        print("  The schedule reconstructed from the ready lists does not match "
              "the insn")
        print("  chain cc1 printed in the SAME dump after the pass. One of them is "
              "being")
        print("  misread and it is not worth guessing which.")
        for line in bad[:5]:
            print(f"    {line}")
        if len(bad) > 5:
            print(f"    ... and {len(bad) - 5} more block(s).")
        print("  Report this — a divergence here is a real finding about the dump "
              "format,")
        print("  not a nuisance.")
        return 2
    print(f"  self-validation: OK — the reconstruction matches cc1's own post-pass "
          f"chain in all {len(blocks)} block(s).")

    addrs, notes = {}, []
    if args.asm:
        addrs, notes = do_asm(args.name, draft, blocks, order)
    for n in notes:
        print(f"  {n}")

    shown = blocks
    if args.uid is not None:
        shown = [b for b in blocks if args.uid in set(b.emission)]
        if not shown:
            sys.exit(f"sched-deps: insn {args.uid} was not scheduled in "
                     f".{args.which} (it may be a note, or in another pass)")
        args.block = shown[0].num
    print_schedule(blocks, bodies, addrs, args.block, args.which)
    if args.verdicts:
        print_verdicts(blocks, bodies, order, args.which, args.name)
    return 0


def reorg_moves(rows, blocks):
    """§6872's other half: what reorg (dbr) did AFTER this pass.

    `rows` is the emitted order; `blocks` is sched2's.  reorg runs after sched2
    and fills delay slots by moving an insn ACROSS the branch, so a difference
    here is a delay-slot fill and is NOT the scheduler's doing.  Verified on
    FUN_80057b80 block 0: sched2 emits ... 30, 21, 32, 33; the asm reads
    ... 30, 32, 33, 21 -- insn 21 went into insn 33's delay slot.
    """
    seen, emitted = set(), []
    for uid, _pat, _text in rows:
        if uid is not None and uid not in seen:
            seen.add(uid)
            emitted.append(uid)
    sched = [u for b in blocks for u in b.emission if u in seen]
    return emitted, sched, emitted != sched


def do_asm(name, draft, blocks, order):
    """§6872's reorg half: cc1's FINAL emission order vs this pass's order.

    Returns (asm_index, notes).  `asm_index` maps uid -> position in the emitted
    asm, which is post-reorg and therefore the real answer to "where did this
    insn end up".  There is no address column and the docstring on `asm_bridge`
    says why at length: every route from here to an address changes the bytes.
    """
    notes = []
    try:
        rows = asm_bridge(name, draft)
    except (RuntimeError, ValueError, FileNotFoundError) as e:
        return {}, [f"--asm: unavailable — {e}"]
    if not rows:
        return {}, ["--asm: cc1 emitted no annotated instructions — -dp produced "
                    "nothing, do not trust this section"]

    emitted, sched, moved = reorg_moves(rows, blocks)
    idx = {uid: i for i, uid in enumerate(emitted)}
    notes.append(
        f"--asm: {len(rows)} instructions emitted, {len(emitted)} RTL insns, from "
        f"cc1's own -dp annotation (final.c:2549) — not a hand-rolled "
        f"correlation. The `asm` column is the position in the FINAL asm, which "
        f"is post-reorg.")
    notes.append(
        "--asm: NO ADDRESS COLUMN, deliberately — -dp's comment defeats maspsx's "
        "gp rewriting (Think1sleep: 4 %gp_rel refs without it, 0 with), so an "
        "address derived here would describe code the build never emits. Use "
        "`tdis --both` for addresses.")
    if moved:
        i = next((k for k, (x, y) in enumerate(zip(emitted, sched)) if x != y), 0)
        notes.append(
            f"--asm: §6872 — the EMITTED order is NOT this pass's order. reorg "
            f"(dbr) runs after sched2 and moved insns, which is what a delay slot "
            f"IS. First difference at emitted index {i}: sched2 ordered "
            f"{sched[i:i + 3]}, the asm reads {emitted[i:i + 3]}. An empty or "
            f"surprising delay slot is reorg's doing — do not attack it with "
            f"priority.")
    else:
        notes.append(
            "--asm: §6872 — the emitted order EQUALS this pass's order; reorg "
            "moved nothing. Any order residual was decided here or earlier.")
    return idx, notes


if __name__ == "__main__":
    sys.exit(main())
