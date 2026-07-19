#include "common.h"
#include "main.exe.h"
#include "gte.h"

/*
 * FUN_80058c70 (0x80058c70, 0x398 bytes) — a DrawTMD-family fast primitive
 * renderer (TMD primitive code 0x3d), the sibling of the switch arms in
 * FUN_80058a54: it transforms each quad's four vertices through the GTE
 * (RTPT + a backface NCLIP + a per-quad RTPS) and assembles a POLY_GT4
 * (code 0x3c, len 0xc) packet, then hands the working state to FUN_80057b80.
 * Compiled-style GTE function under the restricted gte.h policy
 * (docs/gte-policy.md).
 *
 * STATUS: MATCHED in pure C — 0 of 920 bytes differ.
 *
 * 2026-07-19 correction: this also matches under the common compiler flags.
 * The former -fno-strength-reduce exception was compensating for a lost data
 * model: spelling the input as its original TMD_P_TNG4 record gives loop.c one
 * coherent field cursor. The u_short-offset draft manufactured two induction
 * pointers, so the old conclusion that strength reduction was disabled in the
 * original object was wrong.
 *
 * The last residual was not solved by another scheduler fence. It came from
 * two decompiler-style carrier locals that happened to improve one region but
 * trapped the function in a local minimum. The coherent source shape is:
 *
 *  - read `param_6` directly for the packet word;
 *  - keep the unrelated colour word from `param_5` in its own `colorWord`
 *    local instead of reusing `uVar7`, whose later life is a GTE address;
 *  - initialize `pkt[8]`, then `pkt[3]`, before the byte/code fields.
 *
 * Compiler dumps confirm the mechanism. The distinct single-set colour local
 * changes sched1's birthing priorities and places the volatile stack reads in
 * the target order. In sched2 that also leaves the natural `param_4` entry copy
 * after the s0/HWD0/li-4 leaders, matching the target prologue. The natural
 * VWD0-early `/ 2` expressions remain intact; no manual strength expansion,
 * dummy control flow, register pinning, or artificial `cnt` local is needed.
 *
 * === HISTORICAL INVESTIGATION (all residual/open claims below superseded) ===
 * the old residual (s4-group [sw s4,48; move s4,a3] emitted before the s0-group
 * [sw s0,32; lw s0,96; lui/lw HWD0; li v0,4]) is now byte-exact via TWO source
 * changes, and the residual moved to a NEW, SMALLER, single-decision tie:
 *
 * 1. `cnt = param_4;` as the LAST statement before the guard (loop counts cnt).
 *    The old header's "cc1 COALESCES it -> no-op" was a MISDIAGNOSIS: the copy
 *    SURVIVES (combine folds insn10's p83=a3 into it, making it read HARD a3 and
 *    deleting the entry copy — .greg proof), but sched1 was hoisting it to the
 *    TOP of block 1, keeping its sched2 LUID below the body leaders. Mechanism
 *    (sched.c): the backward list scheduler's adjust_priority() boosts insns
 *    whose dest is SINGLE-SET ("birthing", REG_N_SETS==1) to LAUNCH_PRIORITY
 *    0x7f000001 when they become ready; 12/22/25 are all birthing, the cnt-copy
 *    (2 sets: init+decrement) never is, so it loses every contested tick and the
 *    LAST backward pick = TOPMOST placement. rank_for_schedule tie-break within
 *    equal priority is INSN_LUID DESCENDING (confirmed in source).
 * 2. Statement order `sh#1(iVar1/2); iVar2 = VWD0; sh#2` — the VWD0 read moved
 *    AFTER the HWD0 sh (was before local_38). This opens a one-tick BUBBLE at
 *    sched1 T-20 (the VWD0 chain's launches drain first; .i.sched shows ready
 *    list = {copy} ALONE) where the cnt-copy is picked mid-block -> chain pos 10
 *    -> sched2 LUID above all three leaders -> the leader cluster emits
 *    [12][22][25][sw s4; move s4,a3] = TARGET. The bubble is the whole lever:
 *    with VWD0 read early (any earlier position — swept), the copy tops out and
 *    the old 26-rotation returns.
 *
 * NEW RESIDUAL (25 bytes = ONE local-alloc color bit): the HWD0/VWD0 divide
 * region has v0<->v1 swapped: target iVar1(HWD0)=v1, li-4 temp=v0, HWD0-div
 * addu/sra accumulate in v1 (addu v1,v1,v0 — local-alloc ties the addu dest to
 * the DYING dividend'S qty; the tie fires in ours too, whole qty just colored
 * v0), VWD0=v0 both. Ours: iVar1-qty=v0, '4'=v1. The sh#1-vs-lui slide at
 * 0x80058cc8 is a forced anti-dep consequence of the same bit (our VWD0 lw v0
 * must wait for sh-v0; target's sh reads v1 so its lw hoists). ONE decision.
 *
 * REFUTED this round (each measured; ~20 builds):
 *  - de-birthing the leaders to un-boost them (2nd sets via {4,0x96} or
 *    {HWD0,VWD0} variable fusion, dead exit-block sets, carrying the return
 *    through the '4' var or pkt): every variant either (a) makes the fused var
 *    die TWICE in block 1 -> local-alloc drops its qty (combine_regs requires
 *    qty; comment "not local to this block or dies more than once") -> the
 *    div-chain addu loses the dividend tie -> addu v0,... (wrong, 21..64 bytes),
 *    or (b) makes the var GLOBAL -> global.c colors it v0-first (no
 *    REG_ALLOC_ORDER on MIPS) -> same color loss; dead sets are flow-deleted
 *    BEFORE sched1 recounts REG_N_SETS, so they never de-birth anything.
 *  - guard-variable/counter splits (single-set cnt for a birthing boost, n=cnt
 *    in preheader): allocation wall — cnt never reaches s4: its a3 copy-pref
 *    wins while a3 is free (V1 build: beq a3 + preheader move s4,a3), and with
 *    a3 conflicted it falls to v1/t1 (find_reg numeric order), never s4.
 *  - statement-permutation sweep under the working core (10 variants: local_38
 *    positions, const-store hoists, cnt positions, iVar1/iVar2 decl swap):
 *    all 25 (color bit inert) or worse (64-81: moving iVar1=HWD0 after *pkt=4
 *    flips the leader LUID order; hoisting VWD0 reverts the rotation).
 *  - narrow param_4, non-volatile param_5/6, HWD0[] array form: as before
 *    (see git history for the prior header's full text).
 *
 * The color bit DECODED (QTY_CMP_PRI = floor_log2(refs)*refs*1e4/(death-birth),
 * qsort desc, then qty# asc): the contested qty is the TIE-CHAIN TRIPLE
 * {iVar1(85), addu-dest, sra-dest} — block_alloc ties each op's dest to its
 * dying first operand — refs 3+2+2=7, log2=2. Its rival, the li-4 temp qty
 * (refs 2, span [li..sw]=2 suids), scores 10000. The triple's span runs from
 * the HWD0 lw to the sh#1 STORE's sched1 slot: in the color-good (pre-bubble,
 * VWD0-early) chain the VWD0 lw interleaves INTO the div chain, stretching the
 * span to 16 suids -> 8750 < 10000 -> '4' allocates first, takes v0, and the
 * overlapped triple falls to v1 = TARGET. In the bubble chain the div chain is
 * compact (12 suids -> 11667 > 10000) -> triple takes v0 first = our 25.
 * Span-stretch attempts (split `h = iVar1/2; iVar2 = VWD0; sh#1 = (short)h`
 * with fresh h / iVar1-self-update / iVar3-reuse) all re-broke the WINDOW
 * (26/26/43) — the stretch and the bubble compete for the same sched1 slots.
 *
 * PROVEN IRRECONCILABLE at single-statement reach (round 3 close-out; a
 * 12-variant decoupling sweep under the color-good core all 26+ or LENGTH
 * MISMATCH, plus the .i.sched gate analysis):
 *   - The copy can only sink via a tick where it is ALONE-ready. Ticks are
 *     filled by pri>=2 stores unless GATED: a store is unready until every
 *     program-LATER may-alias load is placed (anti-dep, backward sched). The
 *     one protectable tick (T-20) is created by the VWD0-lw's latency-2
 *     launch, and it is empty ONLY when sh#1 and the *pkt=4 store are gated
 *     BEHIND that same VWD0-lw, i.e. `iVar2 = VWD0` AFTER sh#1 in source.
 *   - The color bit needs sh#1's sched1 slot at pos>=10 (div-qty span >= 16
 *     -> 8750 < the '4' qty's 10000). sh#1 reaches pos 10 only UNGATED
 *     (VWD0-lw program-before it) — and then sh#1 itself fills T-20 (V8
 *     dump: sh#1 picked exactly T-20; the copy exiled to T-29 = the old
 *     rotation). One tick, two claimants; every legal gate assignment hands
 *     it to exactly one.
 *   - The cascade anchor cannot shift: uVar3's lw is capped program-BELOW
 *     every pkt-store by may-alias ordering (target bytes: lw 4(v0) before
 *     li 150), so the uVar launch timing that positions the mid-region is
 *     rigid, and the tick budget is fixed by the insn count.
 *   - Fallbacks closed: ref-lowering the div qty must keep the addu-dividend
 *     tie (target addu v1,v1,v0; tie loss measured = 21-byte build) — the
 *     best legal reduction {iVar1,addu} scores EXACTLY 10000 = the '4' qty,
 *     and qty_compare_1's tie-break (qty number asc = birth order) still
 *     picks iVar1 first -> v0. Raising the '4' qty needs a third ref in a
 *     2-suid span: no existing reader, any new one emits (+4), arithmetic
 *     double-reads tree-fold, and dead exit reads are flow-deleted before
 *     the REG_N_REFS recount (measured).
 * A fix, if one exists, needs a construct OUTSIDE single-statement reorder
 * (e.g. cc1 instrumentation to enumerate find_free_reg outcomes, or a
 * different decomposition of the /2 chains that re-times the whole block).
 * ============================================================================
 *
 * WHAT UNLOCKED THE 620 PARK (each independently measured; the park's
 * "failed" singles were pair-negatives, cookbook §4):
 *
 * 1. SUPERSEDED: -fno-strength-reduce appeared necessary only while the input
 *    record was flattened into a u_short pointer. The old argument was:
 *    r0 lives in $a3 with caller-save spill/reload around the jal
 *    (sw a3,24(sp) in the delay slot / lw a3,24(sp) after), and
 *    CALLER_SAVE_PROFITABLE(refs,calls) = 4*calls < refs needs r0's refs
 *    loop-depth-weighted (flow.c REG_N_REFS += loop_depth) => real LOOP
 *    NOTES (do-while, not the park's goto hack); with notes, loop.c would
 *    reduce the 21-giv cursor class (combine_givs sums benefits) into a
 *    second induction register the target provably lacks — no C spelling
 *    denies a 21-member DEST_ADDR class, so SR was off. This one flag +
 *    do-while replaces the park's items 1-2 (the goto loop was compensating
 *    for the wrong root cause and killed the ref weighting).
 * 2. volatile u_long param_5/param_6 (CdaPlayXA parameter-qualifier
 *    precedent), read once each into uVar5/uVar7 at their statement
 *    positions. Plain stack parms' entry copies are pinned at the top and
 *    local-alloc colours them a1/a0 — p85->a0 is what evicted param_1 into
 *    the park's `move t0,a0`. Volatile keeps the reads at their use sites
 *    (lw v0,92/lw v1,88 late), a0 stays free, param_1's pseudo takes its
 *    copy-preference for $a0 and the entry move vanishes.
 * 3. pkt = param_7 local copy carrying ALL body uses. update_equiv_regs
 *    DOUBLES a REG_EQUIV-noted single-set parm's live length
 *    (local-alloc.c:1153), halving param_7's global priority (17880) below
 *    puVar4's (21120) -> wrong s0/s1. The unnoted local scores ~2x and wins
 *    s0 first; the copy coalesces away via the hard-reg copy preference.
 * 4. puVar5/local_38: BOTH defined above the guard (puVar5 = pkt+0x38;
 *    local_38 = puVar5), loop stores via local_38 (the copy -> s3), call arg
 *    = puVar5 (the original -> spilled 16(sp), reload-inherited move s3,t0 +
 *    sw t0,16(sp) in the beqz delay slot, lw a0,16(sp) at the call).
 *    This is the park's two "failed" singles applied JOINTLY.
 * 5. iVar4 = 0x3c named once (li v0,60), stored to +0x53 through it, and
 *    iVar3 = iVar4 inside the guard -> the target's `move s5,v0` (a real
 *    register copy, not a rematerialised li).
 * 6. The gte address-argument scratch reuse: uVar3/uVar8/uVar7 (and their
 *    disjoint second lives) host the stsxy3/stsz4 address temps as OWN
 *    STATEMENTS (in-macro-arg assignments get renamed by combine's
 *    new-pseudo path — measured, +8 bytes): uVar3 = pkt+0x24 (a1, also the
 *    packet-word life), uVar8 = pkt+0x23 / pkt+0x2a (a0 pair), uVar7 =
 *    pkt+0x30 (v1, also the param_5 read). Multi-set kills both
 *    move_movables hoisting (the +144/+168 temps hoisted+spilled otherwise;
 *    loop.c runs before combine so the kill survives the rename) and the
 *    volatile read's birthing bump.
 *
 * Final verification: the unguarded C object has 224 target/body instructions
 * and byte-matches the 0x398-byte target extent exactly.
 */

extern void FUN_80057b80(u_long *outv, u_long *packet, int mode);

u_long *FUN_80058c70(u_short *param_1, u_long param_2, u_long *param_3, int param_4,
                     volatile u_long param_5, volatile u_long param_6, u_long *param_7)
{
    int iVar1;
    int iVar2;
    u_long uVar3;
    int iVar3;
    int iVar4;
    u_long *pkt;
    u_long uVar7;
    u_long uVar8;
    SVECTOR *r0;
    TMD_P_TNG4 *primitive;
    u_long *puVar5;
    u_char uVar6;
    SVECTOR *r0_00;
    SVECTOR *r2;
    SVECTOR *r1;
    u_long *local_38;
    u_long colorWord;

    pkt = param_7;
    iVar1 = HWD0;
    *pkt = 4;
    puVar5 = pkt + 0x38;
    local_38 = puVar5;
    iVar2 = VWD0;
    *(short *)(pkt + 0xd) = (short)(iVar1 / 2);
    *(short *)((int)pkt + 0x36) = (short)(iVar2 / 2);
    uVar3 = *(u_long *)(param_6 + 4);
    colorWord = param_5;
    pkt[8] = 0x96;
    pkt[3] = colorWord;
    *(u_char *)((int)pkt + 0x4f) = 0xc;
    iVar4 = 0x3c;
    pkt[5] = (u_long)param_3;
    *(u_char *)((int)pkt + 0x53) = iVar4;
    pkt[4] = uVar3;
    if (param_4 != 0) {
        r0 = (SVECTOR *)(pkt + 0x20);
        r1 = (SVECTOR *)(pkt + 0x26);
        r2 = (SVECTOR *)(pkt + 0x2c);
        r0_00 = (SVECTOR *)(pkt + 0x32);
        iVar3 = iVar4;
        primitive = (TMD_P_TNG4 *)param_1;
        do {
            *(short *)(pkt + 0x20) = *(u_short *)(primitive->v0 * 8 + param_2);
            *(short *)((int)pkt + 0x82) = *(u_short *)(primitive->v0 * 8 + param_2 + 2);
            *(short *)(pkt + 0x21) = *(u_short *)(primitive->v0 * 8 + param_2 + 4);
            *(short *)(pkt + 0x26) = *(u_short *)(primitive->v1 * 8 + param_2);
            *(short *)((int)pkt + 0x9a) = *(u_short *)(primitive->v1 * 8 + param_2 + 2);
            *(short *)(pkt + 0x27) = *(u_short *)(primitive->v1 * 8 + param_2 + 4);
            *(short *)(pkt + 0x2c) = *(u_short *)(primitive->v2 * 8 + param_2);
            *(short *)((int)pkt + 0xb2) = *(u_short *)(primitive->v2 * 8 + param_2 + 2);
            *(short *)(pkt + 0x2d) = *(u_short *)(primitive->v2 * 8 + param_2 + 4);
            *(short *)(pkt + 0x32) = *(u_short *)(primitive->v3 * 8 + param_2);
            *(short *)((int)pkt + 0xca) = *(u_short *)(primitive->v3 * 8 + param_2 + 2);
            *(short *)(pkt + 0x33) = *(u_short *)(primitive->v3 * 8 + param_2 + 4);
            *local_38 = (u_long)r0;
            local_38[1] = (u_long)r1;
            local_38[2] = (u_long)r2;
            local_38[3] = (u_long)r0_00;
            gte_ldv3(r0, r1, r2);
            gte_rtpt();
            *(short *)(pkt + 0x25) = *(u16 *)&primitive->tu0;
            *(short *)(pkt + 0x2b) = *(u16 *)&primitive->tu1;
            uVar8 = (u_long)(pkt + 0x23);
            gte_stsxy3((u_long *)uVar8, pkt + 0x29, pkt + 0x2f);
            gte_nclip();
            *(short *)(pkt + 0x31) = *(u16 *)&primitive->tu2;
            *(short *)(pkt + 0x37) = *(u16 *)&primitive->tu3;
            gte_stopz(pkt + 6);
            if (0 < (int)pkt[6]) {
                gte_ldv0(r0_00);
                gte_rtps();
                pkt[0x22] = *(u_long *)&primitive->r0;
                uVar6 = (u_char)iVar3;
                *(u_char *)((int)pkt + 0x8b) = uVar6;
                pkt[0x28] = *(u_long *)&primitive->r1;
                *(u_char *)((int)pkt + 0xa3) = uVar6;
                pkt[0x2e] = *(u_long *)&primitive->r2;
                *(u_char *)((int)pkt + 0xbb) = uVar6;
                pkt[0x34] = *(u_long *)&primitive->r3;
                *(u_char *)((int)pkt + 0xd3) = uVar6;
                gte_stsxy(pkt + 0x35);
                uVar3 = (u_long)(pkt + 0x24);
                uVar8 = (u_long)(pkt + 0x2a);
                uVar7 = (u_long)(pkt + 0x30);
                gte_stsz4((u_long *)uVar3, (u_long *)uVar8, (u_long *)uVar7, pkt + 0x36);
                *(short *)((int)pkt + 0x5a) = primitive->clut;
                *(short *)((int)pkt + 0x66) = primitive->tpage;
                FUN_80057b80(puVar5, pkt, 0);
            }
            param_4 = param_4 + -1;
            primitive++;
        } while (param_4 != 0);
    }
    return (u_long *)pkt[5];
}
