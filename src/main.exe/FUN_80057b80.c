#include "common.h"
#include "main.exe.h"
#include "gte.h"

/*
 * FUN_80057b80 (0x80057b80, 3796 bytes) — recursive quad/triangle subdivision
 * renderer of the DrawTMD handler family. Compiled-style C using the PsyQ
 * inline-GTE macros (gte_ldv3/gte_rtpt/gte_stsxy3/gte_stsz3) at two RTPT sites.
 *
 * STATUS: NON_MATCHING — 8 of 3796 bytes differ. LENGTH IS EXACT.
 * (Round history: 759 -> 722 -> 619 -> 506 -> 494 -> 253 -> 208 -> 42 -> 36 -> 8.)
 *
 * The ENTIRE residual is ONE hunk: the two parameter-copy chains in the
 * prologue are emitted s0-first where the target emits s1-first.
 *      target: sw s1,28(sp); move s1,a1; sw s0,24(sp); move s0,a0
 *      ours:   sw s0,24(sp); move s0,a0; sw s1,28(sp); move s1,a1
 * Everything else — all 941 other instructions, every register, every delay
 * slot — is byte-identical. See "THE LAST 8 BYTES" at the bottom.
 *
 * ---------------------------------------------------------------------------
 * ROUND 6 — THE REUSABLE RULE (494 -> 8). Read this first.
 *
 * *** A STORE THAT MUST NOT BE FORWARDED BELONGS INSIDE BOTH if/else ARMS. ***
 *
 * Rounds 3-5 chased four "sites" (A/B/C/D-adj) and correctly identified the
 * mechanism — cse's store-to-load forwarding eats combine's `lh` fold at the
 * first read after a same-block store — but drew the WRONG conclusion from the
 * target's asm. All four fell to one source change.
 *
 * THE TRAP (this is the transferable part): at B/C/D-adj the FINAL asm shows
 * the store and its reload ADJACENT, IN ONE BLOCK, WITH NO LABEL BETWEEN THEM:
 *      sh v0,0x2C(s1)   /  lw v1,8(s0)  /  lh v0,0x2C(s1)
 * Round 5 read exactly that and concluded "a real CODE_LABEL would clear cse's
 * table, but there is no label here, so a label cannot be the answer". That
 * inference is invalid. **jump2's cross-jumping runs AFTER cse** and merges the
 * arms' common tail, so a label that existed at cse time is GONE by the time you
 * read the .s. The correct source is the store DUPLICATED into both arms:
 *      if (p0->x > p1->x) { ctx->xmax = p0->x; ctx->xmin = p1->x; }
 *      else               { ctx->xmax = p1->x; ctx->xmin = p0->x; }
 * At cse time the store is in each arm and the read is after the join, so
 * forwarding never fires and the read stays a real `lh`/`lw` that combine folds
 * from the movhi+ashift+ashiftrt. THEN jump2 cross-jumps the identical tail
 * (`lhu vX,12(vY); sh vX,44(ctx)`) into one shared copy, producing the exact
 * adjacency that looks impossible. Ghidra's `pMin`/`pCx`/`pCy` pointer temps are
 * an ARTIFACT of that late merge, not source.
 *
 * => GENERAL RULE: never infer a cse-time block structure from the final asm.
 *    cse1 runs before jump2; cross-jumping erases the labels cse saw. If a
 *    store/reload pair looks impossibly co-located, ask whether the arms were
 *    merged. The `do{}while(0)` fence is NOT the tool here (proven, round 5: a
 *    loop note is a SCHEDULING barrier, not an aliasing one, and does not block
 *    cse forwarding) — only a real join label does, and only arms supply one.
 *
 * VERIFY THIS WAY (fast, ~1s/variant, no Shake): extract the shape into a
 * standalone .c and run cc1-281 directly with the flags from tools/rtldump.py's
 * CC_FLAGS. The X box reduced to a 25-line testbed reproduced the sll/sra bug
 * and then the target's exact instruction sequence (including operand order) on
 * the first try. Do this BEFORE editing a 750-line draft.
 *
 * The four sites are LENGTH-NEUTRAL AS A PACKAGE, which is why rounds 3-5 could
 * not land them piecemeal: B and C each save 1 insn (sll/sra -> lh), D-adj gains
 * the missing `lw param_2[7]` reload (+1) => net -1, which exactly pays for
 * finding A's `proto = param_2 + 0x13` leaf base (+1). Net 0, length stays 3796.
 * A alone overflowed the carve (3800) and was unscoreable — that was the whole
 * reason it sat open for three rounds.
 *
 * ROUND 6's OTHER FINDINGS:
 *  - `proto` (finding A) CONFIRMED at the asm level: the leaf reads all three of
 *    its fields through a2 (`lhu 0xE(a2)`, `lhu 0x1A(a2)`, `lw 0x0(a2)`), and
 *    the four recursive blocks read 90/102 straight off param_2. Leaf-only.
 *  - `proto` must be assigned AFTER piVar13/local_30 (36 -> 8). The target
 *    computes `addiu t0,s0,136` right after the parameter moves and
 *    `addiu a2,s1,76` LAST, in the delay slot of the first beqz at 0x80057be0.
 *    With proto first, `move s8,t0` fills that slot instead.
 *  - gte_stsz3's THIRD pointer is INLINE, not hoisted (42 -> 36). The target
 *    computes the arg bases in ARGUMENT order (addiu 32/56/80), so
 *    `r2 = param_1 + 0x14;` as a preceding statement computed it FIRST.
 *    Spelling `gte_stsz3(param_1 + 8, param_1 + 0xe, param_1 + 0x14)` lets cse
 *    merge the two occurrences into one pseudo — one `addiu a0,s0,80` computed
 *    third and reused at the second call site, exactly as the target has it.
 *    NOTE the asymmetry: r2_00 (param_1 + 0x13) IS a real preceding statement
 *    (the target computes it early at 0x800584EC). Keep r2_00, inline r2.
 *  - The Z min/max fence round 5 added is now OBSOLETE (253 -> 208): it was
 *    buying a barrier the pre-restructure shape needed, at +4 weighted refs on
 *    param_2. Removing it also makes the two Z blocks symmetric.
 *  - Round 3's finding 1 ("local_30 must NOT be copied from piVar13") is now
 *    MOOT: post-restructure, `local_30 = piVar13;` and `piVar13 = local_30;`
 *    and two independent `param_1 + 0x22` all give byte-identical output (8).
 *    The conflict graph it was measured on no longer exists.
 *
 * ---------------------------------------------------------------------------
 * TWO MEASUREMENT HAZARDS THAT COST THIS ROUND REAL TIME — read before you
 * count anything in a .s.
 *
 * 1. maspsx's DEBUG comment lines QUOTE instructions. `.shake/processed/.../
 *    X.s` contains lines like
 *        nop # DEBUG: Reuse of '$2'. 'sh	$2,44($16)' does not use $at
 *    A naive `grep -c 'sh.*44($16)'` counts the comment as a real store. That
 *    manufactured a phantom "+3 surplus param_2 refs" and a whole false theory
 *    that the arms were NOT being cross-jumped. Strip every line's first '#'
 *    to end-of-line with sed before counting — and NOT with `grep -v '^#'`,
 *    because these lines start with `nop`, not with '#'.
 * 2. The GTE macros emit `swc2 $17, 0($4)` etc. `$17` there is COP2 data
 *    register 17, NOT $s1. Any `$17` count in a gte.h function is inflated by
 *    the swc2/lwc2 lines. Discount them.
 *   With both applied, our param_1/param_2 asm mention counts are 76/107 —
 *   EXACTLY the target's. That is the proof the source structure is right and
 *   only the hard-register assignment ever differed.
 *
 * ---------------------------------------------------------------------------
 * THE REGISTER ALLOCATION (still true, still load-bearing).
 *
 * cc1 2.8.1 global.c allocno_compare:
 *     pri = (floor_log2(n_refs) * n_refs / live_length) * 10000 * size
 * sorted priority-descending, ties to the LOWER allocno. MIPS defines no
 * REG_ALLOC_ORDER, so find_reg takes the first free callee-saved reg = $s0.
 * prune_preferences() strips a0/a1 copy preferences from anything crossing a
 * call, so NOTHING biases this but the sort. flow.c: reg_n_refs is `+= loop_depth`
 * at each ref (the whole fence lever); reg_live_length accumulates over
 * insns-where-live, NOT a first-use..last-use span. Use tools/regalloc.py
 * --compare 80 81; it prints the exact "needs +N weighted ref(s)".
 *
 * THE FENCE (scaffolding, load-bearing, honest about it): a TRIPLE `do{}while(0)`
 * (depth 4) around the LEAF vertex copies. param_1 must outrank param_2 for $s0.
 * Post-restructure the gap was only +1 weighted ref (p80 4421 vs p81 4453); the
 * triple fence takes p80 74 -> 110 refs and wins with margin. Zero collateral —
 * s2..s8 dispositions and every other pseudo are unchanged.
 *   - WHY THE LEAF: histogram BOTH rivals. Net (param_1 - param_2) source
 *     mentions per region: Z block -3, X box -1, Y box -1, leaf OT -12,
 *     recursive -31, midpoints +26, LEAF VERTEX COPIES +12. Only the last two
 *     have param_1 > param_2, and the midpoints are where p124..p128 live (every
 *     fence round 4 tried there permuted s2-s7 — it sited the fence ON TOP of
 *     the collateral). The leaf is the OTHER ARM of the same if/else, so it
 *     cannot touch them.
 *   - THE BOUNDARY IS LOAD-BEARING: `prim = param_2[5]` must stay INSIDE with
 *     its 12 uses. Outside, the loop-begin note cuts the definition from the
 *     uses and param_2[5] is reloaded (+1). General rule: put a fence's boundary
 *     where no value defined just outside is used just inside.
 *
 * Ghidra's mega-pseudos are the other half of the allocation (worth 140 bytes):
 * it reuses ONE variable for many unrelated jobs; each is ONE pseudo and gets ONE
 * hard register for ALL fragments, so a conflict anywhere exiles every use.
 * iVar3 did ~12 jobs and landed in $a3 with 70 mentions where the target's $a3
 * has 5. Split per-site/per-block. Find it by histogramming register mentions:
 * a caller-saved register with 70 mentions is a mega-pseudo; no real source has
 * one. Also load-bearing: each midpoint block writes its fields THROUGH its own
 * SVECTOR pointer (r0/r1_00/r2_01/r1/r2_02); and `sVar1` is a SIGNED short while
 * `uVar2` is u16 — DIFFERENT modes, so cc1 emits both `lh` and `lhu` and does not
 * CSE them (declaring sVar1 as u16 collapses them and costs 2 insns).
 *
 * ---------------------------------------------------------------------------
 * THE LAST 8 BYTES — CLOSED AT THE C LEVEL. The lever is the SIGNATURE, and it
 * is NOT reachable from the body. Round 7 proved this from the gcc sources and
 * from the two matched functions that already have the target's shape.
 * DO NOT re-walk any of the below; DO NOT try another statement order.
 *
 * WHAT THE RESIDUAL IS: exactly one binary choice in .sched2, at T-20.
 *   ;; ready list at T-20: 6 (1) 4 (1), now 6 4          <- picks 6. We need 4.
 *   ;; ready list at T-21: 4 (1) 2016 (1), now 4 2016
 *   ;; insn 2016 has a greater potential hazard, now 2016 4
 * sched.c schedules each block BOTTOM-UP (schedule_insn decrements the
 * LOG_LINKS predecessors' ref counts), so the emitted order is the REVERSE of
 * the pick order: picks 6,2016,4,2018,1998 -> emits 1998, sw s0, move s0,
 * sw s1, move s1 = our exact output. Picking 4 at T-20 gives the target and
 * everything else follows automatically. So the whole hunk is that one tie.
 *
 * ROUND 6's IDEA IS REFUTED BY THE SOURCE (do not retry it). gcc 2.8.1's
 * priority() maxes over LOG_LINKS — the insns an insn DEPENDS ON — so priority
 * is DEPTH FROM THE BLOCK TOP, not the critical path to the block end (that is
 * later-gcc/haifa behaviour, and the trap here). A first-block USE of param_2 is
 * a SUCCESSOR of insn 6 and contributes NOTHING to priority(6); a load-use
 * hazard there cannot lift it. Each copy's ONLY predecessor is its own stack
 * save via REG_DEP_ANTI, and MIPS's ADJUST_COST zeroes anti-deps (clamped to 1),
 * so BOTH priorities are pinned at 1 by structure. Also note the goal was
 * inverted: we need PRIORITY(4) > PRIORITY(6), since the first pick is emitted
 * last. Every tie-break in the ranker is likewise blind to these two insns:
 *   - rank_for_schedule's dependent-class test: `insn_cost(tmp,link,last) == 1`
 *     -> class 3. A reg move has result_ready_cost 1, so both are ALWAYS class 3.
 *   - schedule_select's actual_hazard / potential_hazard: mips.md defines
 *     function units ONLY for load/store/xfer/imul/idiv. A reg-reg move has NO
 *     unit, so both hazards are 0 and best_insn stays 0 (a tie keeps index 0).
 *     (This IS what drags each `sw` to sit just before its copy: a store HAS the
 *     memory unit, so it wins the hazard test against a move. Hence the save
 *     order in the RTL is IRRELEVANT — simulate it: it reproduces our output.)
 * => INSN_LUID is the ONLY discriminator, i.e. the order assign_parms emits the
 *    arg-register copies. The (save,def) pairs then emit in ascending LUID of
 *    the DEF. The target needs LUID(a1-copy) < LUID(a0-copy).
 *
 * HOW THE TARGET GETS THAT (function.c:4142). assign_parms emits each copy
 * INLINE in declaration order UNLESS
 *     nominal_mode != passed_mode || promoted_nominal_mode != promoted_mode
 * in which case it emits `move tempreg,aN` inline but DEFERS the real assignment
 * into `conversion_insns`, flushed only after ALL parms (function.c:4439).
 * tempreg then coalesces into aN, so the deferred insn becomes a PLAIN
 * `move sX,aN` whose LUID is greater than every other parm copy. That condition
 * fires only for a parm NARROWER THAN A WORD. For a pointer/int, all four modes
 * are SImode (line 3789 forces passed_mode = nominal_mode = Pmode when
 * passed_pointer), so it can NEVER fire.
 *
 * THE EVIDENCE (this is why the above is not a theory). Exactly THREE functions
 * in main.exe emit the a1-copy before the a0-copy — this one, and:
 *   create_ninken_character_  MATCHED:  void create_ninken_character_(s16 type, s32 stage)
 *   cd_control                MATCHED:  void cd_control(u8 param_1, u8 *param_2, u8 *param_3)
 * BOTH have a NARROW FIRST PARAMETER. Reproduced standalone in 7 lines: a
 * (int*,int*,int) signature gives our order; changing p1 to `unsigned char`
 * reproduces cd_control's asm exactly — a0-copy last, PLAIN move, with the
 * `andi` truncation pushed out to the USE site (so s0 holds the raw a0, which is
 * why the deferred copy is a bare `move` and not a sll/sra).
 *
 * WHY IT IS STILL 8: param_1 here is used as a POINTER BASE — `addiu t0,s0,0x88`
 * at 0x80057b94, copied to fp and dereferenced — so it cannot be a narrow type,
 * and the deferral cannot fire. A local copy cannot substitute either: s0 must
 * hold a0's value across calls, so the a0->s0 copy IS the parm copy (any
 * `int *ctx = param_1;` is folded by cse / coalesced by the allocator).
 * => The 8 bytes are NOT reachable by ANY statement order, fence, or respelling
 *    of the body. Only the first parameter's DECLARED TYPE can move them.
 * NEXT ROUND, START HERE: recover the original prototype (docs/psx-sym.md — the
 * demo disc's debug symbols) and look at the FIRST parameter's type. If it is
 * narrow, the body's role assignment for s0 needs re-reading; if it is a
 * pointer, this residual is a genuine compiler-input difference and the function
 * should be promoted as-is at 8.
 *
 * TRIED AND MEASURED, ALL STILL 8 (do not repeat):
 *   piVar13/local_30/proto in all 6 statement orders (proto first: 36;
 *   piVar13,proto,local_30: LENGTH MISMATCH 3788; local_30,proto,piVar13: 32);
 *   `local_30 = piVar13;` and `piVar13 = local_30;` copy forms: both 8.
 *
 * DO NOT RE-RUN / RE-DERIVE (all spent, all negative):
 *   - autorules (46 candidates, no win); the permuter (refuses gte.h functions
 *     outright — its parser cannot read inline asm).
 *   - the ctx alias (`int *ctx = param_2;` then `ctx[7] = ...`): it DOES produce
 *     the reload, but the store then uses ctx's register (`sw v0,28(a1)`) where
 *     the target uses param_2's. Superseded entirely by the arms rule above.
 *   - "equalise the priorities, ties break to the lower allocno": arithmetically
 *     empty here, no N lands on the tie.
 *   - "regalloc.py reads the wrong dump": it does not. Its .lreg live_length
 *     scores 0 violations / 53 pairs against the real post-qsort order printed by
 *     `;; N regs to allocate:`; the .flow numbers score 15. live_length is simply
 *     not bounded by the instruction count.
 *   - a fence on the Z block / the second Z min/max / site B: all measured, all
 *     negative (the second Z min/max BREAKS the flip: 494 -> 574).
 *
 * The `lwc2; nop; nop; RTPT` shape at both command sites (0x80058320, 0x80058548)
 * is confirmed from the raw image bytes, so the nop-carrying `gte_rtpt()` is
 * correct here (this is a COMPILED caller — docs/gte-policy.md).
 *
 * Build the draft with `NON_MATCHING=FUN_80057b80 ./Build`. Rebuild it BEFORE
 * every `matchdiff.py -n` — a plain `./Build check` relinks the INCLUDE_ASM stub
 * and matchdiff will (correctly) refuse to score it.
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
    int zA;
    int zB;
    int zC;
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
    int *proto;

    piVar13 = param_1 + 0x22;
    local_30 = param_1 + 0x22;
    proto = param_2 + 0x13;
    if (*(int *)(*param_1 + 0x10) > *(int *)(param_1[1] + 0x10))
    {
        param_2[6] = *(int *)(*param_1 + 0x10);
        param_2[7] = *(int *)(param_1[1] + 0x10);
    }
    else
    {
        param_2[6] = *(int *)(param_1[1] + 0x10);
        param_2[7] = *(int *)(*param_1 + 0x10);
    }
    zA = *(int *)(param_1[2] + 0x10);
    if (zA < param_2[7])
    {
        param_2[7] = zA;
    }
    else if (param_2[6] < zA)
    {
        param_2[6] = zA;
    }
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
            *(u16 *)(param_2 + 0xb) = *(u16 *)(param_1[1] + 0xc);
        }
        else
        {
            *(u16 *)(param_2 + 0xc) = *(u16 *)(param_1[1] + 0xc);
            *(u16 *)(param_2 + 0xb) = *(u16 *)(*param_1 + 0xc);
        }
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
                *(u16 *)((int)param_2 + 0x2e) = *(u16 *)(param_1[1] + 0xe);
            }
            else
            {
                *(u16 *)((int)param_2 + 0x32) = *(u16 *)(param_1[1] + 0xe);
                *(u16 *)((int)param_2 + 0x2e) = *(u16 *)(*param_1 + 0xe);
            }
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
                    do { do { do {
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
                    } while (0); } while (0); } while (0);
                    *(u16 *)(prim + 0xe) = *(u16 *)((int)proto + 0xe);
                    *(u16 *)(prim + 0x1a) = *(u16 *)((int)proto + 0x1a);
                    *(int *)param_2[5] = *proto;
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
                    gte_stsz3(param_1 + 8, param_1 + 0xe, param_1 + 0x14);
                    gte_ldv3(r2_01, r1, r2_02);
                    gte_rtpt();
                    pv = *param_1;
                    piVar13[1] = (int)r0;
                    piVar13[2] = (int)r1_00;
                    piVar13[3] = (int)r2_02;
                    *piVar13 = pv;
                    gte_stsxy3(r2_00, param_1 + 0x19, param_1 + 0x1f);
                    gte_stsz3(param_1 + 0x14, param_1 + 0x1a, param_1 + 0x20);
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
