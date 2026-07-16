#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void AddEnemy(void);
 *     INFOVIEW.C:695, 58 src lines, frame 2064 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+24     unsigned char [70][20] names
 *     stack sp+1424   struct TAdtSelect [70] ItemName
 *     reg   $s4       short i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s7       long type
 *     reg   $s5       long x
 *     reg   $s2       long y
 *     reg   $s0       long z
 *     reg   $s3       short r
 *     reg   $s2       short think
 *     stack sp+1984   struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern short *StageAppearance[10];
 *     extern struct WeaponModelType WeaponModel[41];
 *     extern int StageID;
 *     extern struct ThinkDBtype ThinkDB[20];
 *     extern struct TCameraStatus CamState;
 *     extern int CurrentEnemyID;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — complete pure-C reconstruction at the exact target
 * extent (1152 bytes / 288 instructions) and exact 0x810 frame.  The guarded
 * draft has 42 differing bytes (183 -> 161 -> 125 -> 81 -> 72 -> 52 -> 42).
 *
 * ROUND 6 (-10): loop 2's preheader ORDER, carrier, base and type are now ALL
 * EXACT.  Three things had to be true at once; each alone measured worse:
 *  (1) `menu_base = ItemName` RELOCATED from a pre-loop statement to a
 *      statement inside the do-while, below the think scan.  A hoist is
 *      emitted with emit_insn_before(loop_start) — i.e. AFTER every pre-loop
 *      statement — so a pre-loop `menu_base` can NEVER follow the hoists.
 *      Relocated, it becomes a third movable discovered after the base's, and
 *      move_movables emits movables in discovery order (loop.c appends:
 *      `last_movable->next = m`).  Round 5 found this; alone it is 97.
 *  (2) `think_item = item`, NOT `= ItemName` (round 5) and NOT `= menu_base`.
 *      This is the whole trick.  loop.c:691-703 case (1) is
 *      `reg_in_basic_block_p`, whose FIRST test is
 *      `REGNO_FIRST_UID (regno) != INSN_UID (insn) -> return 0`: the set must
 *      be the reg's FIRST mention in the WHOLE function.  So a relocated
 *      `menu_base` cannot be read earlier in the loop — `think_item =
 *      menu_base` kills the hoist outright.  Round 5 therefore wrote
 *      `think_item = ItemName`, which cse2 rematerialises as
 *      `addiu a1,sp,1424`.  Reading `item` instead satisfies BOTH: menu_base
 *      keeps no earlier mention (case (1) holds, the hoist lands after the
 *      base), AND `item` stays LIVE across the loop, so cse2 still rewrites
 *      the hoisted `menu_base = ItemName` into `move s0,s1`, and — the
 *      cascade retail depends on — the ThinkDB base can no longer reuse $s1,
 *      taking $s6, pushing the carrier to $s8 and type to $s7.  All exact.
 *      This alone is 84: correct structure, pure 4-cycle rename left.
 *  (3) ADD_ENEMY_FENCE_19 on `think_item = item`.  Keeping `item` live costs
 *      it priority (8 refs / 71 insns -> 3380 vs category's 20/54 -> 14814),
 *      which rotated {item,category,i,count} through {s5,s1,s3,s4}.  ONE
 *      ballast site fixes the whole cycle: `tools/regalloc.py AddEnemy
 *      --between 99 90 95 --enclosed-refs 1` says "+19..+20 weighted refs",
 *      and depth 19 lands item at 27 refs -> 15211, above category (14814)
 *      and below p95 (16000).  Order becomes item,category,i,count = retail's
 *      s1,s3,s4,s5.  Do NOT overshoot: 32 refs crosses a floor_log2 step
 *      (4->5) and doubles the priority.
 *
 * ROUND 6 DISPROOF — the brief's two "diagnosed costs" were both WRONG, and
 * both are cheap to re-check, so do not re-derive them:
 *  - "The item/category swap is an allocno_compare inequality; add ballast"
 *    is FALSE for round 5's structure.  Dropping `category = 0` to FENCE_5
 *    flips the allocation ORDER exactly as the arithmetic predicts
 *    (`... 93 95 90 82 80 ...` -> `... 93 95 82 90 80 ...`) and changes NOT
 *    ONE BYTE, because round-5's `item` allocno carries hard regs 16 AND 17
 *    in its CONFLICT list — it cannot hold $s1 at any priority.  A swap is
 *    only a priority question once you have checked the loser CAN hold the
 *    winner's register.  Read the conflict list BEFORE computing priorities.
 *  - "0x8005b81c needs menu_base's value known inside the loop, yet deleting
 *    menu_base removes the copy" was a real contradiction, and `think_item =
 *    item` dissolves it: the copy's source and the cursor's source are the
 *    SAME value under TWO names, and the name each site reads is what decides
 *    both the hoist and the cascade.  0x81c is now exact.
 *
 * ROUND 7 — byte accounting CORRECTED, and (C) re-diagnosed from the gcc
 * source.  Round 6's cluster table summed to 48, not 42, and its per-cluster
 * counts were each wrong; it also claimed "0x81c is now exact" when 0x81c is
 * in the diff.  Byte-account with matchdiff before trusting any of this.
 * The REAL residual (verified: 14+6+8+2+12 = 42, so this is the whole of it):
 *   (A) 14 bytes, 0x5d4-0x5e0 (4 insns): loop-1 preheader ORDER.  Target
 *       [SA][-1][HD][WM][giv]; ours [SA][WM][-1][HD][giv].  Only the
 *       WeaponModel base pair is out of place.  Registers IDENTICAL.
 *   (B)  6 bytes, 0x620-0x624 (2 insns): `move s3,zero` / `sll v1,s4,0x10`
 *       swapped.  Registers IDENTICAL; a sched order pick.
 *   (C)  8 bytes, 0x81c-0x85c (7 insns): an $a0/$a1 swap AND a source-register
 *       difference.  Target `move a1,s0` reads $s0 (the loop-2 base); ours
 *       `move a0,s1` reads $s1 (`item`).  Round 6 fixed only the FORM.
 *   (D)  2 bytes, 0x884 (1 insn): `addu v0,s0,v0` vs ours `addu v0,v0,s0`.
 *   (E) 12 bytes, 0x8dc-0x8e4 (3 insns): `move a0,s7` is FIRST in the exit
 *       block; ours is third, after the CamState la.  Registers IDENTICAL.
 *
 * The la/extern-completion rule does NOT apply anywhere here (checked, round
 * 7): no differing byte is a lui/addiu differing only in destination reg.
 * 0x8e0/0x8e4 are the same-register (unsplit) form on BOTH sides and differ
 * only in position; 0x5d4-0x5e0's pair is split identically on both sides.
 * `.extern CamState, 36` is already complete.  autorules re-run on THIS
 * source (round 6 changed the program materially): 46 candidates, no win.
 *
 * (C) IS UNREACHABLE VIA ANY loop.c MOVABLE — proved from the gcc 2.8.1
 * source, not inferred.  loop.c:697-703 makes a set movable if ANY of:
 *     (3) ! maybe_never && ! loop_reg_used_before_p (...)
 *     (2) ! REG_USERVAR_P (SET_DEST) && ! REG_LOOP_TEST_P (SET_DEST)
 *     (1) reg_in_basic_block_p (p, SET_DEST)
 * and each is independently dead for the loop-2 base:
 *  - case (3): loop.c:922-931 sets `maybe_never = 1` past ANY CODE_LABEL *or*
 *    JUMP_INSN (only a loop-top simplejump is exempt) — NOT merely backward
 *    jumps, which is the half-rule that has been assumed here.  The
 *    `if (ThinkDB[0].name != 0)` branch therefore poisons everything after it.
 *  - case (2): needs a compiler temp.  MEASURED (round 7): deleting menu_base
 *    and using direct `ItemName[...]` gives 1144, and the .s shows why — the
 *    address is rematerialised as `addu $reg,$sp,1424` at ALL FOUR sites and
 *    never hoisted at all.  NEW RULE, worth generalising: **loop.c cannot
 *    hoist a frame-address invariant (`sp+K`).  cse/combine fold it into one
 *    cheap addiu per use, so there is no invariant SET for move_movables to
 *    move, and case (2) can never engage.  A frame address becomes a register
 *    ONLY as a NAMED user variable via case (1).**
 *  - case (1): reg_in_basic_block_p's first test is
 *    `REGNO_FIRST_UID (regno) != INSN_UID (insn) -> return 0`, so the set must
 *    be the reg's first mention in the WHOLE function — it forbids the scan
 *    reading the base.  CONFIRMED by measurement: `think_item = menu_base`
 *    alone is LENGTH 1156 (+1 insn, the hoist dies).
 * And the ORDER pins case (1) as the only survivor: the ThinkDB *base*
 * (lo_sum, $s6) is discovered at the first INDEXED `ThinkDB[i]` (0x830), which
 * is INSIDE the scan — the `ThinkDB[0]` guard folds its %lo into the load
 * (`lw $v0,%lo(ThinkDB)($fp)`) and creates only the *carrier* movable.  So the
 * base must be discovered after the scan to land 3rd in the preheader, i.e.
 * its set must sit after the scan, i.e. it can never be read inside the scan.
 * {hoisted 3rd in the preheader} AND {read inside the scan} is UNSATISFIABLE.
 *
 * SO 0x7f8's `move s0,s1` IS A GIV INIT, NOT A HOIST.  strength_reduce
 * (loop.c:6405) emits giv inits with emit_insn_before(loop_start) AFTER
 * move_movables has run — the ONLY mechanism that can legally land after every
 * hoist in a preheader (this file already proved it once, for loop 1's
 * `addu s7,s5,zero` at 0x5e4).  Two independent pieces of target evidence:
 *  - $s0 holds -1 at 0x7c0 (`addiu $s0,$zero,-0x1`, feeding the `type == -1`
 *    test at 0x7e4), so $s0 is BORN at 0x7f8 — it is not a pre-loop pointer.
 *  - 0x81c's `addu $a1,$s0,$zero` is the last insn of the INNER scan loop's
 *    preheader (0x810-0x81c: menu_char hoisted out of the scan, then the
 *    copy), immediately before .L8005B820 — the canonical giv-init slot, and
 *    the same `reg = reg` shape as loop 1's giv init.  The scan cursor is
 *    loop.c's giv of `count` (base + count*8, count==0 at entry), exactly as
 *    `names_offset` turned out to be in loop 1.
 * ROUND 8 should therefore ask what makes $s0 a GIV of the OUTER loop, not
 * where to put a `menu_base` statement.  Do NOT re-run fence-depth work on
 * (C) (round 6's own proof: item's allocno conflicts with hard regs 16 AND 17,
 * so it cannot hold $s1 at any priority) and do NOT re-site menu_base as a
 * movable (proved unsatisfiable above).
 *
 * (E) is likewise not a statement-order lever.  gcc 2.8's sched is a BACKWARD
 * list scheduler (dump reads T-1 = block END; only the block-end insn is ready
 * initially), and `rank_for_schedule` is priority DESC, then class, then
 * DESCENDING LUID.  In block 32 the CamState `high` (insn 1607) already has
 * priority 1 — the floor — so to be emitted first `move a0,s7` must be picked
 * last, needing either priority < 1 (impossible) or a LOWER LUID than 1607
 * (impossible: expand_call emits hard-reg arg moves only after every arg is
 * evaluated).  Reordering the tail statements cannot move it; (E) needs the
 * CamState chain to carry priority > 1, i.e. a different dependency shape.
 *
 * The stack is exact: names at sp+0x18, ItemName at sp+0x590, output VECTOR at
 * sp+0x7c0, and the zeroed blood VECTOR at sp+0x7d0.
 *
 * Explicit top-tested stage-kind, weapon, and think scans recover retail's
 * loop rotation.  StageAppearance and WeaponModel remain in t1/t0 and spill
 * at sp+0x7e4/sp+0x7e0 around sprintf.  A CFG join retains the weapon wid
 * reload.  i/s4 is reused across both menu scans and count/x share s5.
 * Narrow single-trip fences steer those live ranges but emit no instructions.
 *
 * Closed this pass (-20): THERE IS NO `names_offset` VARIABLE.  The names
 * cursor is loop.c's own strength-reduced GIV of `count`: `names_offset` and
 * `count` are updated in lockstep (both only on the success path, both from
 * 0), so `names_offset == count * 20` identically and the source is simply
 * `buffer = (char *)names[count]`.  Spelling it as a hand-written biv put
 * TWO things in the wrong place at once, and writing `names[count]` fixed
 * both for free:
 *  (1) its init.  A source `names_offset = 0` is a PRE-loop statement, and
 *      loop.c emits hoists with emit_insn_before(loop_start) — i.e. after
 *      every pre-loop statement — so the init could only ever precede the
 *      hoists.  Retail has it LAST in the preheader (`addu s7,s5,zero` at
 *      0x8005b5e4) because strength_reduce (loop.c:6405) emits giv inits
 *      before loop_start AFTER move_movables has already run.  A giv init is
 *      the ONLY thing that can legally sit after a hoist in a preheader.
 *  (2) its increment.  As a giv, `addiu s7,s7,20` is emitted at the biv's
 *      update point rather than mid-block, which stops sched1 floating it up
 *      to 0x8005b6fc and lets dbr fill sprintf's delay slot with it exactly
 *      as retail does (0x8005b738).  That alone was a 16-byte cluster.
 *
 * Closed this pass (-44 bytes), as ONE atomic three-part edit — each part
 * alone breaks the 288-insn length, which is why they were parked separately:
 *  (1) `names_offset` was a MEGA-PSEUDO: one s32 local doing two unrelated
 *      jobs (the names cursor `=0`/`+=20`, and the AdtSelect result reused as
 *      `type`).  Three reaching defs stop combine's num_sign_bit_copies from
 *      proving sign-extension, forcing a narrow on BreedLife's arg0.  Giving
 *      `type` its OWN single-assignment local drops that narrow (-1 insn).
 *      The earlier pass FUSED them deliberately to chase the demo symbol's
 *      "$s7 type"; the allocator lands both in s7 anyway once split, because
 *      the ranges are disjoint.  Forcing a register by source reuse is what
 *      created the conflict.
 *  (2) `think` is `s16` (PSX.SYM: `reg $s2 short think`), not a cast s32.
 *  (3) the update is `think = think | AdtSelect(...)` with NO cast at the
 *      assignment; the loop test and leSetEnemy arg narrow per-use.
 * (2)+(3) restore retail's OR-temp `move s2,v0` (+1 insn), which a previous
 * pass recorded as an un-forceable coalescing tie.  It is forceable: as a
 * HImode store the copy's SOURCE ($v0, the OR result) stays LIVE past the
 * copy because the comparison sign-extends from $v0 (`sll v0,v0,16`), not
 * from the accumulator.  optimize_reg_copy_1 only rewrites the producer when
 * the copy's source DIES at the copy, so it declines here.  Spelling the
 * accumulator differently never moved it; changing which value the compare
 * reads did.
 *
 * `think_base` was deleted (byte-neutral): retail's $s6 ThinkDB base is a
 * loop.c-hoisted invariant of the indexed `ThinkDB[i]` accesses, not a source
 * pointer, which is why the guard keeps its own `%lo(ThinkDB)($fp)` form.
 *
 * Closed this pass (-9): the think/category $s2/$s3 SWAP was an
 * `allocno_compare` inequality, computed from the dumps rather than guessed
 * (cookbook: "A pure callee-saved SWAP residual is an allocno_compare
 * inequality — add ballast").  `.lreg` gives
 *     think    (pseudo 90, reg/v:HI): 26 refs / 62 insns -> 4*26/62 = 16774
 *     category (pseudo 91, reg/v:HI): 24 refs / 54 insns -> 4*24/54 = 17778
 * and `.greg`'s allocation order shows it literally ("... 91 90 ..."):
 * category outranks think and takes $s2 first.  category was over-ballasted
 * by its OWN fence — flow.c weights every ref by loop_depth, so FENCE_10 on
 * `category = 0` bought ~10 weighted refs for a single set.  Dropping that one
 * site to FENCE_6 costs 4 weighted refs, puts think ahead, and emits nothing.
 * A fence's DEPTH is a tunable dial, not a boolean: prefer the smallest depth
 * that wins the inequality, and compute both priorities before touching it.
 *
 * CORRECTED (round 3 got this wrong, and it cost a round): the residual was
 * NOT "register naming only" and NOT one cause.  Byte-accounting the diff by
 * cluster is a 5-minute check that would have caught it: of the 72, a full 40
 * lay in loop 1 / the prologue with IDENTICAL registers on both sides,
 * differing only in ORDER — they could not possibly be caused by loop 2's
 * preheader.  Always attribute the bytes to clusters BEFORE adopting a
 * single-cause story; a story that explains a third of the residual is not a
 * theory of the residual.
 *
 * Remaining residual (52), byte-accounted EXACTLY (round 5; sums to 52, so it
 * is the whole residual, not a story about part of it):
 *   (A) 14 bytes, 0x5d4-0x5e0 (4 insns): loop-1 preheader ORDER only.
 *   (B)  6 bytes, 0x620-0x624 (2 insns): `move s3,zero` / `sll v1,s4,0x10`
 *        swapped.  Registers IDENTICAL; a scheduling order pick.
 *   (C) 32 bytes, 0x7e0-0x924 (13 insns): the loop-2 base/type/carrier
 *        rotation, driven by the preheader order.  See below.
 *
 * ROUND 5 DISPROOF — do not re-run the round-4/5 loop-1 theory.  The brief
 * asked "find retail's two extra long-lived values, because retail keeps
 * ELEVEN to our NINE and its bases sit in t0/t1 because they LOST".  That is
 * FALSE for this checkpoint, and byte-accounting shows it in five minutes:
 * cluster (A) has IDENTICAL registers on both sides.  Ours already emits
 *     lui v0,%hi(StageAppearance) / addiu $t1,... / li $s6,-1 /
 *     addiu $fp,$a0,%lo(HumanData) / lui v0,%hi(WeaponModel) / addiu $t0,... /
 *     addu $s7,$s5,$zero
 * i.e. kind_base ALREADY in $t1, weapon_base ALREADY in $t0, -1 in $s6,
 * HumanData in $fp, giv init last — retail's exact seven insns in retail's
 * exact registers.  The caller-saves are already present and the length is
 * already exact.  WE ALREADY KEEP ELEVEN.  There are no two missing
 * long-lived values.  The nine-vs-eleven deficit belongs only to the
 * *retail-shaped edit* (direct refs + direct guard), a different program that
 * drops the call_spill range-split and so lets the bases win callee-saved
 * regs.  Reapplying it trades a SOLVED allocation for an order fix (-4).
 * Loop 1 is an ORDER problem, and only an order problem.
 *
 * The loop-2 rotation: both preheaders hold the SAME three
 * instructions in a different ORDER (target 0x8005b7f0, block 22):
 *     target: lui s8 / addiu s6,s8,-25024 / move s0,s1
 *     ours:   move s0,s1 / lui s7         / addiu s1,s7,-25024
 * `move s0,s1` is insn 714, `menu_base = ItemName`, which cse2 rewrites to a
 * copy of `item` (pseudo 100, $s1) — and 100 DIES there.  Retail's base is
 * born BEFORE that death, so base and `item` overlap, the base cannot reuse
 * $s1, and it takes $s6; that pushes type $s6->$s7 and the %hi carrier
 * $s7->$s8.  Ours frees $s1 one insn early, so the base reuses it and no
 * cascade happens.  Fix the ORDER and all 72 bytes should follow.
 *
 * Why the order resists: loop.c emits the hoists (insn 1662 `high(ThinkDB)`
 * = carrier, insn 1664 `lo_sum` = base) with emit_insn_before(loop_start),
 * i.e. immediately before NOTE_INSN_LOOP_BEG and therefore AFTER every
 * pre-loop statement.  `menu_base = ItemName` is pre-loop source, so it can
 * only ever precede them unless sched1 swaps — and sched1 will not.
 *
 * NEW, and the reason a fence is never "zero-cost": in gcc 2.8
 * `sched_analyze_insn`, a NOTE_INSN_LOOP_BEG/END in mid-block is a FULL
 * scheduling barrier (it must be, or the loop_depth that weights reg_n_refs
 * would go wrong) — every following insn gains a dependency on all prior
 * sets.  So a do{}while(0) fence has TWO effects: ref ballast AND a hard
 * sched1 order pin.  Proof here: insn 1662's LOG_LINKS is `(insn_list 714)`,
 * a dependency with no data behind it, purely the FENCE_10's notes.  Fences
 * therefore freeze the emitted order of the statements they wrap.
 *
 * ROUND 5, loop 2: THE ORDER IS REACHABLE — proved, then reverted.  The lead
 * above is correct and it works.  Moving `menu_base = ItemName` from a
 * pre-loop statement to a statement INSIDE the do-while, placed after the
 * think scan (i.e. after the base's movable in body-scan order), and
 * initialising the cursor with `think_item = ItemName` instead of
 * `= menu_base`, emits (verified in the .s):
 *     addu $17,$18,$zero / lui $23,%hi(ThinkDB) / addiu $21,$23,%lo(ThinkDB)
 *     / addu $16,$19,$zero / addu $20,$0,$zero
 * against retail's
 *     addu $s3,$s2,$zero / lui $fp,%hi(ThinkDB) / addiu $s6,$fp,%lo(ThinkDB)
 *     / addu $s0,$s1,$zero / addu $s5,$zero,$zero
 * — the SAME five insns in the SAME order, menu_base landing in retail's $s0,
 * and the guard still reusing the carrier (`lw $2,%lo(ThinkDB)($23)`).  The
 * mechanism is loop.c:691-703 case (1), `reg_in_basic_block_p`: it is a
 * STANDALONE disjunct that does NOT require !maybe_never and does NOT care
 * that the dest is a user variable, so a NAMED `menu_base` hoists freely
 * provided every use sits in the set's own basic block.  (That is why the
 * "named user var blocks the hoist" rule below is only half the story: it
 * blocks via cases (2)/(3), and case (1) buys it back.)
 *
 * It measured 97, NOT a reduction, so it was reverted.  Two costs, both
 * diagnosed, and they are the whole of the next round's work:
 *  (1) `item`/`category` swap ($s1<->$s3) plus a knock-on rotation of count
 *      (s5->s4), i (s4->s3), j (s3->s5), base (s6->s5), carrier (fp->s7),
 *      type (s7->s6).  A pure allocno_compare inequality — the pattern this
 *      file already beat once at -9.  `.greg` order was
 *      `29 regs to allocate: 277 92 87 276 98 86 314 101 100 89 93 95 90 82
 *       80 88 96 83 99 85 91 94 140 120 270 372 141 81 265`.
 *  (2) 0x8005b81c: retail `move a1,s0` (think_item = a copy of menu_base) vs
 *      ours `addiu a1,sp,1424` — cse2 REMATERIALISES the frame address
 *      because it clears its table at the multi-predecessor loop head, so it
 *      never learns $s0 == sp+1424 inside the loop.
 * Deleting `menu_base` entirely (all uses direct `ItemName[...]`) does NOT
 * fix (2): the copy stops existing at all and the length goes to 1144 (-2).
 * So `move s0,s1` exists ONLY because a NAMED menu_base exists, yet retail's
 * 0x81c needs that name's value known INSIDE the loop.  Reconciling those two
 * is the open question; the order lever itself is solved and cheap to redo.
 *
 * NEW RULE (measured here, worth generalising): a do{}while(0) fence CANNOT
 * ballast a set that loop.c HOISTS.  move_movables emits the insn with
 * emit_insn_before(loop_start), which lifts it OUT of the fence's
 * NOTE_INSN_LOOP_BEG/END region, so it loses the loop_depth weighting
 * entirely.  Wrapping the relocated `menu_base = ItemName` in FENCE_10
 * changed nothing (97 -> 97).  Ballast only works on sets that STAY PUT.
 *
 * LOOP-1 PREHEADER, and the general rule it establishes.  Retail's order is
 *     [StageAppearance base] [-1] [HumanData base] [WeaponModel base] [giv]
 * and ours is
 *     [StageAppearance base] [WeaponModel base] [-1] [HumanData base] [giv]
 * i.e. the ONLY thing left out of place is the WeaponModel base.  Round 3
 * called this shape unreachable because loop.c emits every hoist after every
 * pre-loop statement.  That is true but it is the wrong lever: the fix is to
 * stop the value being a pre-loop statement at all.  move_movables emits
 * movables in BODY-SCAN DISCOVERY order, so a loop-invariant assignment
 * written INSIDE the body lands at its scan position.  Our body scan order is
 * already StageAppearance -> -1 -> HumanData -> WeaponModel, exactly retail's.
 * THE RULE (read this before touching loop 1 again — it is why a NAMED base
 * can never be hoisted).  loop.c:691-703 will only move a set if one of:
 *     (1) it is used only in the same basic block as the set, OR
 *     (2) `! REG_USERVAR_P (SET_DEST (set)) && ! REG_LOOP_TEST_P (...)`, OR
 *     (3) the set is guaranteed executed once the loop starts and the reg is
 *         not used until after that.
 * So **assigning a loop invariant to a NAMED USER VARIABLE blocks the hoist**
 * whenever the value is also used in another block and the set sits under a
 * conditional (`maybe_never`).  A compiler TEMP — what a direct indexed
 * reference like `WeaponModel[weapon].name` or `WeaponModel[0].wid` produces —
 * takes case (2) and hoists freely.  This one flag explains every loop-1
 * result, including two that look contradictory:
 *  - `weapon_base = WeaponModel;` written in the body: 1124 (-7).  User var,
 *    maybe_never, multi-block -> all three cases fail -> not hoisted at all.
 *  - direct refs but keeping `weapon_entry = WeaponModel;`: 1140 (-3).  The
 *    lo_sum's dest is the user var, so only the *temp* `high` hoists and each
 *    `%lo` is folded into its use.
 *  - direct refs AND a direct guard `if (WeaponModel[0].wid != -1)` (retail's
 *    own shape: `lh $v0,0x4($t0)` at 0x6a4, cursor `addu $v1,$t0,$zero` at
 *    0x6d0): the guard's indexed load creates the temp FIRST, cse1 then makes
 *    the later `weapon_entry = WeaponModel` a copy of it, and the whole lo_sum
 *    hoists.  **This reproduces retail's loop-1 preheader EXACTLY** — same 7
 *    insns, same order, giv init last.  Verified in the .s.
 *
 * That shape is 1136 (-4), and the -4 is fully diagnosed: it is exactly the
 * four caller-save insns (2 `sw` + 2 `lw`).  Our masks are IDENTICAL to
 * retail's (0xc0ff0000 = s0-s7+fp+ra), but retail keeps ELEVEN long-lived
 * values (those nine PLUS t0/t1) where we keep nine, so our two bases outrank
 * retail's and grab callee-saved regs instead of being pushed into caller-
 * saved t0/t1.  Retail's bases are in t0/t1 *because they lost*: `find_reg`
 * gave them no callee-saved reg, and caller-save.c then spilled them.
 * SUPERSEDED by the ROUND 5 DISPROOF above: this paragraph's premise ("our
 * bases outrank retail's and win callee-saved regs") is true ONLY of the
 * retail-shaped edit, never of this checkpoint, whose bases are already in
 * t0/t1.  Do not go looking for "two extra long-lived values" — we already
 * keep eleven.  Loop 1's 20 bytes are order, not pressure.  The real loop-1
 * question is the same one loop 2 answers: how to emit the WeaponModel base
 * as a movable discovered AFTER the -1/HumanData movables while KEEPING the
 * t0 allocation and the four caller-saves that the named-var shape earns.
 *
 * ALSO CONFIRMED, and it retires a piece of scaffolding: the two `sw` around
 * sprintf are gcc's CALLER-SAVE of the two bases, not source stores.  With
 * `blood.call_spill[]` deleted entirely, gcc spilled them on its own to
 * sp+0x7e0 / sp+0x7e4 — byte-identical slots to the hand-written hack, which
 * is why the hack ever looked right.  `call_spill` and the volatile-reload
 * if/else pair are therefore MODELLING A COMPILER PASS and must go with the
 * edit above; they are kept here only because the 52 checkpoint needs the
 * exact length and that edit is -4 until the allocation is solved.
 *
 * Rejected (do not repeat): folding the stage slot, narrowing the weapon id,
 * eliminating the weapon CFG join, the s8-next_j allocation hack (autorules
 * now scores it 1004 — a clear length regression, not the recorded -2), and
 * menu_base[count]->ItemName[count] (recomputes the base, +2).
 * Rejected THIS pass, each measured:
 *  - `menu_base = ItemName` moved to the loop-body TOP: 119.  loop.c hoists
 *    movables in discovery order, so at the top it is still found before the
 *    base's and the order does not flip.
 *  - `think_base` as a real source pointer for the indexed accesses: LENGTH
 *    +1 (1156).  It breaks the carrier SHARING — the `ThinkDB[0]` guard stops
 *    reusing the hoisted %hi and emits its own `lui v0` / `lw v0,-25024(v0)`
 *    where retail has one `lw v0,-25024(s8)`.  This is why the base must stay
 *    a loop.c-hoisted invariant, confirming the earlier note.
 *  - Unfencing `menu_base = ItemName` to drop the sched1 pin: 119.  The order
 *    does NOT flip (sched1 still keeps 714 first) and `item` loses $s1.
 *  - Relocating that ballast to FENCE_10(think_item = menu_base): 126.
 *  - autorules: no improving edit among 50 candidates.
 *  - Bounded permuter (420s, -j4, --stop-on-zero): no zero.
 * Rejected in ROUND 7, each measured on THIS source (not inherited claims):
 *  - `think_item = menu_base` (one word, everything else unchanged): LENGTH
 *    1156.  The case-(1) hoist dies exactly as predicted.  This is the first
 *    independent confirmation of round 6's rule; it holds.
 *  - Deleting `menu_base` again, all loop-2 uses direct `ItemName[...]`, on
 *    the round-6 structure (NOT a repeat of round 5 — the program changed):
 *    LENGTH 1144, same as round 5.  The result is robust, and the .s finally
 *    explains it: four `addu $reg,$sp,1424`, no hoist anywhere.
 *  - autorules on the round-6 source: 46 candidates, no improving edit.
 * Rejected in ROUND 5, each measured (see the loop-2 note above for detail):
 *  - `menu_base = ItemName` relocated below the think scan + `think_item =
 *    ItemName`: 97.  ORDER CORRECT (this is the advance), but it costs the
 *    item/category swap and 0x81c rematerialises.  Redo it only WITH a fix
 *    for those two; the shape itself is right.
 *  - FENCE_10 on that relocated statement, to restore item's lost ballast:
 *    97 (no change) — a hoisted set escapes its fence's notes.
 *  - Deleting `menu_base` entirely, all uses direct `ItemName[...]`: LENGTH
 *    1144 (-2).  The `move s0,s1` copy stops being emitted at all.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AddEnemy", AddEnemy);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/AddEnemy", debug_menu_enemy_layout_add__override__prt_8005b740_aee7b64a);
#else

#include "item.h"

typedef struct
{
    s16 type;
    s16 wepid;
    s16 turn;
    s16 life;
    s16 width;
    s16 height;
    MotionRegistType *mtbl;
    u8 *name;
    u32 *model;
} AddEnemyHumanData;

typedef struct
{
    u8 *name;
    s16 wid;
    u32 *model;
} AddEnemyWeaponModel;

typedef struct
{
    u8 *name;
    s16 value;
} AddEnemyThinkDB;

typedef struct
{
    VECTOR TargetVector;
    Humanoid *Owner;
    s32 Mode;
    s16 DirectionRX;
    s16 DirectionRY;
    s32 OldMode;
    u8 Valiation;
} AddEnemyCameraStatus;

/* Retail reserves the two words after the temporary VECTOR for the caller-
 * saved WeaponModel and StageAppearance bases around sprintf. */
typedef struct
{
    VECTOR vector;
    u32 call_spill[2];
} AddEnemyBloodScratch;

extern AddEnemyHumanData HumanData[63];
extern AddEnemyWeaponModel WeaponModel[41];
extern s16 *StageAppearance[];
extern s32 StageID;
extern AddEnemyThinkDB ThinkDB[20];
extern AddEnemyCameraStatus CamState;
extern s32 CurrentEnemyID;

extern char D_80013FA8[];
extern char D_80013FB4[];
extern char D_80097D48[];
extern char D_80097D50[];

extern s32 AdtSelect(char *title, debug_menu_choice *menu, s32 mode);
extern int sprintf(char *buf, char *fmt, ...);
extern void *memset(void *s, int c, u32 n);
extern s32 leSetEnemy(s32 type, s16 think, s32 x, s32 y, s32 z, s16 r);
extern Humanoid *BreedLife(s16 type, s32 x, s32 y, s32 z, s32 r);
extern void SetBleeds(VECTOR *pos, short grange, short srange, short n,
                      int time, long col);

#define ADD_ENEMY_FENCE_5(statement)                                           \
    do                                                                         \
    {                                                                          \
        do                                                                     \
        {                                                                      \
            do                                                                 \
            {                                                                  \
                do                                                             \
                {                                                              \
                    do                                                         \
                    {                                                          \
                        statement;                                             \
                    } while (0);                                                \
                } while (0);                                                    \
            } while (0);                                                        \
        } while (0);                                                            \
    } while (0)

#define ADD_ENEMY_FENCE_6(statement)                                           \
    do                                                                         \
    {                                                                          \
        ADD_ENEMY_FENCE_5(statement);                                          \
    } while (0)

#define ADD_ENEMY_FENCE_10(statement)                                          \
    do                                                                         \
    {                                                                          \
        do                                                                     \
        {                                                                      \
            do                                                                 \
            {                                                                  \
                do                                                             \
                {                                                              \
                    ADD_ENEMY_FENCE_6(statement);                              \
                } while (0);                                                    \
            } while (0);                                                        \
        } while (0);                                                            \
    } while (0)

/* An allocator fence.  The nested single-trip loops increase the source
 * weight of a live range (flow.c weights every ref by loop_depth) without
 * surviving jump2.  Zero-code but NOT side-effect-free: the loop notes are
 * also a hard sched1 barrier (gcc 2.8 sched_analyze_insn refuses to schedule
 * across a NOTE_INSN_LOOP_BEG/END, else loop_depth would be wrong), so a
 * fence pins the emitted order of the statement it wraps.  The depth is a
 * dial — use the smallest that wins the allocno_compare inequality. */
#define ADD_ENEMY_FENCE_16(statement)                                          \
    do                                                                         \
    {                                                                          \
        do                                                                     \
        {                                                                      \
            do                                                                 \
            {                                                                  \
                do                                                             \
                {                                                              \
                    do                                                         \
                    {                                                          \
                        do                                                     \
                        {                                                      \
                            ADD_ENEMY_FENCE_10(statement);                     \
                        } while (0);                                            \
                    } while (0);                                                \
                } while (0);                                                    \
            } while (0);                                                        \
        } while (0);                                                            \
    } while (0)

#define ADD_ENEMY_FENCE_19(statement)                                          \
    do                                                                         \
    {                                                                          \
        do                                                                     \
        {                                                                      \
            do                                                                 \
            {                                                                  \
                ADD_ENEMY_FENCE_16(statement);                                 \
            } while (0);                                                        \
        } while (0);                                                            \
    } while (0)

void AddEnemy(void)
{
    u8 names[70][20];
    debug_menu_choice ItemName[70];
    VECTOR pos;
    AddEnemyBloodScratch blood;
    s32 count;
    s32 type;
    s16 i;
    s16 j;
    s16 next_j;
    s32 human_type;
    s32 weapon;
    s32 weapon_id;
    s32 human_weapon_id;
    s16 think;
    s16 category;
    s32 stage;
    s32 stage_slot;
    s32 menu_char;
    s16 **kind_base;
    s16 *stage_kinds;
    AddEnemyWeaponModel *weapon_base;
    AddEnemyWeaponModel *weapon_entry;
    AddEnemyWeaponModel *weapon_scan;
    debug_menu_choice *item;
    debug_menu_choice *menu_base;
    debug_menu_choice *think_item;
    char *buffer;
    char *format;
    char *human_name;
    char *weapon_name;
    ModelArchiveType *model;
    Humanoid *human;
    s32 y;
    s32 z;
    s16 r;

    count = 0;
    i = count;
    if (HumanData[0].type != -1)
    {
        kind_base = StageAppearance;
        weapon_base = WeaponModel;
        while (HumanData[i].type != -1)
        {
            if (count >= 70)
                break;

#define ADD_ENEMY_STAGE_KINDS \
    (kind_base[stage_slot])
            stage = StageID;
            /* Keep the stage+1 slot named: folding it into the array access
             * rotates the following scan away from retail's CFG. */
            stage_slot = stage + 1;
            if (ADD_ENEMY_STAGE_KINDS[0] != -1)
            {
                j = 0;
                human_type = HumanData[i].type;
                /* goto-loop (a real do-while's notes would let loop.c hoist
                 * the stage_slot arithmetic that retail recomputes each
                 * iteration).  next_j is assigned AFTER the type test and
                 * both exit arms update j, so jump1's common-code hoist
                 * puts the single j=next_j move between the sentinel load
                 * and the branch: next_j stays block-local, lreg hands it
                 * v0 with the address temp in v1 (a next_j spanning the
                 * type branch goes to greg and lands the pair swapped),
                 * and dbr fills both delay slots as retail. */
add_enemy_stage_scan:
                stage_slot = stage + 1;
                stage_kinds = ADD_ENEMY_STAGE_KINDS;
                if (stage_kinds[j] == human_type)
                    goto add_enemy_stage_scan_done;
                next_j = j + 1;
                if (stage_kinds[next_j] != -1)
                {
                    j = next_j;
                    goto add_enemy_stage_scan;
                }
                j = next_j;
add_enemy_stage_scan_done:

                if (stage_kinds[j] != -1)
                {
                    weapon = 0;
                    weapon_entry = weapon_base;
                    if (weapon_entry->wid != -1)
                    {
                        human_weapon_id = HumanData[i].wepid;
                        /* Three interlocking pieces recover retail's cursor
                         * init (`move v1,t0` after the wepid load, reload
                         * kept):
                         *  - jump1's diamond transform always hoists the
                         *    ELSE copy above the branch; the empty loop-note
                         *    barrier then stops sched1 from floating that
                         *    copy above the wepid chain (floated, the
                         *    cursor's range covers the chain temps and
                         *    cursor/wepid rotate onto a0/a1 instead of
                         *    retail's v1/a0);
                         *  - the arms must LOOK unequal (weapon_entry vs
                         *    weapon_base — the same value) or jump1 deletes
                         *    the conditional copy as redundant, the join
                         *    label dies, and cse folds the wid reload into
                         *    a copy of the guard value;
                         *  - the multi-pred join label makes cse drop its
                         *    table, keeping retail's second wid load.
                         * jump2 erases the branch and the arm copy once
                         * regalloc has made them byte-identical.
                         * The test must be `count` — an s32 already homed in
                         * a callee-saved reg (plain bnez, no temp).  `i != 0`
                         * would reuse the chain's (s16)i temp and extend v1's
                         * hard liveness over the cursor/wepid births, which
                         * is exactly the conflict that rotated them onto
                         * a0/a1. */
                        do
                        {
                        } while (0);
                        if (count != 0)
                            weapon_scan = weapon_entry;
                        else
                            weapon_scan = weapon_base;
                        weapon_id = weapon_scan->wid;
add_enemy_weapon_scan:
                        if (weapon_id == human_weapon_id)
                            goto add_enemy_weapon_scan_done;
                        weapon_scan++;
                        weapon_id = weapon_scan->wid;
                        ADD_ENEMY_FENCE_10(weapon++);
                        if (weapon_id != -1)
                            goto add_enemy_weapon_scan;
                    }
add_enemy_weapon_scan_done:

                    buffer = (char *)names[count];
                    format = D_80097D48;
                    human_name = (char *)HumanData[i].name;
                    weapon_name = (char *)weapon_base[weapon].name;
                    blood.call_spill[0] = (u32)weapon_base;
                    blood.call_spill[1] = (u32)kind_base;
                    sprintf(buffer, format, human_name, weapon_name);
                    ItemName[count].choice_name = buffer;
                    ItemName[count].choice_number = HumanData[i].type;
                    count++;
                    /* Volatile reads retain the two reloads; the ordinary
                     * stores remain schedulable into sprintf's delay slot. */
                    if (j != 0)
                        kind_base =
                            (s16 **)*(volatile u32 *)&blood.call_spill[1];
                    else
                        kind_base =
                            (s16 **)*(volatile u32 *)&blood.call_spill[1];
                    if (weapon != 0)
                        weapon_base =
                            (AddEnemyWeaponModel *)*(volatile u32 *)
                                &blood.call_spill[0];
                    else
                        weapon_base =
                            (AddEnemyWeaponModel *)*(volatile u32 *)
                                &blood.call_spill[0];
                }
            }
            i++;
        }
#undef ADD_ENEMY_STAGE_KINDS
    }

    item = ItemName;
    ItemName[count].choice_name = D_80097D50;
    ItemName[count].choice_number = -1;
    count++;
    ItemName[count].choice_name = 0;
    /* `type` is its OWN single-assignment local, not a reuse of the names
     * cursor's range: one sign-extended reaching def lets combine's
     * num_sign_bit_copies elide BreedLife's arg0 narrow.  The allocator still
     * lands it in retail's s7 because the two ranges are disjoint. */
    type = (s16)AdtSelect(D_80013FA8, item, 0);
    if (type == -1)
        return;

    think = 0;
    ADD_ENEMY_FENCE_6(category = 0);
    do
    {
        count = 0;
        /* Reusing the first scan's i range restores retail's s4 assignment. */
        ADD_ENEMY_FENCE_16(i = count);
        /* Wrapping the guard in a single do{}while(0) walls off a copy the
         * local allocator would otherwise propagate through the think scan,
         * restoring retail's register choices there (zero-code fence). */
        do
        {
            if (ThinkDB[0].name != 0)
            {
                /* Retail emits menu_char (the compared constant) before the
                 * think_item cursor, which lands them in $a2/$a1 as the
                 * target does. */
                ADD_ENEMY_FENCE_6(menu_char = (s16)category + 0x31);
                ADD_ENEMY_FENCE_19(think_item = item);
add_enemy_think_scan:
                if (count >= 70)
                    goto add_enemy_think_scan_done;
                if (menu_char == ThinkDB[i].name[0])
                {
                    think_item->choice_name = (char *)ThinkDB[i].name;
                    think_item->choice_number = ThinkDB[i].value;
                    think_item++;
                    count++;
                }
                i++;
                if (ThinkDB[i].name != 0)
                    goto add_enemy_think_scan;
            }
        } while (0);
add_enemy_think_scan_done:
        menu_base = ItemName;
        ADD_ENEMY_FENCE_10(menu_base[count].choice_name = 0);
        ADD_ENEMY_FENCE_10(
            think = think | AdtSelect(D_80013FB4, menu_base, 0));
    } while (think != 0x1111 && think != 0x2222 &&
             ++category < 4);

    model = CamState.Owner->model;
    /* count is dead here and shares retail's s5 range with x. */
    count = model->locate.coord.t[0];
    y = model->locate.coord.t[1];
    r = model->rotate.vy;
    z = model->locate.coord.t[2];
    CurrentEnemyID =
        leSetEnemy(type, think, count, y, z, r);
    human = BreedLife(type, count, y, z, 0);
    human->model->rotate.vy = r;
    human->target = (ModelType *)CamState.Owner->model;

    memset(&blood.vector, 0, sizeof(VECTOR));
    blood.vector.vx = human->model->locate.coord.t[0];
    blood.vector.vy = human->model->locate.coord.t[1] - 1200;
    blood.vector.vz = human->model->locate.coord.t[2];
    pos = blood.vector;
    SetBleeds(&pos, 400, 0, 50, 30, 0xffffff);
}

#undef ADD_ENEMY_FENCE_16
#undef ADD_ENEMY_FENCE_10
#undef ADD_ENEMY_FENCE_6

#endif /* NON_MATCHING */

// triage: HARD — 288 insns, 5 loop, frame 0x810, 6 callees, ~0.06 to GetMotionID
// likely-relevant cookbook sections:
//   - Loops: 5 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Stack objects: 0x810 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void AddEnemy(void)
//
// {
//   short sVar1;
//   short sVar2;
//   int iVar3;
//   ulong uVar4;
//   ModelArchiveType *pMVar5;
//   WeaponModelType *pWVar6;
//   byte *pbVar7;
//   int iVar8;
//   TAdtSelect *pTVar9;
//   WeaponModelType *pWVar10;
//   undefined *puVar11;
//   long z;
//   char *buffer;
//   uint uVar12;
//   long y;
//   int iVar13;
//   int iVar14;
//   int iVar15;
//   long x;
//   int iVar16;
//   char acStack_7f8 [1400];
//   TAdtSelect local_280;
//   char *local_278 [139];
//   int local_4c;
//   undefined4 local_48;
//   undefined4 local_44;
//   undefined4 local_40;
//   int local_3c;
//   undefined4 local_38;
//   undefined4 local_34;
//   WeaponModelType *local_30;
//   undefined *local_2c;
//
//   iVar14 = 0;
//   sVar2 = 0;
//   if (HumanData[0].type != -1) {
//     puVar11 = &DAT_8008f188;
//     pWVar10 = WeaponModel;
//     iVar16 = 0;
//     iVar13 = iVar14;
//     do {
//       iVar14 = iVar13;
//       if (0x45 < iVar13) break;
//       if (**(short **)(puVar11 + (StageID + 1) * 4) != -1) {
//         iVar15 = 0;
//         do {
//           iVar8 = *(int *)(puVar11 + (StageID + 1) * 4);
//           iVar3 = iVar15 + 1;
//           if (*(short *)(((iVar15 << 0x10) >> 0xf) + iVar8) == HumanData[sVar2].type) break;
//           iVar15 = iVar3;
//         } while (*(short *)((iVar3 * 0x10000 >> 0xf) + iVar8) != -1);
//         if (*(short *)(((iVar15 << 0x10) >> 0xf) + iVar8) != -1) {
//           iVar14 = 0;
//           if (pWVar10->wid != -1) {
//             sVar1 = pWVar10->wid;
//             pWVar6 = pWVar10;
//             do {
//               if (sVar1 == HumanData[sVar2].wepid) break;
//               sVar1 = pWVar6[1].wid;
//               iVar14 = iVar14 + 1;
//               pWVar6 = pWVar6 + 1;
//             } while (sVar1 != -1);
//           }
//           buffer = acStack_7f8 + iVar16;
//           iVar16 = iVar16 + 0x14;
//           local_30 = pWVar10;
//           local_2c = puVar11;
//           sprintf(buffer,s__s__s_80097d48,HumanData[sVar2].name,pWVar10[iVar14].name);
//           local_278[iVar13 * 2 + -2] = buffer;
//           iVar14 = iVar13 + 1;
//           local_278[iVar13 * 2 + -1] = (char *)(int)HumanData[sVar2].type;
//           pWVar10 = local_30;
//           puVar11 = local_2c;
//         }
//       }
//       sVar2 = sVar2 + 1;
//       iVar13 = iVar14;
//     } while (HumanData[sVar2].type != -1);
//   }
//   (&local_280)[iVar14].name = s_cancel_80097d50;
//   local_278[iVar14 * 2 + -1] = (char *)0xffffffff;
//   (&local_280)[iVar14 + 1].name = (char *)0x0;
//   uVar4 = AdtSelect("select type",&local_280,0);
//   iVar14 = (int)(short)uVar4;
//   uVar12 = 0;
//   if (iVar14 != -1) {
//     iVar13 = 0;
//     do {
//       iVar15 = 0;
//       iVar16 = 0;
//       if (ThinkDB[0].name != (uchar *)0x0) {
//         pTVar9 = &local_280;
//         do {
//           if (0x45 < iVar15) break;
//           iVar3 = (iVar16 << 0x10) >> 0xd;
//           pbVar7 = *(byte **)((int)&ThinkDB[0].name + iVar3);
//           if ((int)(short)iVar13 + 0x31U == (uint)*pbVar7) {
//             pTVar9->name = (char *)pbVar7;
//             iVar15 = iVar15 + 1;
//             pTVar9->value = (int)*(short *)((int)&ThinkDB[0].value + iVar3);
//             pTVar9 = pTVar9 + 1;
//           }
//           iVar16 = iVar16 + 1;
//         } while (*(int *)((int)&ThinkDB[0].name + (iVar16 * 0x10000 >> 0xd)) != 0);
//       }
//       (&local_280)[iVar15].name = (char *)0x0;
//       uVar4 = AdtSelect("custom think setting",&local_280,0);
//       uVar12 = uVar12 | uVar4;
//       sVar2 = (short)uVar12;
//     } while (((sVar2 != 0x1111) && (iVar13 = iVar13 + 1, sVar2 != 0x2222)) &&
//             (iVar13 * 0x10000 >> 0x10 < 4));
//     pMVar5 = (CamState.Owner)->model;
//     x = (pMVar5->locate).coord.t[0];
//     y = (pMVar5->locate).coord.t[1];
//     sVar1 = (pMVar5->rotate).vy;
//     z = (pMVar5->locate).coord.t[2];
//     DAT_80097d44 = leSetEnemy(iVar14,sVar2,x,y,z,sVar1);
//     iVar14 = BreedLife(iVar14,x,y,z,0);
//     *(short *)(*(int *)(iVar14 + 0x58) + 0x52) = sVar1;
//     *(ModelArchiveType **)(iVar14 + 0x74) = (CamState.Owner)->model;
//     memset((uchar *)&local_40,'\0',0x10);
//     local_278[0x8a] = *(char **)(*(int *)(iVar14 + 0x58) + 0x18);
//     local_4c = *(int *)(*(int *)(iVar14 + 0x58) + 0x1c) + -0x4b0;
//     local_48 = *(undefined4 *)(*(int *)(iVar14 + 0x58) + 0x20);
//     local_44 = local_34;
//     local_40 = local_278[0x8a];
//     local_3c = local_4c;
//     local_38 = local_48;
//     SetBleeds((short)local_278 + 0x228,400,0);
//   }
//   return;
// }
