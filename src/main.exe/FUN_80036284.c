#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsOT *OTablePt;
 *
 * PSX.SYM suggests this may be `SetGore` (LOW confidence, EFFECT.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

typedef struct
{
    u_long tag;
    u_long code[1];
} DR_TPAGE;

typedef struct
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
} POLY_F4;

typedef struct
{
    DR_TPAGE tpage;
    POLY_F4 ply;
} POLY_XF4;

struct GsOT
{
    u32 length;
    u32 *org;
    u32 offset;
    u32 point;
    u32 *tag;
};

extern long GameClock;
extern GsOT *OTablePt;
extern void SetPolyXF4(POLY_XF4 *ply, short attrib);
extern void AddXF4(void *ot, POLY_XF4 *ply);
extern void *GsGetWorkBase(void);
extern void GsSetWorkBase(void *workBase);

/*
 * STATUS: NON_MATCHING — 34 of 728 bytes differ. Exact target length (170
 * insns), exact instruction sequence, exact stack/calls/branches. Every
 * differing byte is a register FIELD in one a0/a2 rotation: the target holds
 * the interval end (param->pz) in a0 and the duration in a2; this draft
 * holds them in a2/a0. That single swap cascades into the six division
 * results, because each quotient lands on whichever register pz/duration
 * just vacated. Direct color-field assignments were 24 bytes too long (each
 * expanded divu needed an mflo hazard nop); computing the three channel
 * quotients first and grouping the stores restores the exact 0x2d8 shape.
 *
 * The PRIORITY TABLE IS NOT THE LEVER — measured, not assumed:
 *   regalloc.py --order:  p84 duration refs=14 live=30 -> 14000 (alloc #6, a0)
 *                         p85 pz       refs= 4 live=16 ->  5000 (alloc #11, a2)
 *   regalloc.py --compare 85 84: "p85 > p84: needs +4 weighted ref(s)".
 * That +4 is unreachable, and the target did not do it that way:
 *   - duration's refs are 14 in the TARGET too — countable in its own asm:
 *     1 def (subu a2,a0,v0) + 12 (six divu/bnez pairs) + 1 (subu v1,a2,a1).
 *     Identical to p84's printed refs, so the two tables agree on refs.
 *   - pz has 3 mentions on BOTH sides (lw / subu / sltu). For pz to outrank
 *     duration it needs floor_log2(4)*4/live*10000 > 14000, i.e. live <= 5.
 *     In the target's own asm pz is def'd at 0x800362e4 and last used at
 *     0x8003631c — 14 insns apart, and FURTHER apart pre-reorg (reorg stole
 *     that sltu into the dispatch delay slot from case 2's block). So the
 *     target's pz live range is >= 14 and can never reach <= 5.
 * Conclusion: allocation ORDER is the same on both sides; the divergence is
 * in find_reg's exclusion set / pre-RA insn order, not in the qsort order.
 * Do not spend another round on re-weighting pz.
 *
 * Levers TRIED and MEASURED (nullcheck.py verdict in brackets):
 *   - separate `remaining = duration - elapsed` local  [NO-OP, identical hash]
 *     The "two registers = two variables" reading fails here: gcc-2.8.1 has
 *     no coalescing pass, and global_conflicts processes REG_DEAD notes
 *     BEFORE mark_reg_store, so `elapsed` dies AT the subu, never conflicts
 *     with `remaining`, and the two allocnos merge onto one hard register.
 *   - per-site split of duration (`u32 d0 = duration;` per case)  [NO-OP]
 *     cse1 folds the copy back onto `duration` before global_alloc sees two
 *     allocnos. A copy-based split of a GLOBAL allocno cannot demote it.
 *   - explicit `end = param->pz` local hoisted for case 2        [NO-OP]
 *   - guard moved before the `duration - elapsed` producer       [NO-OP]
 *   - mode read last / mid / switch on param->unk10 directly     [34, no change]
 *   - duration computed before elapsed                    [REAL, 34 -> 70]
 *     Statement order is the one class that does move bytes (it changes the
 *     pre-RA RTL order regalloc sees, which sched2 later normalizes) — this
 *     direction is simply wrong. That is where a future round should dig.
 *
 * ROUND 2 (2026-07-18): re-verified baseline (34, matches this file and the
 * whole image) and re-ran the full ladder fresh, since round 1's
 * binop-operand-seed candidates were compiled against a since-fixed C89 bug
 * (48eba95) and its permuter run predated -fno-builtin reaching the search
 * in this same session (d02da20). Both re-runs, done properly this time,
 * agree with round 1's conclusion — recorded here so round 3 does not repeat
 * either:
 *   - reghist.py: delta sum +0 across 5 differing registers (v0 -4, a0 +11,
 *     a1 +2, a2 -7, a3 -2) — confirms pure rename/rotation, no structural fix
 *     on offer.
 *   - autorules.py (plain): 32 candidates now compile (the binop-operand-seed
 *     fix landed), including both operands at L138/L139 — still no improving
 *     edit. The round-1 "tooling gap" is closed; it was not hiding a win here.
 *   - tools/rtlguide.py: independently reaches round 1's hand-derived
 *     conclusion and sharpens it — "owner: regalloc" and explicit
 *     `HARD-CONFLICT` lines: "$a0 -> $v0 x4: locals duration,b ... target
 *     hard-conflicts p84,p87" and "$a1 -> $v1 x4: locals elapsed ... target
 *     hard-conflicts p83". Priority/weighting is PROVABLY insufficient, not
 *     just empirically stuck.
 *   - autorules.py --guided (rtlguide-directed, beam depth 2, every rule the
 *     tool listed as relevant to lines 139-177, 160 compiled candidates): no
 *     improving path.
 *   - ONE bounded FOREGROUND permuter run (240s search + authoritative
 *     rescore, done AFTER confirming -fno-builtin is active in both
 *     Build.hs and permute.py): retained 7 candidates at 31/32/32/33/33/34
 *     (base)/37 whole-image bytes. Best-looking candidate (31) is a REAL
 *     score improvement but PROVEN NOT ADOPTABLE: every retained candidate
 *     is the same mutation family — reuse `duration` as a mutable scratch
 *     for case 2's `g` quotient before computing `b`, e.g. `duration =
 *     (elapsed*unk1)/duration; g = duration; b = (elapsed*unk2)/duration;`
 *     (or the unk2-first variant). The target's raw asm disproves this
 *     directly: `$a2` (duration) is written EXACTLY ONCE, at 0x800362f0
 *     (`subu a2,a0,v0`), and READ seven times across all six divisions
 *     (0x80036340/8003636c/80036398 for case 0, 0x8003645c/80036488/800364b4
 *     for case 2) with NO other write to `$a2` anywhere in the function —
 *     `grep -n '\$a2' FUN_80036284.s` confirms it. Any source that
 *     reassigns `duration` before the third division cannot reproduce that
 *     dependency chain; the lower whole-image score is register-pressure
 *     fallout elsewhere in the function, not progress toward the target's
 *     shape (the runbook's "state that scores better while being further
 *     from the target's shape" class). Do not re-permute this function
 *     expecting a different family — the local search space is currently
 *     dominated by this one trap; a future attempt should seed the permuter
 *     away from case 2's `g`/`duration` statements or just distrust any
 *     candidate touching them.
 *   - Manual `.greg` RTL trace (raw dump, not just regalloc.py's summary)
 *     explains mechanically why THIS draft lands duration/pz where it does:
 *     p84 (duration) is processed at allocation position #6, immediately
 *     after the six same-priority (15000) mult/div product/quotient temps
 *     (positions 0-5), none of which claim v0 or a0 in this draft — so a0 is
 *     p84's first numerically-free register (v0/v1 excluded by direct
 *     conflict). p85 (pz) is processed at #11; by then a0 (p84,p87) and a1
 *     (p83/elapsed) are both taken and both conflict with p85, so it falls
 *     through to a2. The open thread: the target's OWN six per-channel
 *     temps spread differently — case 0's r lands in $a0, case 2's r in
 *     $a1, both cases' b in $v0 (read straight off the target asm: mflo a0
 *     @0x80036350, mflo a1 @0x8003646c, mflo v0 @0x800363a8/0x800364c4) —
 *     versus this draft's six temps landing in v1/v1/t4/v1/v1/a3 (all
 *     avoiding v0 and a0 entirely). WHAT excludes v0 from all six of this
 *     draft's temps when the target only partially excludes it is the
 *     as-yet-unresolved root cause upstream of the duration/pz swap this
 *     file has focused on; round 1's `p85 vs p84 priority` framing was a
 *     downstream symptom, not the cause. A future round should chase THAT
 *     question (why $v0 is unavailable to the first of the six mult/div
 *     temps here — cc1says shows it happens inside the `Spilling reg
 *     8/9/10/11/64/65` HI/LO/MD_REGS sub-pass, before the general GR_REGS
 *     allocation even starts) rather than re-deriving the pz/duration
 *     priority math, which is now settled twice.
 *
 * ROUND 3 (2026-07-18): fast-forwarded to master HEAD (a21a758), re-confirmed
 * baseline 34 (file + whole image). This round SETTLES the round-1/2 open
 * thread and closes off the permuter as a clean lever:
 *   - FRESH permuter run from base (my own, -fno-builtin confirmed active in
 *     Build.hs + permute.py): reaches 16 whole-image bytes — LOWER than round
 *     2's 31, and a DIFFERENT mutation family. It is NOT the "reuse duration
 *     as scratch" trap round 2 warned about; it is a case-0 register-tie
 *     (q0 computed into b, then copied to r; g copied to new_var2):
 *       b = (elapsed*unk0)/duration;  r = b;
 *       g = (elapsed*unk1)/duration;  new_var2 = g;
 *       b = (elapsed*unk2)/duration;  then stores r0=r, g0=new_var2, b0=b
 *     The copy `r = b` register-ties r<->b, which perturbs the GLOBAL coloring
 *     and FIXES the core divergence: duration lands in a2 and pz in a0 (p84->a2),
 *     exactly the target. Verified by matchdiff: after this edit the entry-block
 *     lw/subu/sltu diffs and the case-0 divisor all vanish; residual 16 is just
 *     (a) case-0 b: v0(tgt) vs a1, and (b) case-2 new_elapsed: v1(tgt) vs a1
 *     plus its product cascade.
 *   - RE-SEEDED the permuter from that 16 (contract's re-seed protocol): it
 *     reaches 12 — but ONLY by STACKING another seed-temp copy in case 2
 *     (`r = elapsed; r = (r*unk0)/duration;`). So the 34->16->12 ladder is a
 *     chain of human-implausible register-tie copies; each improving edit is a
 *     synthetic seed temp. REJECTED per the owner's standing directive (prefer
 *     clean human structure even at higher bytes; SetWire 70->80,
 *     mission_score_screen 145->187). Kept the clean 34.
 *   - This is now DIAGNOSTIC-COMPLETE: the target's case-0 AND case-2 both
 *     compute the channels in unk0,unk1,unk2 ORDER and store r0,g0,b0 — read
 *     straight off the target asm (mult v1,v0 x3 then sb x3 in each case), i.e.
 *     the target's SOURCE STRUCTURE IS IDENTICAL TO THIS DRAFT'S. The only
 *     divergence is the register coloring, and the permuter proves it is
 *     drivable to a match ONLY through register-tie copies no human writes.
 *     The mechanism (fresh .greg trace): duration=p84 is coloured 7th and
 *     takes a0 as its first numerically-free reg (v0/v1/t4/a3 excluded by the
 *     six 15000-priority temps); reaching the target's duration->a2 requires a0
 *     ALSO excluded at p84's turn, which needs a conflicting value tied into a0
 *     — achievable only by an artificial copy. No clean C creates that tie.
 *   - New NO-OPs confirmed this round (nullcheck identical-hash): separate
 *     case-2 variable `nel = duration - elapsed` (RE-confirmed — the old
 *     elapsed is dead at the subu so the pseudo colours to a1 regardless of
 *     name); and reordering the scalar local DECLARATIONS (u32 b/u16 g/u32 r)
 *     — gcc defers scalar pseudo allocation to first-use, so declaration order
 *     cannot renumber the allocnos. Neither is a lever.
 * Bottom line for round 4: do NOT re-run the permuter expecting a clean win —
 * its entire improving neighbourhood here is seed-temp register-ties. This is a
 * genuine sub-C coloring tie with a structurally-identical source; leave parked.
 *
 * Keep this pure-C draft guarded; do not replace the residual with inline asm.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80036284", FUN_80036284);
#else /* NON_MATCHING */

void FUN_80036284(TEffectSlot *ef)
{
    XF4Type *param;
    POLY_XF4 local;
    POLY_XF4 *ply;
    long elapsed;
    u32 duration;
    u32 r;
    u16 g;
    u32 b;
    u8 mode;

    param = &ef->param.xf4;
    SetPolyXF4(&local, 1);
    local.ply.x0 = -0xA0;
    local.ply.y0 = -0x78;
    local.ply.x1 = 0xA0;
    local.ply.y1 = -0x78;
    local.ply.x2 = -0xA0;
    local.ply.y2 = 0x78;
    local.ply.x3 = 0xA0;
    local.ply.y3 = 0x78;

    mode = param->unk10;
    elapsed = GameClock - param->py;
    duration = param->pz - param->py;
    switch (mode)
    {
    case 0:
        r = (elapsed * param->unk0) / duration;
        g = (elapsed * param->unk1) / duration;
        b = (elapsed * param->unk2) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        if ((u32)GameClock >= (u32)param->pz)
        {
            param->unk10 = param->unk10 + 1;
            param->pz = param->pz + 3;
        }
        break;
    case 1:
        local.ply.r0 = param->unk0;
        local.ply.g0 = param->unk1;
        local.ply.b0 = param->unk2;
        if ((u32)GameClock >= (u32)param->pz)
        {
            param->unk10 = param->unk10 + 1;
            param->pz = param->pz + 0x28;
        }
        break;
    case 2:
        elapsed = duration - elapsed;
        if ((u32)GameClock >= (u32)param->pz)
        {
            ef->proc = 0;
            return;
        }
        r = (elapsed * param->unk0) / duration;
        g = (elapsed * param->unk1) / duration;
        b = (elapsed * param->unk2) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        break;
    }

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    *ply = local;
    AddXF4(OTablePt->org + param->px, ply);
}

#endif /* NON_MATCHING */

// triage: MEDIUM — 182 insns, mul/div, 4 callees, ~0.07 to FUN_80038c0c
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80036284(undefined4 *param_1)
//
// {
//   byte bVar1;
//   char cVar2;
//   GsOT *pGVar3;
//   POLY_XF4 *ply;
//   int iVar4;
//   uint uVar5;
//   POLY_XF4 local_30;
//
//   SetPolyXF4(&local_30,1);
//   local_30.ply.x0 = -0xa0;
//   local_30.ply.y0 = -0x78;
//   local_30.ply.x1 = 0xa0;
//   local_30.ply.y1 = -0x78;
//   local_30.ply.x2 = -0xa0;
//   local_30.ply.y2 = 0x78;
//   local_30.ply.x3 = 0xa0;
//   local_30.ply.y3 = 0x78;
//   bVar1 = *(byte *)(param_1 + 5);
//   iVar4 = GameClock - param_1[3];
//   uVar5 = param_1[4] - param_1[3];
//   if (bVar1 == 1) {
//     local_30.ply._4_3_ = *(undefined3 *)(param_1 + 1);
//     if ((uint)GameClock < (uint)param_1[4]) goto LAB_800364d4;
//     cVar2 = *(char *)(param_1 + 5);
//     iVar4 = param_1[4] + 0x28;
//   }
//   else {
//     if (1 < bVar1) {
//       if (bVar1 == 2) {
//         iVar4 = uVar5 - iVar4;
//         if ((uint)param_1[4] <= (uint)GameClock) {
//           *param_1 = 0;
//           return;
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         local_30.ply.g0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 5)) / uVar5);
//         local_30.ply.r0 = (u_char)((iVar4 * (uint)*(byte *)(param_1 + 1)) / uVar5);
//         local_30.ply.b0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 6)) / uVar5);
//       }
//       goto LAB_800364d4;
//     }
//     if (bVar1 != 0) goto LAB_800364d4;
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     local_30.ply.g0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 5)) / uVar5);
//     local_30.ply.r0 = (u_char)((iVar4 * (uint)*(byte *)(param_1 + 1)) / uVar5);
//     local_30.ply.b0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 6)) / uVar5);
//     if ((uint)GameClock < (uint)param_1[4]) goto LAB_800364d4;
//     cVar2 = *(char *)(param_1 + 5);
//     iVar4 = param_1[4] + 3;
//   }
//   *(char *)(param_1 + 5) = cVar2 + '\x01';
//   param_1[4] = iVar4;
// LAB_800364d4:
//   ply = (POLY_XF4 *)GsGetWorkBase();
//   GsSetWorkBase(ply + 1);
//   pGVar3 = OTablePt;
//   (ply->tpage).tag = local_30.tpage.tag;
//   (ply->tpage).code[0] = local_30.tpage.code[0];
//   (ply->ply).tag = local_30.ply.tag;
//   (ply->ply).r0 = local_30.ply.r0;
//   (ply->ply).g0 = local_30.ply.g0;
//   (ply->ply).b0 = local_30.ply.b0;
//   (ply->ply).code = local_30.ply.code;
//   (ply->ply).x0 = local_30.ply.x0;
//   (ply->ply).y0 = local_30.ply.y0;
//   (ply->ply).x1 = local_30.ply.x1;
//   (ply->ply).y1 = local_30.ply.y1;
//   (ply->ply).x2 = local_30.ply.x2;
//   (ply->ply).y2 = local_30.ply.y2;
//   (ply->ply).x3 = local_30.ply.x3;
//   (ply->ply).y3 = local_30.ply.y3;
//   AddXF4((undefined *)(pGVar3->org + param_1[2]),ply);
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AddXF4(s32, void *);                              /* extern */
// void *GsGetWorkBase(u32, u32, u32, u32);            /* extern */
// ? GsSetWorkBase(s32);                               /* extern */
// ? SetPolyXF4(s32 *, ?);                             /* extern */
// extern u32 GameClock;
// extern void *OTablePt;
//
// void FUN_80036284(void *arg0) {
//     s32 sp10;
//     u8 sp1C;
//     u8 sp1D;
//     u8 sp1E;
//     s16 sp20;
//     s16 sp22;
//     s16 sp24;
//     s16 sp26;
//     s16 sp28;
//     s16 sp2A;
//     s16 sp2C;
//     s16 sp2E;
//     s32 temp_v0;
//     s32 temp_v1_2;
//     u32 temp_a2;
//     u32 var_a0;
//     u32 var_a1;
//     u32 var_a3;
//     u32 var_v1;
//     u8 temp_v1;
//     u8 var_v0;
//     void *temp_s1;
//     void *temp_v0_2;
//
//     temp_s1 = arg0 + 4;
//     SetPolyXF4(&sp10, 1);
//     var_a3 = GameClock;
//     sp22 = -0x78;
//     sp26 = -0x78;
//     sp20 = -0xA0;
//     sp24 = 0xA0;
//     sp28 = -0xA0;
//     sp2A = 0x78;
//     sp2C = 0xA0;
//     sp2E = 0x78;
//     temp_v0 = temp_s1->unk8;
//     var_a0 = temp_s1->unkC;
//     temp_v1 = temp_s1->unk10;
//     var_a1 = var_a3 - temp_v0;
//     temp_a2 = var_a0 - temp_v0;
//     switch (temp_v1) {                              /* irregular */
//     case 0:
//         var_a0 = (u32) (var_a1 * arg0->unk4) / temp_a2;
//         sp1C = (u8) var_a0;
//         sp1D = (u8) ((u32) (var_a1 * temp_s1->unk1) / temp_a2);
//         sp1E = (u8) ((u32) (var_a1 * temp_s1->unk2) / temp_a2);
//         if (var_a3 >= (u32) temp_s1->unkC) {
//             var_v0 = temp_s1->unk10 + 1;
//             var_v1 = temp_s1->unkC + 3;
// block_10:
//             temp_s1->unk10 = var_v0;
//             temp_s1->unkC = var_v1;
//         }
//     default:
// block_14:
//         temp_v0_2 = GsGetWorkBase(var_a0, var_a1, temp_a2, var_a3);
//         GsSetWorkBase(temp_v0_2 + 0x20);
//         temp_v0_2->unk0 = sp10;
//         temp_v0_2->unk4 = sp14;
//         temp_v0_2->unk8 = sp18;
//         temp_v0_2->unkC = (s32) sp1C;
//         temp_v0_2->unk10 = (s32) sp20;
//         temp_v0_2->unk14 = (s32) sp24;
//         temp_v0_2->unk18 = (s32) sp28;
//         temp_v0_2->unk1C = (s32) sp2C;
//         AddXF4(OTablePt->unk4 + (temp_s1->unk4 * 4), temp_v0_2);
//         return;
//     case 1:
//         sp1C = arg0->unk4;
//         sp1D = temp_s1->unk1;
//         sp1E = temp_s1->unk2;
//         if (var_a3 >= (u32) temp_s1->unkC) {
//             var_v0 = temp_s1->unk10 + 1;
//             var_v1 = temp_s1->unkC + 0x28;
//             goto block_10;
//         }
//         goto block_14;
//     case 2:
//         temp_v1_2 = temp_a2 - var_a1;
//         if (var_a3 >= var_a0) {
//             arg0->unk0 = 0;
//             return;
//         }
//         var_a1 = (u32) (temp_v1_2 * arg0->unk4) / temp_a2;
//         var_a3 = temp_v1_2 * temp_s1->unk1;
//         var_a0 = var_a3 / temp_a2;
//         sp1C = (u8) var_a1;
//         sp1D = (u8) var_a0;
//         sp1E = (u8) ((u32) (temp_v1_2 * temp_s1->unk2) / temp_a2);
//         goto block_14;
//     }
// }
