#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawImpact(struct tag_EffectSlot *ef);
 *     EFFECT.C:876, 15 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s2       struct tag_EffectSlot * ef
 *     reg   $s1       struct ImpactType * param
 *     reg   $s0       struct Sprite3D * spr
 * END PSX.SYM */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawImpact", DrawImpact);
#else

/*
 * STATUS: NON_MATCHING — 4 of 772 bytes differ (was 28).  Exact 772-byte /
 * 193-instruction extent, exact physical CFG, and exact instruction sequence;
 * the residual is 4 register fields (2 clusters x 2 insns, 1 byte each),
 * mirrored across the green and blue channels: the start-colour load is $v0
 * where the target uses $a0.
 *
 * WHAT FIXED 28 -> 4 (both instances of ONE rule; see the cookbook entry
 * "Give the value the variable's identity"):
 *   - `start = param->start_color.channel.r; start = start * inverse;`
 *     instead of the one-expression `start = ...r * inverse;`.  The load then
 *     IS the `start` pseudo rather than a separate local temp, reproducing the
 *     target's `lbu a0,18(s0) / mult a0,a2 / mflo a0` (all $a0).  This ALSO
 *     dissolved the whole "coupled allocation cycle" the previous checkpoint
 *     described: ratio -> $a3, the end-colour load -> $a1 and the size
 *     quotient -> $v0 all fell out at once.  28 -> 8.
 *   - `start2 = start2 >> 12;` as its own statement before the store, so the
 *     shifted value is `start2` itself: `sra v1,v1,0xc` (was `sra a1,v1,0xc`).
 *     8 -> 4.
 *
 * The previous note's diagnosis was inverted.  It read the four wrong
 * registers as one irreducible cycle needing "new identity/preference
 * evidence"; in fact ratio's $t0 was a SYMPTOM.  The cause was the shared
 * `end_raw` mega-pseudo (6 refs) winning $a3 at allocation slot #9 and exiling
 * ratio (priority 2068, slot #18) to $t0 — confirmed with `regalloc.py
 * --order`.  Splitting `end_raw` per site does fix ratio -> $a3 on its own,
 * but costs +4 bytes; the identity rule above fixes it for free and subsumes
 * it, so `end_raw` stays shared here.
 *
 * ROUND 2 re-derived the residual from scratch and CORRECTS the round-1 note
 * above on three points.  The residual itself is unchanged: 2 clusters,
 * 4 insns, 4 bytes, all register-field-only —
 *     T  lbu a0,17(s0) / mult a0,a2      O  lbu v0,17(s0) / mult v0,a2
 *     T  lbu a0,16(s0) / mult a0,a2      O  lbu v0,16(s0) / mult v0,a2
 * The green/blue start-colour load is $v0; the target uses $a0.  Their product
 * correctly stays $v1 (start2), so the load is a separate pseudo — reg131
 * (green) and reg138 (blue) in the .lreg RTL, each born at the load and dead
 * at the mult.  (Round 1 named "reg126" as the start_colour load; in the
 * current source reg126 is an `ashiftrt` operand.  Its `;; 125 conflicts: 2
 * 29` vs `;; 130 conflicts: 29` grep refers to pseudo numbers that no longer
 * exist — do not start there.)
 *
 * WHY $a0 IS EARNED, AND WHY IT IS OUT OF REACH FROM C HERE.  `start` (p85)
 * gets $a0 because it is live across the size computation, where $v0 (size's
 * end product) and $v1 (red's `start >> 12`) are both occupied — hence its
 * `conflicts: v0 v1` in `regalloc.py --order`.  The green/blue loads occupy a
 * 2-insn window in which nothing else is live, so they conflict with nothing
 * and take the lowest free reg, $v0.  Every lever that grants them those
 * conflicts also perturbs something that already matches:
 *
 *   - Routing green+blue through `start` -> 20 bytes.  NOT for round 1's
 *     stated reason ("extends `start`'s range to $a1").  Measured: p85's refs
 *     go 12 -> 16, and floor_log2 is a STEP (3 -> 4), so its priority jumps
 *     18947 -> 27826 and it moves from allocation slot #4 to #0, where it
 *     takes $a1 and exiles end_raw to $a0.
 *   - ...but the priority is NOT the lever either: routing GREEN ONLY gives
 *     p85 refs 14 / priority exactly 20000 / slot #1, identical
 *     `conflicts: v0 v1` — and it STILL takes $a1 (20 bytes).  p85 takes $a0
 *     at slot #4 and $a1 at slots #0 and #1 with the same conflict set, so the
 *     deciding factor is find_reg's register CHOICE, not the order.
 *     regalloc.py --order self-validates the ORDER only; its "takes the lowest
 *     free register each time" model does not predict this (the FUN_80036284
 *     caveat).  Any plan of the form "move pseudo X to slot N so it takes the
 *     lowest free reg" must be MEASURED, not derived.
 *   - A shared green+blue load carrier (`s32 col; col = ...g; start2 = col *
 *     inverse;`) is a literal NO-OP — `nullcheck.py` exits 1, identical
 *     codegen.  It DOES do what it looks like (col is promoted out of
 *     local_alloc into a global allocno, p87), but with refs 4 / live 4 its
 *     priority is 20000 -> slot #1, where nothing conflicts and it takes $v0
 *     anyway.  Recorded because it looks promising and costs a round.
 *   - The same carrier extended to all THREE channels -> 24 bytes.  The $v0 /
 *     $v1 values live across red's load are GLOBAL allocnos (p123 = size's end
 *     product, p124 = red's `start >> 12`), not local qtys, so they inject
 *     allocno conflicts, not hard-reg conflicts; col at slot #1 is coloured
 *     BEFORE them, grabs $v0 and exiles them (ratio -> $t0, end_raw -> $a3,
 *     red's load -> $v0).
 *   - Hoisting the carrier's load one statement (above the preceding channel's
 *     store) -> 764 bytes, LENGTH MISMATCH.  The target's two load-delay nops
 *     (0x80034024, 0x80034064) are load-bearing: the hoist lets the scheduler
 *     fill BOTH (2 x 4 = the 8 missing bytes).  So the load must be born
 *     immediately before its mult and NO range-extension lever exists on the
 *     load side — which is what closes off the conflict-injection route.
 *
 * Also measured and rejected: operand swap `inverse * ...` (4, but emits
 * `mult a2,v0`); `end_raw` per-site split (776); full-inline `/0x1000` (784);
 * hoisting the load before `size` (45); a named `end_q` (776, kills the
 * delay-slot fill).  The `do{}while(0)` below is LOAD-BEARING (fence-unwrap
 * costs 5).  autorules: 54 candidates, nothing — and this run DID include the
 * repaired `binop-operand-seed` (20 real scores, no `invalid`).  permuter:
 * 18715 iterations, plateaued at its base score, no candidate below it.
 * `siblingdiff --demo` is a dead end (demo body 236 bytes vs retail 772,
 * 6/193 insns); the PSX.SYM local list above describes the DEMO body.
 *
 * ROUND 3 independently RE-DERIVED the local_alloc claim above from raw RTL
 * (not just regalloc.py's summary) and it holds; one more lever was measured
 * and closed.  All facts below cross-check ROUND 2; none of it moves the 4.
 *
 *   - `tools/regalloc.py DrawImpact --order` confirms p131/p138 are absent
 *     from the 19-entry GLOBAL allocno table (`;; 19 regs to allocate: 86 127
 *     134 141 85 123 87 90 92 88 93 81 84 124 82 117 97 89 80` — matches
 *     `cc1says.py --pass greg` exactly) and appear only in the LOCAL-ONLY
 *     list, both `->v0`.
 *   - Reading the raw `.lreg` dump (`tools/rtldump.py DrawImpact --pass
 *     lreg`) rather than trusting the summary: green's load (reg131) is "in
 *     block 8" and so is reg130, and NOTHING ELSE is local to block 8.
 *     reg130 is red's finished `spr->r` sum (born at its `addu`, dead at its
 *     `sb`); reg131 is the green load (born at `lbu`, dead at `mult`) — the
 *     two do not overlap (130 dies exactly when 131 is born), so both
 *     independently take $v0 for free; block 8's "Registers live at start"
 *     lists only cross-block GLOBAL pseudo NUMBERS (80 81 82 84 89 97 124
 *     127), which local_alloc cannot yet be avoiding since it runs BEFORE
 *     global_alloc assigns them a hard reg.  There is no SECOND local
 *     quantity in this window to reorder against — which is exactly the
 *     precondition the AttackBowControl "seed the loser" trick
 *     (docs/matching-cookbook.md) needs and does not have here.
 *   - Proved that absence matters, not just asserted it: mirrored the RED
 *     fix onto GREEN ALONE (not a shared carrier — round 2 tested carriers,
 *     not this) — `start2 = param->start_color.channel.g; start2 = start2 *
 *     inverse;`.  `nullcheck.py` confirms it is a REAL edit (codegen hash
 *     changes), but it merges the load into `start2`'s (p86) own eventual
 *     register, $v1 — not $a0 — emitting `lbu v1,17(s0)` / `mult v1,a2`.
 *     Still 4 bytes, same two clusters, just a different wrong register.
 *     Mechanically this is why: RED's identity trick worked because `start`
 *     (p85) itself already resolves to $a0; GREEN/BLUE's persisting variable
 *     `start2` (p86) resolves to $v1, so identity-merging is GUARANTEED to
 *     reproduce $v1, never $a0.  This closes "give it identity" as a lever
 *     for green/blue specifically (it is not a re-run of round 2's carrier
 *     experiments, which shared the two channels; this is the per-channel,
 *     unshared form regalloc.py's own LOCAL-ONLY hint suggests first).
 *   - Fresh bounded permuter run (240s wall, output redirected to a FILE per
 *     the pipe-hang contract note, current post-cc38bd2 tooling): 20603
 *     iterations, base score 20 never beaten.  Authoritative post-SIGTERM
 *     full-link rescore ties base.c and all 3 retained candidates at exactly
 *     4/4/772 differing bytes.  Independently reconfirms round 2's 18715-
 *     iteration plateau under the now-fixed rescore path.
 *   - `tools/reghist.py`: delta sum 0 (v0 +4 / a0 -4), a pure register-field
 *     swap, no opcode or count difference anywhere in 193 insns.
 *
 * Do not re-open this without a genuinely new SOURCE-STRUCTURE theory: two
 * independent methods (regalloc.py's conflict list; raw .lreg block
 * liveness) now agree the load's 2-insn window is provably conflict-free on
 * our side, and the identity-merge experiment shows the "give it identity"
 * lever is mechanically guaranteed to land on the wrong register here. A
 * tool that, given a LOCAL-ONLY pseudo, printed its block's full local-only
 * roster plus pairwise conflict status directly (regalloc.py --order already
 * does this for GLOBAL allocnos) would have made this cross-check one call
 * instead of hand-correlating cc1says --pass lreg against the raw dump.
 */
#include "effect.h"

typedef union
{
    s32 word;
    struct
    {
        u8 b;
        u8 g;
        u8 r;
        u8 pad;
    } channel;
} ImpactColor;

typedef struct
{
    s32 px;
    s32 py;
    s32 pz;
    GsCOORDINATE2 *super;
    ImpactColor start_color;
    ImpactColor end_color;
    s16 start_size;
    s16 end_size;
    s16 rotate;
    s16 rotate_speed;
    u8 type;
    u8 count;
    u8 time;
} RetailImpactType;

extern GsSPRITE D_800BEAA8[];
extern GsOT *OTablePt;
extern void GsGetLs(GsCOORDINATE2 *coord, MATRIX *m);
extern void GsSetLsMatrix(MATRIX *m);
extern void GetScreenPosition(s32 x, s32 y, s32 z, SVECTOR *scr);
extern s32 RotTransPers(SVECTOR *v0, s32 *sxy, void *p, void *flg);
extern void GsSortSprite(GsSPRITE *spr, GsOT *ot, s32 priority);

void DrawImpact(TEffectSlot *ef)
{
    RetailImpactType *param;
    GsSPRITE *spr;
    SVECTOR scr;
    s32 ratio;
    s32 inverse;
    s32 start;
    s32 start2;
    s32 end;
    s32 end_raw;
    s32 size;
    s32 z;
    s16 scale;
    s32 priority;
    GsCOORDINATE2 *super;

    param = (RetailImpactType *)&ef->param;
    ratio = (param->count << 12) / param->time;
    spr = &D_800BEAA8[param->type];
    spr->rotate = param->rotate << 12;
    inverse = 0x1000 - ratio;

    start = param->start_size * inverse;
    param->rotate = param->rotate + param->rotate_speed;
    if (start < 0)
    {
        start = start + 0xfff;
    }

    size = (start >> 12) + (param->end_size * ratio) / 0x1000;

    start = param->start_color.channel.r;
    start = start * inverse;
    end_raw = param->end_color.channel.r;
    if (start < 0)
    {
        start = start + 0xfff;
    }
    spr->r = (start >> 12) + (end_raw * ratio) / 0x1000;

    start2 = param->start_color.channel.g * inverse;
    end_raw = param->end_color.channel.g;
    if (start2 < 0)
    {
        start2 = start2 + 0xfff;
    }
    start2 = start2 >> 12;
    spr->g = start2 + (end_raw * ratio) / 0x1000;

    start2 = param->start_color.channel.b * inverse;
    end_raw = param->end_color.channel.b;
    if (start2 < 0)
    {
        start2 = start2 + 0xfff;
    }
    start2 = start2 >> 12;
    spr->b = start2 + (end_raw * ratio) / 0x1000;

    do
    {
        end = param->px;
    } while (0);
    start2 = param->py;
    super = param->super;
    inverse = param->pz;
    if (super != 0)
    {
        *(s16 *)0x1f800020 = end;
        *(s16 *)0x1f800022 = start2;
        *(s16 *)0x1f800024 = inverse;
        GsGetLs(super, (MATRIX *)0x1f800000);
        GsSetLsMatrix((MATRIX *)0x1f800000);
        scr.vz = (s16)RotTransPers((SVECTOR *)0x1f800020, (s32 *)&scr,
                                   (void *)0x1f800028,
                                   (void *)0x1f80002c);
    }
    else
    {
        GetScreenPosition(end, start2, inverse, &scr);
    }

    z = scr.vz;
    if (z > 0x24)
    {
        scale = (s16)((size * 300) / z) + 1;
        spr->scaley = scale;
        spr->scalex = scale;
        spr->x = scr.vx;
        spr->y = scr.vy;

        start2 = (s32)((u16)scr.vz << 16) >> 18;
        if (start2 < 0)
        {
            goto zero;
        }
        priority = 0x4e1;
        if (start2 < 0x4e2)
        {
            priority = start2;
        }
        goto done;
    zero:
        priority = 0;
    done:
        GsSortSprite(spr, OTablePt, (u16)priority);
    }

    if (param->count >= param->time)
    {
        ef->proc = 0;
    }
    param->count = param->count + 1;
}

#endif

// triage: MEDIUM — 193 insns, mul/div, 5 callees, ~0.04 to ReqItemShinsoku
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80034050) */
// /* WARNING: Removing unreachable block (ram,0x80034010) */
// /* WARNING: Removing unreachable block (ram,0x80034090) */
// /* DrawImpact? */
//
// void DrawImpact(undefined4 *param_1)
//
// {
//   byte bVar1;
//   short sVar2;
//   int iVar3;
//   long lVar4;
//   uint uVar5;
//   int iVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   uint uVar10;
//   undefined4 local_20;
//   ushort local_1c;
//
//   uVar5 = (uint)*(byte *)((int)param_1 + 0x26);
//   uVar10 = ((uint)*(byte *)((int)param_1 + 0x25) << 0xc) / uVar5;
//   if (uVar5 == 0) {
//     trap(0x1c00);
//   }
//   if ((uVar5 == 0xffffffff) && (*(byte *)((int)param_1 + 0x25) == 0x80000)) {
//     trap(0x1800);
//   }
//   bVar1 = *(byte *)(param_1 + 9);
//   iVar6 = (uint)bVar1 * 0x24;
//   *(int *)(&DAT_800beac8 + iVar6) = (int)*(short *)(param_1 + 8) << 0xc;
//   iVar9 = 0x1000 - uVar10;
//   iVar7 = *(short *)(param_1 + 7) * iVar9;
//   *(short *)(param_1 + 8) = *(short *)(param_1 + 8) + *(short *)((int)param_1 + 0x22);
//   if (iVar7 < 0) {
//     iVar7 = iVar7 + 0xfff;
//   }
//   iVar3 = (int)*(short *)((int)param_1 + 0x1e) * uVar10;
//   if (iVar3 < 0) {
//     iVar3 = iVar3 + 0xfff;
//   }
//   iVar8 = (uint)*(byte *)((int)param_1 + 0x16) * iVar9;
//   if (iVar8 < 0) {
//     iVar8 = iVar8 + 0xfff;
//   }
//   (&DAT_800beabc)[iVar6] =
//        (char)(iVar8 >> 0xc) + (char)((int)(*(byte *)((int)param_1 + 0x1a) * uVar10) >> 0xc);
//   iVar8 = (uint)*(byte *)((int)param_1 + 0x15) * iVar9;
//   if (iVar8 < 0) {
//     iVar8 = iVar8 + 0xfff;
//   }
//   (&DAT_800beabd)[iVar6] =
//        (char)(iVar8 >> 0xc) + (char)((int)(*(byte *)((int)param_1 + 0x19) * uVar10) >> 0xc);
//   iVar9 = (uint)*(byte *)(param_1 + 5) * iVar9;
//   if (iVar9 < 0) {
//     iVar9 = iVar9 + 0xfff;
//   }
//   (&DAT_800beabe)[iVar6] =
//        (char)(iVar9 >> 0xc) + (char)((int)(*(byte *)(param_1 + 6) * uVar10) >> 0xc);
//   if ((GsCOORDINATE2 *)param_1[4] == (GsCOORDINATE2 *)0x0) {
//     GetScreenPosition(param_1[1],param_1[2],param_1[3],&local_20);
//   }
//   else {
//     Scratchpad._32_2_ = (undefined2)param_1[1];
//     Scratchpad._34_2_ = (undefined2)param_1[2];
//     Scratchpad._36_2_ = (undefined2)param_1[3];
//     GsGetLs((GsCOORDINATE2 *)param_1[4],(MATRIX *)Scratchpad);
//     GsSetLsMatrix((MATRIX *)Scratchpad);
//     lVar4 = RotTransPers((SVECTOR *)(Scratchpad + 0x20),&local_20,(long *)(Scratchpad + 0x28),
//                          (long *)(Scratchpad + 0x2c));
//     local_1c = (ushort)lVar4;
//   }
//   iVar9 = (int)(short)local_1c;
//   if (0x24 < iVar9) {
//     iVar7 = ((iVar7 >> 0xc) + (iVar3 >> 0xc)) * 300;
//     if (iVar9 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar9 == -1) && (iVar7 == -0x80000000)) {
//       trap(0x1800);
//     }
//     sVar2 = (short)(iVar7 / iVar9) + 1;
//     *(short *)(&DAT_800beac6 + iVar6) = sVar2;
//     *(short *)(&DAT_800beac4 + iVar6) = sVar2;
//     *(undefined2 *)(&DAT_800beaac + iVar6) = (undefined2)local_20;
//     *(undefined2 *)(&DAT_800beaae + iVar6) = local_20._2_2_;
//     iVar7 = (int)((uint)local_1c << 0x10) >> 0x12;
//     if (iVar7 < 0) {
//       iVar9 = 0;
//     }
//     else {
//       iVar9 = 0x4e1;
//       if (iVar7 < 0x4e2) {
//         iVar9 = iVar7;
//       }
//     }
//     GsSortSprite((GsSPRITE *)(&DAT_800beaa8 + (uint)bVar1 * 9),OTablePt,(ushort)iVar9);
//   }
//   if (*(byte *)((int)param_1 + 0x26) <= *(byte *)((int)param_1 + 0x25)) {
//     *param_1 = 0;
//   }
//   *(char *)((int)param_1 + 0x25) = *(char *)((int)param_1 + 0x25) + '\x01';
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? GetScreenPosition(s32, s32, s32, u16 *);          /* extern */
// ? GsGetLs(s32, ?, s32, s32);                        /* extern */
// ? GsSetLsMatrix(?);                                 /* extern */
// ? GsSortSprite(void *, s32, s32);                   /* extern */
// s16 RotTransPers(?, u16 *, ?, ?);                   /* extern */
// extern ? D_800BEAA8;
// extern s32 OTablePt;
//
// void DrawImpact(void *arg0) {
//     u16 sp10;
//     s16 sp14;
//     s16 temp_v0_2;
//     s32 temp_a0;
//     s32 temp_a2;
//     s32 temp_a2_2;
//     s32 temp_lo;
//     s32 temp_v0;
//     s32 temp_v1;
//     s32 temp_v1_2;
//     s32 var_a0;
//     s32 var_a0_2;
//     s32 var_a2;
//     s32 var_v0;
//     s32 var_v0_2;
//     s32 var_v0_3;
//     s32 var_v0_4;
//     s32 var_v1;
//     s32 var_v1_2;
//     void *temp_s0;
//     void *temp_s1;
//
//     temp_s0 = arg0 + 4;
//     temp_lo = (s32) (temp_s0->unk21 << 0xC) / (s32) temp_s0->unk22;
//     temp_s1 = (temp_s0->unk20 * 0x24) + &D_800BEAA8;
//     temp_s1->unk20 = (s32) (temp_s0->unk1C << 0xC);
//     temp_a2 = 0x1000 - temp_lo;
//     var_a0 = temp_s0->unk18 * temp_a2;
//     temp_s0->unk1C = (s16) ((u16) temp_s0->unk1C + temp_s0->unk1E);
//     if (var_a0 < 0) {
//         var_a0 += 0xFFF;
//     }
//     var_v0 = temp_s0->unk1A * temp_lo;
//     if (var_v0 < 0) {
//         var_v0 += 0xFFF;
//     }
//     var_a0_2 = temp_s0->unk12 * temp_a2;
//     if (var_a0_2 < 0) {
//         var_a0_2 += 0xFFF;
//     }
//     var_v0_2 = temp_s0->unk16 * temp_lo;
//     if (var_v0_2 < 0) {
//         var_v0_2 += 0xFFF;
//     }
//     temp_s1->unk14 = (s8) ((var_a0_2 >> 0xC) + (var_v0_2 >> 0xC));
//     var_v1 = temp_s0->unk11 * temp_a2;
//     if (var_v1 < 0) {
//         var_v1 += 0xFFF;
//     }
//     var_v0_3 = temp_s0->unk15 * temp_lo;
//     if (var_v0_3 < 0) {
//         var_v0_3 += 0xFFF;
//     }
//     temp_s1->unk15 = (s8) ((var_v1 >> 0xC) + (var_v0_3 >> 0xC));
//     var_v1_2 = temp_s0->unk10 * temp_a2;
//     if (var_v1_2 < 0) {
//         var_v1_2 += 0xFFF;
//     }
//     var_v0_4 = temp_s0->unk14 * temp_lo;
//     if (var_v0_4 < 0) {
//         var_v0_4 += 0xFFF;
//     }
//     temp_s1->unk16 = (s8) ((var_v1_2 >> 0xC) + (var_v0_4 >> 0xC));
//     temp_v0 = arg0->unk4;
//     temp_v1 = temp_s0->unk4;
//     temp_a0 = temp_s0->unkC;
//     temp_a2_2 = temp_s0->unk8;
//     if (temp_a0 != 0) {
//         *(s16 *)0x1F800020 = (s16) temp_v0;
//         *(s16 *)0x1F800022 = (s16) temp_v1;
//         *(s16 *)0x1F800024 = (s16) temp_a2_2;
//         GsGetLs(temp_a0, 0x1F800000, temp_a2_2, temp_lo);
//         GsSetLsMatrix(0x1F800000);
//         sp14 = RotTransPers(0x1F800020, &sp10, 0x1F800028, 0x1F80002C);
//     } else {
//         GetScreenPosition(temp_v0, temp_v1, temp_a2_2, &sp10);
//     }
//     if (sp14 >= 0x25) {
//         temp_v0_2 = ((s32) (((var_a0 >> 0xC) + (var_v0 >> 0xC)) * 0x12C) / sp14) + 1;
//         temp_s1->unk1E = temp_v0_2;
//         temp_s1->unk1C = temp_v0_2;
//         temp_s1->unk4 = sp10;
//         temp_s1->unk6 = sp12;
//         temp_v1_2 = (s32) ((u16) sp14 << 0x10) >> 0x12;
//         if (temp_v1_2 >= 0) {
//             var_a2 = 0x4E1;
//             if (temp_v1_2 < 0x4E2) {
//                 var_a2 = temp_v1_2;
//             }
//         } else {
//             var_a2 = 0;
//         }
//         GsSortSprite(temp_s1, OTablePt, var_a2 & 0xFFFF);
//     }
//     if ((u8) temp_s0->unk21 >= (u8) temp_s0->unk22) {
//         arg0->unk0 = 0;
//     }
//     temp_s0->unk21 = (u8) (temp_s0->unk21 + 1);
// }
