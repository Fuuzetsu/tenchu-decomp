#include "common.h"
#include "main.exe.h"
#include "gte.h"

/*
 * FUN_80057b80 (0x80057b80, 3796 bytes) — recursive quad/triangle subdivision
 * renderer of the DrawTMD handler family. Compiled-style C using the PsyQ
 * inline-GTE macros (gte_ldv3/gte_rtpt/gte_stsxy3/gte_stsz3) at two RTPT sites.
 *
 * STATUS: NON_MATCHING — 506 of 3796 bytes differ. LENGTH IS EXACT (945
 * captured instructions / 949 words, no knock-on shift to the rest of the
 * image).
 *
 * ROUND 5 CLOSED THE $s0/$s1 SWAP (619 -> 506). param_1 now lands in $s0 and
 * param_2 in $s1, as in the target, via the `do{}while(0)` fence around the
 * LEAF vertex-copy block (see ROUND 5 at the bottom — read that first). Every
 * other pseudo disposition is byte-identical to the pre-fence draft. The
 * residual is now ONLY the four sites A/B/C/D-adj described under "ROUND 3".
 *
 * The `lwc2; nop; nop; RTPT` shape at both command sites (0x80058320,
 * 0x80058548) is confirmed from the raw image bytes, so the nop-carrying
 * `gte_rtpt()` is correct here (this is a COMPILED caller — docs/gte-policy.md).
 * Note both dumps elide the paired zero words as objdump `...` runs, so the
 * captured-instruction counts (945) each omit 4 real nops; 945+4 = 949 words
 * = 3796 bytes.
 *
 * The body reproduces Ghidra's logic exactly. Two source-structure findings got
 * the register allocation to match almost entirely:
 *   1. local_30 must be computed as an independent `param_1 + 0x22`, NOT copied
 *      from piVar13: the copy carries a register-preference edge that makes cc1
 *      coalesce the two into one register, losing the target's 0x10(sp) spill.
 *   2. Each midpoint block writes its fields THROUGH its own SVECTOR pointer
 *      (r0/r1_00/r2_01/r1/r2_02) after the first store establishes the base —
 *      writing them all as `param_1 + offset` starves the midpoints of refs, so
 *      they score below piVar13 and steal its register.
 * With both, s2..s8 reference counts match the target exactly (20/17/18/13/18/
 * 18/19) and piVar13 correctly lands in $fp.
 *
 *   3. `sVar1` is a SIGNED `short`, not a `u16` (Ghidra had it right). The two
 *      reads of the same halfword — `sVar1 = *(short *)(p + 0xc)` and
 *      `uVar2 = *(u16 *)(p + 0xc)` — are DIFFERENT modes, so cc1 emits two
 *      loads (`lh` + `lhu`) and does not CSE them. Declaring sVar1 as `u16`
 *      collapsed both to one `lhu` plus a redundant `andi rX,rY,0xffff` copy,
 *      and forced the `(short)` compare operands to be sign-extended in-register
 *      with `sll`/`sra` instead of reloaded with `lh`. That cost exactly the two
 *      surplus instructions; fixing the declaration made the length exact.
 *
 * ---------------------------------------------------------------------------
 * ROUND 3 — WHAT CHANGED (759 -> 619) AND THE ONE THING LEFT.
 *
 * THE ROUND-3 WIN (the reusable rule, worth 140 bytes here):
 *   GHIDRA REUSES ONE VARIABLE FOR MANY UNRELATED JOBS; EACH SUCH VARIABLE IS
 *   ONE PSEUDO, AND ONE PSEUDO GETS ONE HARD REGISTER FOR ALL ITS FRAGMENTS.
 *   A conflict in ANY fragment therefore exiles EVERY use of it. Ghidra's
 *   `iVar3` did ~12 jobs (two pointer merges, three z-compares, the leaf prim
 *   pointer, the four recursive divisions, the final param_2[5] merge) and the
 *   whole lot landed in $a3 — 70 mentions, where the target's $a3 has 5.
 *   Same for puVar4/puVar5/iVar6/iVar8/uVar2, reused across all four recursive
 *   blocks. Splitting them into per-site / per-block locals: 759 -> 722 -> 619.
 *
 *   HOW IT WAS FOUND (do this first on any big function): histogram the
 *   REGISTER MENTIONS of target vs draft. a2/a3 were 25/5 in the target but
 *   72/70 in the draft — a caller-saved register carrying 70 mentions across
 *   the whole function is a mega-pseudo, and no real source has one. The proof
 *   that these are separate source variables is direct: the target assigns
 *   `puVar4 = param_2[5]` to a1,a1,a1,a2 across the four blocks, and a single
 *   pseudo can only ever get ONE register.
 *
 *   Two downstream effects came free, which is why this is worth more than the
 *   register names: (a) v0 went 657 -> 695 (target 692) and a0/a3/t0/s2..s7 now
 *   match EXACTLY; (b) the four `sra` in the recursive-division delay slots
 *   disappeared. Those were never a source bug — the target's value lives in
 *   v0, so `sra v0,v0,2` would clobber its own input and the delay slot must
 *   stay `nop`; ours lived in a3, so the filler could speculate it in. A
 *   delay-slot/scheduling difference can be a pure CONSEQUENCE of register
 *   allocation — fix the allocation, not the schedule.
 *
 * Align on MNEMONICS ONLY to see the real structure (a plain per-index diff
 * desynchronises after the first insert/delete, and difflib cannot align
 * across the s0/s1 swap). The four open sites, all confirmed against the
 * target asm:
 *
 *   A. MISSING (-1) `addiu a2,param_2,76` at target index 25 (0x80057be0).
 *      PROVEN: the target keeps a base pointer to `param_2 + 0x13` and the
 *      LEAF block reads all three of its fields through it —
 *      `lhu v0,14(a2)`/`lhu v0,26(a2)`/`lw v0,0(a2)` = param_2 + 90/102/76.
 *      The four RECURSIVE blocks instead read 90/102 directly off param_2 in
 *      BOTH builds (identical indices 683/686/757/760/831/834/905/908), so the
 *      pointer is leaf-only. Offsets 76/90/102 all fit a signed 16-bit
 *      immediate, so cc1 would never invent the base — it is source structure.
 *      Adding `int *proto = param_2 + 0x13;` at the top and spelling the leaf's
 *      three reads through it reproduces `addiu a2,s0,76` at EXACTLY index 25
 *      and the leaf reads at 274-282. It costs +1 (-> 946 insns = 3800 bytes),
 *      which OVERFLOWS the 3796-byte carve and makes matchdiff refuse to score,
 *      so it can only land together with a payback from site B/C below.
 *      Bonus: it also drops param_2 from 104 to 102 refs (see D).
 *
 *   B/C. SURPLUS (+1 each) at 0x80057d00 (X box, param_2+44) and 0x80057df4
 *      (Y box, param_2+46). We emit `sll`/`sra` where the target emits one
 *      `lh` reload.
 *   D-adjacent. MISSING (-1) the `lw param_2[7]` reload at 0x80057c14.
 *
 *      A, B and C share ONE root cause, established from the RTL dumps: cc1's
 *      expander emits `movhi` + `ashift` + `ashiftrt` carrying a
 *      `REG_EQUAL (sign_extend:SI (mem:HI ...))` note, and COMBINE normally
 *      folds that triple back into a single `lh`. At the first read after a
 *      store to the same address IN THE SAME BASIC BLOCK, cse's store-to-load
 *      forwarding has already replaced the movhi's source MEM with the stored
 *      register, so combine has no load left to fold and the sll/sra survive.
 *      Later reads (different block, cse table cleared) keep the movhi and DO
 *      become `lh` — see the draft .s, where param_2+44 is read as sll/sra once
 *      and as `lh` twice. The target reloads at ALL of them.
 *      The same forwarding explains the missing `lw param_2[7]` reload.
 *
 *      MECHANISM CONFIRMED, spelling NOT yet found: routing the STORE through
 *      an alias assigned in a different basic block (`int *ctx = param_2;` at
 *      the top, then `ctx[7] = ...`) makes cse unable to prove the addresses
 *      equal and the `lw param_2[7]` reload appears exactly where the target
 *      has it. But the store then uses ctx's own register (`sw v0,28(a1)`)
 *      where the target uses param_2's (`sw v0,28(s1)`) — ctx and param_2 are
 *      both live so they never coalesce. The target performs the store AND the
 *      reload through the same base register, so the real source blocks the
 *      forwarding some other way. Do not ship the ctx alias.
 *
 *   D. THE WHOLE REMAINING RESIDUAL: param_1/param_2 swapped between $s0/$s1.
 *          p80 param_1: 74 refs / 1318 live -> 3368
 *          p81 param_2: 104 refs / 1470 live -> 4244   (102 -> 4157 with proto)
 *      p81 outranks p80, so param_2 takes $s0 first; the target has param_1
 *      there. VERIFIED against the pinned gcc-2.8.1 source
 *      (/nix/store/117i80brbgcdmcl46gmpzwizikbjyx5m-gcc-2.8.1.tar.gz):
 *        - global.c allocno_compare is exactly regalloc.py's model:
 *          pri = (floor_log2(n_refs)*n_refs / live_length) * 10000 * size,
 *          sorted priority-descending, ties broken by LOWER allocno number.
 *        - MIPS does NOT define REG_ALLOC_ORDER, so find_reg scans plain
 *          register order and the first free callee-saved reg is $s0 (16).
 *        - prune_preferences() strips the a0/a1 copy preferences from any
 *          allocno that crosses a call, so NO preference biases this — the
 *          sort order alone decides who gets $s0.
 *
 *      CORRECTION TO ROUND 2: "just EQUALISE the priorities, ties break to the
 *      lower allocno" IS NOT A CHEAPER LEVER — it is arithmetically empty.
 *      pri80(N) = int(60000*N/1318) steps ~45 per ref: N=93 -> 4234 (lose),
 *      N=94 -> 4279 (win). No N lands on 4244, so the tie-break never fires.
 *      The ask is simply pri80 >= pri81, i.e. UNCHANGED at +20 refs.
 *
 *      AND THE REF LEVER IS EXHAUSTED — this is the round-3 dead end, stated
 *      precisely so round 4 does not re-walk it. reg_n_refs is counted by
 *      flow.c immediately before global_alloc, so it tracks the FINAL asm
 *      mention count (our draft: 74 refs/76 mentions and 104 refs/106
 *      mentions — a constant +2). The TARGET's asm mentions param_1 76 times
 *      and param_2 107 times. Our draft after the four fixes lands on exactly
 *      those numbers (106 +1 D-adj +1 B +1 C -3 leaf-reads-via-proto +1 proto
 *      = 107). So the target's own p80 is ~74 refs, NOT the ~94 the priority
 *      needs. param_1 CANNOT be given +20 refs and still emit the target's
 *      asm.
 *
 *      => The flip must come from LIVE_LENGTH, not refs. Required:
 *         L80 <= L81 * 74/105 ~= 1036 (we are at 1318; ~-280).
 *      That means param_1 must DIE ~280 insns earlier than in our draft.
 *      Unresolved tension for round 4: the target still reads param_1[1] off
 *      $s0 at 0x8005891c, deep in the 4th recursive block, so a naive
 *      first-use..last-use span gives L80/L81 ~= 0.9, not the needed 0.70.
 *      Either param_1 is DEAD across a long stretch our draft keeps it live
 *      (flow.c counts live_length as insns-live + insns-set, accumulated only
 *      where actually live, so a branch that never mentions param_1 costs it
 *      nothing), or regalloc.py's live_length is read from an earlier pass
 *      than the one global_alloc uses — note 1470 > 949, so the parsed dump is
 *      pre-combine RTL. RESOLVE THAT FIRST: confirm which dump regalloc.py
 *      parses vs which life_analysis global_alloc consumes. If they differ,
 *      the +20 number itself is suspect and the swap may fall to a much
 *      smaller change.
 *
 * Aligned operand residual: 658/949 instructions match as-is; 774/949 match
 * under a mechanical s0<->s1 rename. Of the 175 that do not, most are
 * MISALIGNMENT artifacts of the four sites above (an index-aligned compare
 * desynchronises from the missing `move s8,t0` at index 7 until `addiu
 * a2,s1,76` at index 24 resyncs it, and again from the missing reload at 37
 * to the B site at 98) — the mnemonic alignment shows only those four real
 * regions, so the swap plus the four sites is plausibly the entire residual.
 *
 * Do NOT re-run / re-derive (all spent, all negative):
 *   - autorules (46 candidates, no win) and the permuter (refuses gte.h
 *     functions outright — its parser cannot read inline asm).
 *   - `proto` (finding A) as a swap lever: measured on the ROUND-3 draft it
 *     moves p81 104->102 refs and 4244->4157, still needing +18. It does NOT
 *     flip the swap, and alone it costs +1 insn (3800 > the 3796 carve), so it
 *     can only ever land packaged with a B/C payback.
 *   - "equalise the priorities" (see the CORRECTION under D).
 *   - the `do{}while(0)` fence as a swap lever: it DOES flip p80/p81 (ROUND 4
 *     note 3) but every region costs +1 nop. Do not re-derive the flip; only
 *     revisit if a -1 payback exists to pair with it.
 *   - "regalloc.py reads the wrong dump" (ROUND 4 note 1: it does not).
 *
 * WARNING for round 4: every register-allocation finding above numbered 1-3
 * was established on the PRE-SPLIT draft. The mega-pseudo split rewrote the
 * conflict graph, so re-verify rather than assume — in particular finding 1
 * (local_30 must not be copied from piVar13) has NOT been re-tested since.
 *
 * ---------------------------------------------------------------------------
 * ROUND 4 — the swap lever is FOUND and PROVEN, but costs +1 nop. Still 619.
 *
 * 1. regalloc.py IS CORRECT — do NOT "fix" it. The suspicion that its
 *    live_length (1470 for a 949-insn function) is a stale PRE-COMBINE read is
 *    DISPROVEN. `dump_conflicts` prints `;; N regs to allocate:` in
 *    allocno_order, i.e. the real POST-qsort order — ground truth for any
 *    priority model. Scoring the 54 listed allocnos against that order:
 *      .lreg live_length (p80 1318 / p81 1470):  0 violations / 53 pairs
 *      .flow live_length (p80  763 / p81  838): 15 violations / 53 pairs
 *    The .lreg table is exactly what global_alloc consumes. live_length is
 *    simply NOT bounded by the final instruction count (it is not a naive
 *    per-insn count of the final chain), so "1470 > 949" is a red herring.
 *    REUSABLE: that `regs to allocate:` line is a free oracle — regalloc.py
 *    currently parses it as a SET and throws the ORDER away.
 *
 * 2. `reg_n_refs` IS LOOP-DEPTH WEIGHTED — flow.c adds one unit of weight per
 *    enclosed reference per loop depth (regalloc.py's own `--enclosed-refs`
 *    models this). So refs do NOT track asm mentions, and "the ref lever is
 *    exhausted because the target only mentions param_1 76x" is UNSOUND
 *    REASONING. It happens to hold here only because this function has no
 *    loops (verified: 0 real backward branches; the four that a naive scan
 *    finds are the recursive `jal FUN_80057b80` branching back to the entry
 *    glabel — a FALSE POSITIVE, exclude calls). Target mentions, for the
 *    record: s0=76 s1=107 s2=20 s3=17 s4=18 s5=13 s6=18 s7=18 fp=19.
 *
 * 3. THE FENCE LEVER WORKS AND FLIPS THE SWAP. A `do{}while(0)` emits no code
 *    but DOES leave the loop notes flow.c weights by. Wrapping ~20 param_1
 *    refs one level deeper takes p80 74 -> 94 refs, priority 3368 -> 4279 >
 *    p81's 4244, and the allocation flips to the target's p80->s0 / p81->s1.
 *    Verified at the cc1 level: `move $16,$4` / `move $17,$5` (vs the base's
 *    `move $17,$4` / `move $16,$5`), and the .s is the SAME LENGTH (766 = 766)
 *    — the flip is FREE inside the compiler. Every region tried flipped it:
 *    [384..437] [396..458] [408..458] [408..457] [396..449] [411..458]
 *    [384..444] [384..458] (file lines; param_1's refs cumulate to 20 at f437).
 *
 * 4. WHY IT STILL COSTS +4 (3800 > the 3796 carve): the fence re-weights EVERY
 *    pseudo inside it, and a short live range gains more priority per ref than
 *    a long one — so the s2-s7 neighbours permute. Under [396..458]:
 *    p125 (14/239 -> 1757) becomes 25/239 -> 4184 and OVERTAKES p124
 *    (16/345 -> 1855 -> 27/345 -> 3130). Best variants leave only two swaps
 *    (p124<->p125, p128<->p630); [384..444] instead moves p122 a1 -> a2.
 *    Either way exactly ONE nop appears, always from a broken load-delay-slot
 *    fill. The whole-function objdump A/B is a single hunk:
 *      base:  lbu v0,21(a0); lbu v1,21(a1); addiu a1,s1,76   <- addiu fills
 *      fence: addiu a1,s0,76; lbu v0,21(a0); lbu v1,21(a2); nop
 *    With p122 in a2 the anti-dependence on a1 vanishes, the scheduler hoists
 *    the addiu, and the slot goes unfilled. TARGET GROUND TRUTH @0x800584EC:
 *      lbu $v0,0x15($a0); lbu $v1,0x15($a1); addiu $a1,$s0,0x4C; addu ...
 *    i.e. the target has the FENCE's register ($s0) AND the BASE's ordering.
 *    Correcting the permutation cascades (p124 would need +5 more refs, which
 *    then breaks p126/p128), so no fence isolates param_1 here: its refs are
 *    structurally interleaved with the s2-s7 midpoint pointers.
 *
 * 5. ROUND 5 — IT MUST BE live_length, AND ONLY param_1's CAN MOVE. With
 *    refs pinned at 74/104, pri80 > pri81 needs LL81/LL80 > 642/456 = 1.408;
 *    ours is 1470/1318 = 1.115. So LL80 <= 1044 (-274) or LL81 >= 1853 (+383).
 *    p81's flow-pass range is ALREADY MAXIMAL (838 of 839 insns — param_2 is
 *    passed to all four recursive calls, the last at insn 1732), so LL81
 *    CANNOT grow: param_1's range must SHRINK. param_1 dies at body line 366
 *    (`iVar6_d = param_1[3]`), param_2 at 393 of 400. The late param_1 reads
 *    (body 298/300/328/331/333/362/364/366, all param_1[0..3] AFTER the
 *    recursive calls) are what hold it live — that is the ladder to attack.
 *
 * ---------------------------------------------------------------------------
 * ROUND 5 — THE SWAP IS CLOSED (619 -> 506), LENGTH STILL EXACT.
 *
 * The fence lever round 4 found is now PAID FOR, and both of round 4's open
 * questions had the same answer: SITE THE FENCE SOMEWHERE ELSE.
 *
 * 1. WHERE TO FENCE — histogram BOTH rivals, not just the one you want to
 *    lift. A `do{}while(0)` weights EVERY pseudo whose refs it encloses, so a
 *    region that mentions param_2 more than param_1 makes the gap WORSE.
 *    Measured net (param_1 - param_2) source mentions per region:
 *        Z block   [317..353]  10 vs 13  = -3     X box [356..386] 10 vs 11 = -1
 *        Y box     [390..420]  10 vs 11  = -1     leaf OT [441..448] 0 vs 12 = -12
 *        recursive [520..656]  16 vs 47  = -31
 *        leaf vtx copies [429..440]  12 vs 0  = +12   <-- ONLY these two
 *        midpoints       [452..517]  26 vs 0  = +26   <-- have param_1 > param_2
 *    DISPROOF OF A ROUND-4 ASSUMPTION: a fence on the Z block moves p81 MORE
 *    than p80 (74->88 but 104->128), taking the ask from +20 to +40 refs.
 *    Round 4 tried 8 regions, ALL inside [384..458] — i.e. all in the midpoint
 *    block, which is exactly where p124..p128 (the midpoint SVECTOR pointers)
 *    live. That is why every one of them permuted s2-s7: the fence was sited
 *    ON TOP of the collateral. The LEAF vertex-copy block is the OTHER BRANCH
 *    of the same if/else, so it cannot touch the midpoint pointers at all.
 *
 * 2. THE FENCE (what is actually in the source now): a DOUBLE `do{}while(0)`
 *    around leaf lines [428..440]. Depth 3 => each enclosed ref counts 3x
 *    instead of 1x, i.e. +2 per ref, so 12 mentions buy +24: p80 74 -> 98
 *    (3368 -> 4461) and p81 104 -> 106 (4244 -> 4326). p80 now outranks p81
 *    and takes $s0 first. A SINGLE fence there is NOT enough (+12 -> 86 refs,
 *    still under p81) — the nesting depth is load-bearing.
 *    RESULT: p80->s0 / p81->s1 (the target's), and p84/p116/p118..p122/p124..
 *    p129/p630 dispositions are ALL UNCHANGED from the pre-fence draft. Zero
 *    collateral — the swap is bought for nothing.
 *
 * 3. WHY ROUND 4's "+1 nop" TAX WAS NOT INHERENT (its stated cause is WRONG).
 *    Round 4 blamed the nop on the register permutation ("with p122 in a2 the
 *    anti-dependence vanishes, the scheduler hoists the addiu"). But the leaf
 *    fence permutes NOTHING and STILL cost +1 (3800) when sited at [429..440].
 *    The real cause: a fence's NOTE_INSN_LOOP_BEG/END are cse/scheduling
 *    BARRIERS. `prim = param_2[5]` (line 428) sat just OUTSIDE the fence while
 *    all 12 of its uses sat inside, so the begin-note cut the definition off
 *    from its uses and param_2[5] was RELOADED (a surplus `lw v1,20(s1)` at
 *    0x80057ed4; the target loads it once at 0x80057ed8). Moving the fence one
 *    line up to [428..440], so the DEFINING statement is inside with its uses,
 *    removed the reload and the length went back to exact.
 *    => GENERAL RULE: put a fence's boundary where no value defined just
 *    outside is used just inside. Cost the boundary, not just the contents.
 *
 * 3b. A SECOND FENCE PAID TOO: one around the FIRST Z min/max update
 *    (`if (zA < zmin) ... else if (zmax < zA) ...`) took 506 -> 494 with the
 *    length still exact, and shortened the aligned residual 77 -> 66 lines.
 *    That is the shape of a MINMAX macro, so it is plausible source, not just
 *    a lever. It lifts p81 104 -> 108 refs (4244 -> 4408) against p80's 4461,
 *    leaving only 53 points of margin — every further param_2-heavy fence eats
 *    into that.
 *
 * 3c. DISPROOFS — do NOT re-walk these (all measured this round):
 *    - Fencing the SECOND Z min/max (zB) as well: 494 -> 574. It BREAKS the
 *      flip. Its read of param_2[7] already follows a join, so it reloads
 *      anyway and the fence buys nothing but p81 refs. A fence pays ONLY where
 *      it creates a barrier the target actually needs; elsewhere it is pure
 *      cost. (This is the cleanest confirmation of the whole ref model.)
 *    - Fencing the X-box min/max (site B): +1 length and the sll/sra DOES NOT
 *      become `lh`. So a loop note does NOT block cse's store-to-load
 *      forwarding. CORRECTION to note 3 above: the `prim` reload the leaf
 *      fence caused was a SCHEDULING effect, not a cse one. Sites B/C/D-adj
 *      are therefore still unspelled, and the fence is not the tool for them.
 *
 * 3d. RTL GROUND TRUTH for the B/C/D-adj forwarding (.cse dump, draft):
 *    at the D-adj site the store is
 *      (insn 63 (set (mem/s:SI (plus (reg/v:SI 81) (const_int 28))) (reg:SI 140)))
 *    and the compare three insns later uses `(reg:SI 140)` DIRECTLY — cse
 *    forwarded and deleted the load. The SAME read after `code_label 95`
 *    (insn 103) is a genuine reload. So the boundary is all that matters.
 *    In cse.c the recording of a store's destination is skipped only when
 *    `sets[i].src_elt == 0` ("if we didn't put a REG_EQUAL value or a source
 *    into the hash table, there is no point is recording DEST"), which fires
 *    for ZERO_EXTRACT/SIGN_EXTRACT (bit-field) destinations and for sources
 *    that set `do_not_record` (volatile). Neither is reachable here without
 *    changing the emitted store, so the spelling remains open — the next lever
 *    to try is something that makes the STORE's source unhashable while still
 *    emitting a plain `sh`/`sw` through param_2's own register.
 *
 * 4. flow.c ground truth for the two levers (gcc 2.8.1, pinned tarball):
 *    reg_live_length is incremented once per insn where the reg is live
 *    (propagate_block's final pass) plus once for the insn that sets it — it
 *    is an accumulation over insns-where-live, NOT a first-use..last-use span.
 *    reg_n_refs is `+= loop_depth` at each ref, which is the whole fence lever.
 *
 * Build the draft with `NON_MATCHING=FUN_80057b80 ./Build`.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80057b80", FUN_80057b80);
#else
void FUN_80057b80(int *param_1, int *param_2, int param_3)
{
    short sVar1;
    u16 uVar2;
    u16 uVar2_a;
    u16 uVar2_b;
    u16 uVar2_c;
    u16 uVar2_d;
    int iVar3;
    int pMin;
    int zA;
    int zB;
    int zC;
    int pCx;
    int pCy;
    int prim;
    int pv;
    int pv2;
    int dz1;
    int dz2;
    int dz3;
    int dz4;
    u32 *otp;
    u32 *puVar4_a;
    u32 *puVar4_b;
    u32 *puVar4_c;
    u32 *puVar4_d;
    u32 *puVar5_a;
    u32 *puVar5_b;
    u32 *puVar5_c;
    u32 *puVar5_d;
    int iVar6_a;
    int iVar6_b;
    int iVar6_c;
    int iVar6_d;
    short *psVar7;
    long *r2;
    int iVar8_a;
    int iVar8_b;
    int iVar8_c;
    int iVar8_d;
    short *psVar10;
    long *r2_00;
    SVECTOR *r2_01;
    SVECTOR *r2_02;
    SVECTOR *r1;
    SVECTOR *r0;
    SVECTOR *r1_00;
    int *piVar13;
    int *local_30;

    piVar13 = param_1 + 0x22;
    local_30 = param_1 + 0x22;
    if (*(int *)(*param_1 + 0x10) > *(int *)(param_1[1] + 0x10))
    {
        param_2[6] = *(int *)(*param_1 + 0x10);
        pMin = param_1[1];
    }
    else
    {
        param_2[6] = *(int *)(param_1[1] + 0x10);
        pMin = *param_1;
    }
    param_2[7] = *(int *)(pMin + 0x10);
    zA = *(int *)(param_1[2] + 0x10);
    do {
    if (zA < param_2[7])
    {
        param_2[7] = zA;
    }
    else if (param_2[6] < zA)
    {
        param_2[6] = zA;
    }
    } while (0);
    zB = *(int *)(param_1[3] + 0x10);
    if (zB < param_2[7])
    {
        param_2[7] = zB;
    }
    else if (param_2[6] < zB)
    {
        param_2[6] = zB;
    }
    zC = param_2[6];
    if (zC < 0)
    {
        zC = zC + 3;
    }
    param_2[6] = zC >> 2;
    if (param_2[8] <= zC >> 2)
    {
        if (*(short *)(*param_1 + 0xc) > *(short *)(param_1[1] + 0xc))
        {
            *(u16 *)(param_2 + 0xc) = *(u16 *)(*param_1 + 0xc);
            pCx = param_1[1];
        }
        else
        {
            *(u16 *)(param_2 + 0xc) = *(u16 *)(param_1[1] + 0xc);
            pCx = *param_1;
        }
        *(u16 *)(param_2 + 0xb) = *(u16 *)(pCx + 0xc);
        sVar1 = *(short *)(param_1[2] + 0xc);
        uVar2 = *(u16 *)(param_1[2] + 0xc);
        if (sVar1 < *(short *)(param_2 + 0xb))
        {
            *(u16 *)(param_2 + 0xb) = uVar2;
        }
        else if (*(short *)(param_2 + 0xc) < sVar1)
        {
            *(u16 *)(param_2 + 0xc) = uVar2;
        }
        sVar1 = *(short *)(param_1[3] + 0xc);
        uVar2 = *(u16 *)(param_1[3] + 0xc);
        if (sVar1 < *(short *)(param_2 + 0xb))
        {
            *(u16 *)(param_2 + 0xb) = uVar2;
        }
        else if (*(short *)(param_2 + 0xc) < sVar1)
        {
            *(u16 *)(param_2 + 0xc) = uVar2;
        }
        if ((-(int)*(short *)(param_2 + 0xd) <= (int)*(short *)(param_2 + 0xc)) &&
            ((int)*(short *)(param_2 + 0xb) <= (int)*(short *)(param_2 + 0xd)))
        {
            if (*(short *)(*param_1 + 0xe) > *(short *)(param_1[1] + 0xe))
            {
                *(u16 *)((int)param_2 + 0x32) = *(u16 *)(*param_1 + 0xe);
                pCy = param_1[1];
            }
            else
            {
                *(u16 *)((int)param_2 + 0x32) = *(u16 *)(param_1[1] + 0xe);
                pCy = *param_1;
            }
            *(u16 *)((int)param_2 + 0x2e) = *(u16 *)(pCy + 0xe);
            sVar1 = *(short *)(param_1[2] + 0xe);
            uVar2 = *(u16 *)(param_1[2] + 0xe);
            if (sVar1 < *(short *)((int)param_2 + 0x2e))
            {
                *(u16 *)((int)param_2 + 0x2e) = uVar2;
            }
            else if (*(short *)((int)param_2 + 0x32) < sVar1)
            {
                *(u16 *)((int)param_2 + 0x32) = uVar2;
            }
            sVar1 = *(short *)(param_1[3] + 0xe);
            uVar2 = *(u16 *)(param_1[3] + 0xe);
            if (sVar1 < *(short *)((int)param_2 + 0x2e))
            {
                *(u16 *)((int)param_2 + 0x2e) = uVar2;
            }
            else if (*(short *)((int)param_2 + 0x32) < sVar1)
            {
                *(u16 *)((int)param_2 + 0x32) = uVar2;
            }
            if ((-(int)*(short *)(param_2 + 0xd) <= (int)*(short *)((int)param_2 + 0x32)) &&
                ((int)*(short *)((int)param_2 + 0x2e) <= (int)*(short *)(param_2 + 0xd)))
            {
                if ((*param_2 == param_3) ||
                    ((*(short *)(param_2 + 0xc) - *(short *)(param_2 + 0xb) < 0xff) &&
                     (*(short *)((int)param_2 + 0x32) - *(short *)((int)param_2 + 0x2e) < 0x7f)))
                {
                    do { do {
                    prim = param_2[5];
                    *(u32 *)(prim + 8) = *(u32 *)(*param_1 + 0xc);
                    *(u32 *)(prim + 0x14) = *(u32 *)(param_1[1] + 0xc);
                    *(u32 *)(prim + 0x20) = *(u32 *)(param_1[2] + 0xc);
                    *(u32 *)(prim + 0x2c) = *(u32 *)(param_1[3] + 0xc);
                    *(u32 *)(prim + 0xc) = *(u32 *)(*param_1 + 0x14);
                    *(u32 *)(prim + 0x18) = *(u32 *)(param_1[1] + 0x14);
                    *(u32 *)(prim + 0x24) = *(u32 *)(param_1[2] + 0x14);
                    *(u32 *)(prim + 0x30) = *(u32 *)(param_1[3] + 0x14);
                    *(u32 *)(prim + 4) = *(u32 *)(*param_1 + 8);
                    *(u32 *)(prim + 0x10) = *(u32 *)(param_1[1] + 8);
                    *(u32 *)(prim + 0x1c) = *(u32 *)(param_1[2] + 8);
                    *(u32 *)(prim + 0x28) = *(u32 *)(param_1[3] + 8);
                    } while (0); } while (0);
                    *(u16 *)(prim + 0xe) = *(u16 *)((int)param_2 + 0x5a);
                    *(u16 *)(prim + 0x1a) = *(u16 *)((int)param_2 + 0x66);
                    *(int *)param_2[5] = param_2[0x13];
                    otp = (u32 *)(param_2[4] + (param_2[6] >> param_2[3]) * 4);
                    param_2[0xe] = (int)otp;
                    *(u32 *)param_2[5] = *otp & 0xffffff | 0xc000000;
                    *(u32 *)param_2[0xe] = param_2[5] & 0xffffff;
                    iVar3 = param_2[5] + 0x34;
                }
                else
                {
                    psVar7 = (short *)*param_1;
                    psVar10 = (short *)param_1[1];
                    *(short *)(param_1 + 4) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
                    r0 = (SVECTOR *)(param_1 + 4);
                    *(short *)((int)r0 + 2) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
                    *(short *)((int)r0 + 4) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
                    *(char *)((int)r0 + 8) = (char)((int)((u32)*(u8 *)(psVar7 + 4) + (u32)*(u8 *)(psVar10 + 4)) >> 1);
                    *(char *)((int)r0 + 9) = (char)((int)((u32)*(u8 *)((int)psVar7 + 9) + (u32)*(u8 *)((int)psVar10 + 9)) >> 1);
                    *(char *)((int)r0 + 0xa) = (char)((int)((u32)*(u8 *)(psVar7 + 5) + (u32)*(u8 *)(psVar10 + 5)) >> 1);
                    *(u8 *)((int)r0 + 0xb) = *(u8 *)((int)psVar7 + 0xb);
                    *(char *)((int)r0 + 0x14) = (char)((int)((u32)*(u8 *)(psVar7 + 10) + (u32)*(u8 *)(psVar10 + 10)) >> 1);
                    *(char *)((int)r0 + 0x15) = (char)((int)((u32)*(u8 *)((int)psVar7 + 0x15) + (u32)*(u8 *)((int)psVar10 + 0x15)) >> 1);
                    psVar7 = (short *)*param_1;
                    psVar10 = (short *)param_1[2];
                    *(short *)(param_1 + 10) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
                    r1_00 = (SVECTOR *)(param_1 + 10);
                    *(short *)((int)r1_00 + 2) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
                    *(short *)((int)r1_00 + 4) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
                    *(char *)((int)r1_00 + 8) = (char)((int)((u32)*(u8 *)(psVar7 + 4) + (u32)*(u8 *)(psVar10 + 4)) >> 1);
                    *(char *)((int)r1_00 + 9) = (char)((int)((u32)*(u8 *)((int)psVar7 + 9) + (u32)*(u8 *)((int)psVar10 + 9)) >> 1);
                    *(char *)((int)r1_00 + 0xa) = (char)((int)((u32)*(u8 *)(psVar7 + 5) + (u32)*(u8 *)(psVar10 + 5)) >> 1);
                    *(u8 *)((int)r1_00 + 0xb) = *(u8 *)((int)psVar7 + 0xb);
                    *(char *)((int)r1_00 + 0x14) = (char)((int)((u32)*(u8 *)(psVar7 + 10) + (u32)*(u8 *)(psVar10 + 10)) >> 1);
                    *(char *)((int)r1_00 + 0x15) = (char)((int)((u32)*(u8 *)((int)psVar7 + 0x15) + (u32)*(u8 *)((int)psVar10 + 0x15)) >> 1);
                    psVar7 = (short *)param_1[2];
                    psVar10 = (short *)param_1[3];
                    *(short *)(param_1 + 0x10) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
                    r2_01 = (SVECTOR *)(param_1 + 0x10);
                    *(short *)((int)r2_01 + 2) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
                    *(short *)((int)r2_01 + 4) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
                    *(char *)((int)r2_01 + 8) = (char)((int)((u32)*(u8 *)(psVar7 + 4) + (u32)*(u8 *)(psVar10 + 4)) >> 1);
                    *(char *)((int)r2_01 + 9) = (char)((int)((u32)*(u8 *)((int)psVar7 + 9) + (u32)*(u8 *)((int)psVar10 + 9)) >> 1);
                    *(char *)((int)r2_01 + 0xa) = (char)((int)((u32)*(u8 *)(psVar7 + 5) + (u32)*(u8 *)(psVar10 + 5)) >> 1);
                    *(u8 *)((int)r2_01 + 0xb) = *(u8 *)((int)psVar7 + 0xb);
                    *(char *)((int)r2_01 + 0x14) = (char)((int)((u32)*(u8 *)(psVar7 + 10) + (u32)*(u8 *)(psVar10 + 10)) >> 1);
                    *(char *)((int)r2_01 + 0x15) = (char)((int)((u32)*(u8 *)((int)psVar7 + 0x15) + (u32)*(u8 *)((int)psVar10 + 0x15)) >> 1);
                    gte_ldv3(r0, r1_00, r2_01);
                    gte_rtpt();
                    psVar7 = (short *)param_1[3];
                    psVar10 = (short *)param_1[1];
                    *(short *)(param_1 + 0x16) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
                    r1 = (SVECTOR *)(param_1 + 0x16);
                    *(short *)((int)r1 + 2) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
                    *(short *)((int)r1 + 4) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
                    *(char *)((int)r1 + 8) = (char)((int)((u32)*(u8 *)(psVar7 + 4) + (u32)*(u8 *)(psVar10 + 4)) >> 1);
                    *(char *)((int)r1 + 9) = (char)((int)((u32)*(u8 *)((int)psVar7 + 9) + (u32)*(u8 *)((int)psVar10 + 9)) >> 1);
                    *(char *)((int)r1 + 0xa) = (char)((int)((u32)*(u8 *)(psVar7 + 5) + (u32)*(u8 *)(psVar10 + 5)) >> 1);
                    *(u8 *)((int)r1 + 0xb) = *(u8 *)((int)psVar7 + 0xb);
                    *(char *)((int)r1 + 0x14) = (char)((int)((u32)*(u8 *)(psVar7 + 10) + (u32)*(u8 *)(psVar10 + 10)) >> 1);
                    *(char *)((int)r1 + 0x15) = (char)((int)((u32)*(u8 *)((int)psVar7 + 0x15) + (u32)*(u8 *)((int)psVar10 + 0x15)) >> 1);
                    psVar7 = (short *)*param_1;
                    psVar10 = (short *)param_1[3];
                    *(short *)(param_1 + 0x1c) = (short)(((int)*psVar7 + (int)*psVar10) / 2);
                    r2_02 = (SVECTOR *)(param_1 + 0x1c);
                    *(short *)((int)r2_02 + 2) = (short)(((int)psVar7[1] + (int)psVar10[1]) / 2);
                    *(short *)((int)r2_02 + 4) = (short)(((int)psVar7[2] + (int)psVar10[2]) / 2);
                    *(char *)((int)r2_02 + 8) = (char)((int)((u32)*(u8 *)(psVar7 + 4) + (u32)*(u8 *)(psVar10 + 4)) >> 1);
                    *(char *)((int)r2_02 + 9) = (char)((int)((u32)*(u8 *)((int)psVar7 + 9) + (u32)*(u8 *)((int)psVar10 + 9)) >> 1);
                    *(char *)((int)r2_02 + 0xa) = (char)((int)((u32)*(u8 *)(psVar7 + 5) + (u32)*(u8 *)(psVar10 + 5)) >> 1);
                    *(u8 *)((int)r2_02 + 0xb) = *(u8 *)((int)psVar7 + 0xb);
                    *(char *)((int)r2_02 + 0x14) = (char)((int)((u32)*(u8 *)(psVar7 + 10) + (u32)*(u8 *)(psVar10 + 10)) >> 1);
                    r2_00 = param_1 + 0x13;
                    *(char *)((int)r2_02 + 0x15) = (char)((int)((u32)*(u8 *)((int)psVar7 + 0x15) + (u32)*(u8 *)((int)psVar10 + 0x15)) >> 1);
                    gte_stsxy3(param_1 + 7, param_1 + 0xd, r2_00);
                    r2 = param_1 + 0x14;
                    gte_stsz3(param_1 + 8, param_1 + 0xe, r2);
                    gte_ldv3(r2_01, r1, r2_02);
                    gte_rtpt();
                    pv = *param_1;
                    piVar13[1] = (int)r0;
                    piVar13[2] = (int)r1_00;
                    piVar13[3] = (int)r2_02;
                    *piVar13 = pv;
                    gte_stsxy3(r2_00, param_1 + 0x19, param_1 + 0x1f);
                    gte_stsz3(r2, param_1 + 0x1a, param_1 + 0x20);
                    param_3 = param_3 + 1;
                    FUN_80057b80(local_30, param_2, param_3);
                    iVar6_a = *param_1;
                    puVar4_a = (u32 *)param_2[5];
                    iVar8_a = param_1[1];
                    puVar4_a[2] = *(u32 *)(iVar6_a + 0xc);
                    puVar4_a[5] = *(u32 *)(iVar8_a + 0xc);
                    puVar4_a[8] = *(u32 *)&r0[1].vz;
                    dz1 = *(int *)(iVar6_a + 0x10);
                    if (dz1 < 0)
                    {
                        dz1 = dz1 + 3;
                    }
                    param_2[6] = dz1 >> 2;
                    puVar4_a[3] = (u32)*(u16 *)(iVar6_a + 0x14);
                    puVar4_a[6] = (u32)*(u16 *)(iVar8_a + 0x14);
                    puVar4_a[9] = (u32)(u16)r0[2].vz;
                    puVar4_a[1] = *(u32 *)(iVar6_a + 8);
                    puVar4_a[4] = *(u32 *)(iVar8_a + 8);
                    puVar4_a[7] = *(u32 *)(r0 + 1);
                    *(u16 *)((int)puVar4_a + 0xe) = *(u16 *)((int)param_2 + 0x5a);
                    uVar2_a = *(u16 *)((int)param_2 + 0x66);
                    *(u8 *)((int)puVar4_a + 3) = 9;
                    *(u8 *)((int)puVar4_a + 7) = 0x34;
                    *(u16 *)((int)puVar4_a + 0x1a) = uVar2_a;
                    puVar5_a = (u32 *)(param_2[4] + (param_2[6] >> param_2[3]) * 4);
                    param_2[0xe] = (int)puVar5_a;
                    *puVar4_a = *puVar5_a & 0xffffff | 0x9000000;
                    *(u32 *)param_2[0xe] = (u32)puVar4_a & 0xffffff;
                    param_2[5] = param_2[5] + 0x28;
                    *piVar13 = (int)r0;
                    pv2 = param_1[1];
                    piVar13[2] = (int)r2_02;
                    piVar13[1] = pv2;
                    piVar13[3] = (int)r1;
                    FUN_80057b80(local_30, param_2, param_3);
                    iVar6_b = param_1[2];
                    puVar4_b = (u32 *)param_2[5];
                    iVar8_b = *param_1;
                    puVar4_b[2] = *(u32 *)(iVar6_b + 0xc);
                    puVar4_b[5] = *(u32 *)(iVar8_b + 0xc);
                    puVar4_b[8] = *(u32 *)&r1_00[1].vz;
                    dz2 = *(int *)(iVar6_b + 0x10);
                    if (dz2 < 0)
                    {
                        dz2 = dz2 + 3;
                    }
                    param_2[6] = dz2 >> 2;
                    puVar4_b[3] = (u32)*(u16 *)(iVar6_b + 0x14);
                    puVar4_b[6] = (u32)*(u16 *)(iVar8_b + 0x14);
                    puVar4_b[9] = (u32)(u16)r1_00[2].vz;
                    puVar4_b[1] = *(u32 *)(iVar6_b + 8);
                    puVar4_b[4] = *(u32 *)(iVar8_b + 8);
                    puVar4_b[7] = *(u32 *)(r1_00 + 1);
                    *(u16 *)((int)puVar4_b + 0xe) = *(u16 *)((int)param_2 + 0x5a);
                    uVar2_b = *(u16 *)((int)param_2 + 0x66);
                    *(u8 *)((int)puVar4_b + 3) = 9;
                    *(u8 *)((int)puVar4_b + 7) = 0x34;
                    *(u16 *)((int)puVar4_b + 0x1a) = uVar2_b;
                    puVar5_b = (u32 *)(param_2[4] + (param_2[6] >> param_2[3]) * 4);
                    param_2[0xe] = (int)puVar5_b;
                    *puVar4_b = *puVar5_b & 0xffffff | 0x9000000;
                    *(u32 *)param_2[0xe] = (u32)puVar4_b & 0xffffff;
                    param_2[5] = param_2[5] + 0x28;
                    *piVar13 = (int)r1_00;
                    piVar13[1] = (int)r2_02;
                    piVar13[2] = param_1[2];
                    piVar13[3] = (int)r2_01;
                    FUN_80057b80(local_30, param_2, param_3);
                    iVar6_c = param_1[3];
                    puVar4_c = (u32 *)param_2[5];
                    iVar8_c = param_1[2];
                    puVar4_c[2] = *(u32 *)(iVar6_c + 0xc);
                    puVar4_c[5] = *(u32 *)(iVar8_c + 0xc);
                    puVar4_c[8] = *(u32 *)&r2_01[1].vz;
                    dz3 = *(int *)(iVar6_c + 0x10);
                    if (dz3 < 0)
                    {
                        dz3 = dz3 + 3;
                    }
                    param_2[6] = dz3 >> 2;
                    puVar4_c[3] = (u32)*(u16 *)(iVar6_c + 0x14);
                    puVar4_c[6] = (u32)*(u16 *)(iVar8_c + 0x14);
                    puVar4_c[9] = (u32)(u16)r2_01[2].vz;
                    puVar4_c[1] = *(u32 *)(iVar6_c + 8);
                    puVar4_c[4] = *(u32 *)(iVar8_c + 8);
                    puVar4_c[7] = *(u32 *)(r2_01 + 1);
                    *(u16 *)((int)puVar4_c + 0xe) = *(u16 *)((int)param_2 + 0x5a);
                    uVar2_c = *(u16 *)((int)param_2 + 0x66);
                    *(u8 *)((int)puVar4_c + 3) = 9;
                    *(u8 *)((int)puVar4_c + 7) = 0x34;
                    *(u16 *)((int)puVar4_c + 0x1a) = uVar2_c;
                    puVar5_c = (u32 *)(param_2[4] + (param_2[6] >> param_2[3]) * 4);
                    param_2[0xe] = (int)puVar5_c;
                    *puVar4_c = *puVar5_c & 0xffffff | 0x9000000;
                    *(u32 *)param_2[0xe] = (u32)puVar4_c & 0xffffff;
                    param_2[5] = param_2[5] + 0x28;
                    *piVar13 = (int)r2_02;
                    piVar13[1] = (int)r1;
                    piVar13[2] = (int)r2_01;
                    piVar13[3] = param_1[3];
                    FUN_80057b80(local_30, param_2, param_3);
                    iVar8_d = param_1[1];
                    puVar4_d = (u32 *)param_2[5];
                    iVar6_d = param_1[3];
                    puVar4_d[2] = *(u32 *)(iVar8_d + 0xc);
                    puVar4_d[5] = *(u32 *)(iVar6_d + 0xc);
                    puVar4_d[8] = *(u32 *)&r1[1].vz;
                    dz4 = *(int *)(iVar8_d + 0x10);
                    if (dz4 < 0)
                    {
                        dz4 = dz4 + 3;
                    }
                    param_2[6] = dz4 >> 2;
                    puVar4_d[3] = (u32)*(u16 *)(iVar8_d + 0x14);
                    puVar4_d[6] = (u32)*(u16 *)(iVar6_d + 0x14);
                    puVar4_d[9] = (u32)(u16)r1[2].vz;
                    puVar4_d[1] = *(u32 *)(iVar8_d + 8);
                    puVar4_d[4] = *(u32 *)(iVar6_d + 8);
                    puVar4_d[7] = *(u32 *)(r1 + 1);
                    *(u16 *)((int)puVar4_d + 0xe) = *(u16 *)((int)param_2 + 0x5a);
                    uVar2_d = *(u16 *)((int)param_2 + 0x66);
                    *(u8 *)((int)puVar4_d + 3) = 9;
                    *(u8 *)((int)puVar4_d + 7) = 0x34;
                    *(u16 *)((int)puVar4_d + 0x1a) = uVar2_d;
                    puVar5_d = (u32 *)(param_2[4] + (param_2[6] >> param_2[3]) * 4);
                    param_2[0xe] = (int)puVar5_d;
                    *puVar4_d = *puVar5_d & 0xffffff | 0x9000000;
                    *(u32 *)param_2[0xe] = (u32)puVar4_d & 0xffffff;
                    iVar3 = param_2[5] + 0x28;
                }
                param_2[5] = iVar3;
            }
        }
    }
    return;
}
#endif
