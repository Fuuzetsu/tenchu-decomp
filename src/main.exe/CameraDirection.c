#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CameraDirection(struct Humanoid *pl, struct GsRVIEW2 *vDif);
 *     CAMERA.C:898, 114 src lines, frame 160 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct Humanoid * pl
 *     param $s3       struct GsRVIEW2 * vDif
 *     stack sp+24     struct GsRVIEW2 target
 *     reg   $s2       struct ModelArchiveType * mad
 *     stack sp+56     struct SVECTOR r
 *     stack sp+64     struct SVECTOR CamLoc
 *     stack sp+72     struct VECTOR vc
 *     stack sp+88     struct MATRIX mat
 *     stack sp+120    int rx
 *     stack sp+124    int ry
 *     stack sp+128    short x
 *     stack sp+130    short y
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — exact retail extent (860 bytes / 215 instructions)
 * with 7 differing linked bytes: 5 register-only instructions in two aligned
 * hunks, both inside the `target.vrx`/`r.vx` computation at 0x80031e98..
 * 0x80031ec4.  Calls and control flow match exactly (6 calls, 14 conditional
 * branches, 4 jumps, 1 return); every OTHER register in the function's 215
 * instructions matches target exactly (`reghist` delta sum 0, opcodes equal).
 *
 * Round history (each step permuter-found, then verified with matchdiff and
 * narrowed by hand):
 *   860 exact-length baseline -> 17 bytes: prior park (see git history for
 *     the original vx/vy/vz init-order and donor-pair reasoning).
 *   17 -> 12: `y = ViewInfo.vrx; vDif->vrx = (target.vrx - y) / 4;` — naming
 *     ViewInfo.vrx through the already-declared (and by this point dead) `y`
 *     local, instead of reading the extern inline, fixed the REORDER cluster:
 *     the `target.vrx - ViewInfo.vrx` subtraction now schedules immediately
 *     after target.vrx's first computation (matching retail) instead of
 *     after target.vpx.  Permuter round 1, candidate output-60-1.
 *   12 -> 7: wrapping the whole target-fields/vDif tail (`target.vrx = ...`
 *     through `vDif->vpz = ...`, i.e. everything from the first target.
 *     field assignment to the end of the function) in one do-while-0 fence.
 *     This is a pure QTY_CMP_PRI reweighting fence (no runtime effect: no
 *     break/continue/goto crosses it) — bisected by hand from the
 *     permuter's original candidate (which wrapped nearly the entire
 *     function body, including the DirectionRY clamp/sin/cos/RotateVectorS/
 *     CamLoc setup, all of which turned out to be dead weight: the same 7
 *     bytes hold with only the tail wrapped).  Narrower cuts were tried and
 *     both REGRESSED THE WHOLE-FUNCTION LENGTH (excluding the vDif-> stores
 *     from the fence but keeping target.* inside: 864 bytes; cutting further
 *     to fence only target.*: 868 bytes) — the vDif-> tail must stay inside
 *     the fence.  Permuter round 2, candidate output-30-1.
 *
 * The residual 7 bytes are a single LOCAL-ONLY quantity-INTERFERENCE tie
 * between r.vx (pseudo 220, wants target's $t0) and the `CamLoc.vx - r.vx`
 * temp that becomes target.vrx (pseudo 224, wants target's $v0) — our compile
 * has them the other way around ($v0 for r.vx, $t0 for the temp).
 *
 * `tools/regalloc.py --local` now prints and SELF-VALIDATES the block_alloc
 * walk against cc1's printed homes (this postdates the older note that "no
 * tool computes LOCAL qty arithmetic" — it does now).  The walk for block 17
 * (the target/vDif tail) shows the tie is NOT a plain priority flip:
 *     p220 r.vx           refs 6, live [8,20),  QTY_CMP_PRI 10000, home $v0
 *     p224 target.vrx     refs 8, live [14,50), QTY_CMP_PRI  6666, home $t0
 * plus two pri-40000, ~2-insn field pseudos p234 [32,34) / p239 [36,38) that
 * take $v0 FIRST.  p224 lives to index 50 (its second store — see below) and
 * therefore CONFLICTS p234/p239, so it can never land in $v0 no matter how it
 * is reweighted: outranking pri-40000 needs ~32 refs (unreachable), and p224
 * cannot avoid the conflict without dying before index 32.  r.vx (p220) does
 * NOT conflict p234/p239 (disjoint) so it freely shares $v0.  To match, p224
 * must die early (like retail, where target.vrx's $v0 is recycled into
 * target.vry one insn later); ours keeps it live because the vrx computation
 * is stored TWICE to sp+28 (the vrx donor is a real second `target.vrx =`
 * assignment; jump-opt keeps both across the /4 bias branch — `rtldump
 * --pass rtl,jump`: fp+28 mem-refs 2 -> 5).  But that second store is exactly
 * the +4 bytes that make the frame 860: removing it (or any donor) drops to
 * 856 (measured — same as the historical `sin = CamLoc.vx` -4).  So the donor
 * is load-bearing for LENGTH and simultaneously the cause of the tie; no
 * length-neutral respelling found this round splits those two effects.
 *
 * Rejected length experiments (each changes the compiled LENGTH):
 *   - `sin = CamLoc.vx;` before the real computation: 856 bytes (-4).
 *   - `cos = r.vx;` before the real computation: 848 bytes (-12).
 *   - dropping the vrx/vpx donors + the tail fence entirely: 856 bytes (-4).
 *   - `target.vrx = CamLoc.vx; target.vrx -= r.vx;` two-step: 864 bytes (+4).
 *
 * THREE bounded foreground permuter runs at this checkpoint have plateaued at
 * 7 with the authoritative full-link rescore keeping base.c best — the third
 * (this round) was AFTER the documented permuter `-fno-builtin` fix and still
 * found nothing, so the plateau is not a stale-tooling artifact.  Plain +
 * guided autorules (type-width, empty-loop-boundary, loop-fence, nested-loop-
 * fence, redundant-field-donor, fence-unwrap, cmp-swap, seed variants) and
 * swapping the donor order all confirmed no win below 7.  Genuine sub-C local-
 * alloc interference tie: only a find_reg / jump-store-dedup change (not a
 * source change) can move it.  Do not re-open with more surgical sessions.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CameraDirection", CameraDirection);
#else

#include "item.h"

typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
    s32 rz;
    void *super;
} GsRVIEW2;

typedef struct
{
    VECTOR TargetVector;         /* 0x00 */
    Humanoid *Owner;             /* 0x10 */
    s32 Mode;                    /* 0x14 */
    s16 DirectionRX;             /* 0x18 */
    s16 DirectionRY;             /* 0x1A */
    u8 OldMode;                  /* 0x1C */
    u8 CriticalHit;              /* 0x1D */
} TCameraStatus;

extern TCameraStatus CamState;
extern GsRVIEW2 ViewInfo;

extern void GetPadXY(s16 no, s16 *x, s16 *y);
extern void SetCameraMode(s32 mode);
extern s32 rsin(s32 angle);
extern s32 rcos(s32 angle);
extern void RotateVectorS(SVECTOR *vec, s32 rx, s32 ry, s32 rz);
extern void FUN_80030644(VECTOR *pos, s32 amount);

void CameraDirection(Humanoid *pl, GsRVIEW2 *vDif)
{
    GsRVIEW2 target;
    ModelArchiveType *mad;
    SVECTOR r;
    VECTOR CamLoc;
    s32 sin;
    s32 cos;
    s32 y;
    s16 x;
    s16 z;

    mad = pl->model;
    GetPadXY(0, &x, &z);
    if (CamState.OldMode == 3) {
        x = x / 2;
        z = z / 2;
    } else if ((CamState.Owner->pad.data & 4) == 0) {
        SetCameraMode(0);
    }

    CamState.DirectionRX = CamState.DirectionRX - z;
    CamState.DirectionRY = CamState.DirectionRY + x;
    if (CamState.DirectionRX >= 0x38F) {
        CamState.DirectionRX = 0x38E;
    } else if (CamState.DirectionRX < -0x38E) {
        CamState.DirectionRX = -0x38E;
    }
    if (CamState.DirectionRY >= 0x401) {
        CamState.DirectionRY = 0x400;
    } else if (CamState.DirectionRY < -0x400) {
        CamState.DirectionRY = -0x400;
    }

    sin = rsin(mad->rotate.vy) / 6;
    cos = rcos(mad->rotate.vy) / 6;
    r.vx = 0;
    r.vy = 0;
    r.vz = 0x4B0;
    RotateVectorS(&r,
                  mad->rotate.vx + CamState.DirectionRX,
                  mad->rotate.vy + CamState.DirectionRY,
                  mad->rotate.vz);

    CamLoc.vx = mad->locate.coord.t[0];
    CamLoc.vy = mad->locate.coord.t[1] - 0x60E;
    CamLoc.vz = mad->locate.coord.t[2];
    FUN_80030644(&CamLoc, 1000);
    CamLoc.vx -= sin;
    /* A weight-free loop boundary preserves the retail load schedule. */
    do {
    } while (0);
    CamLoc.vz -= cos;
    y = r.vy;
    if (y > 0) {
        CamLoc.vy = CamLoc.vy - y;
    } else {
        if (y < 0) {
            y += 3;
        }
        CamLoc.vy = CamLoc.vy + (y >> 2);
    }

    /* Weighting fence (permuter-found, round 2): birthing the target/vDif
     * tail as one unit shifts local_alloc's quantity order enough to
     * resolve r.vy and r.vz's allocation (now exact) and shrink r.vx vs the
     * target.vrx temp from a 4-way rotation to a plain 2-way $v0/$t0 swap
     * (STATUS header has the residual detail). Narrower boundaries
     * (excluding the vDif-> stores) regressed the whole-function length by
     * 4-8 bytes, so the vDif tail must stay inside. */
    do {
    target.vrx = CamLoc.vx - r.vx;
    target.vpx = CamLoc.vx + r.vx;
    target.vry = CamLoc.vy - r.vy;
    target.vrz = CamLoc.vz - r.vz;
    target.vpy = CamLoc.vy + r.vy;
    target.vpz = CamLoc.vz + r.vz;
    /* Safe allocation donors: target is an unobserved automatic aggregate. */
    target.vpx = CamLoc.vx + r.vx;
    target.vrx = CamLoc.vx - r.vx;

    y = ViewInfo.vrx;
    vDif->vrx = (target.vrx - y) / 4;
    vDif->vry = (target.vry - ViewInfo.vry) / 4;
    vDif->vrz = (target.vrz - ViewInfo.vrz) / 4;
    vDif->vpx = (target.vpx - ViewInfo.vpx) / 8;
    vDif->vpy = (target.vpy - ViewInfo.vpy) / 8;
    vDif->vpz = (target.vpz - ViewInfo.vpz) / 8;
    } while (0);
}

#endif

// triage: MEDIUM — 215 insns, mul/div, 6 callees, ~0.05 to ProcItemTeleport
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void CameraDirection(Humanoid *pl,GsRVIEW2 *vDif)
//
// {
//   short sVar1;
//   int iVar2;
//   int iVar3;
//   uint uVar4;
//   int iVar5;
//   int iVar6;
//   ModelArchiveType *pMVar7;
//   SVECTOR local_38;
//   int local_30;
//   int local_2c;
//   int local_28;
//   ushort local_20;
//   ushort local_1e [3];
//
//   pMVar7 = pl->model;
//   GetPadXY(0,(short *)&local_20,(short *)local_1e);
//   if ((undefined1)CamState.OldMode == CMODE_SIGHT) {
//     local_20 = (ushort)(((int)((uint)local_20 << 0x10) >> 0x10) -
//                         ((int)((uint)local_20 << 0x10) >> 0x1f) >> 1);
//     local_1e[0] = (ushort)(((int)((uint)local_1e[0] << 0x10) >> 0x10) -
//                            ((int)((uint)local_1e[0] << 0x10) >> 0x1f) >> 1);
//   }
//   else if ((((CamState.Owner)->pad).data & 4) == 0) {
//     SetCameraMode(CMODE_NORMAL);
//   }
//   uVar4 = (uint)(ushort)CamState.DirectionRX;
//   CamState.DirectionRY = CamState.DirectionRY + local_20;
//   CamState.DirectionRX = (short)(uVar4 - local_1e[0]);
//   iVar5 = (int)((uVar4 - local_1e[0]) * 0x10000) >> 0x10;
//   if (iVar5 < 0x38f) {
//     sVar1 = -0x38e;
//     if (iVar5 < -0x38e) goto LAB_80031d68;
//   }
//   else {
//     sVar1 = 0x38e;
// LAB_80031d68:
//     CamState.DirectionRX = sVar1;
//   }
//   if (CamState.DirectionRY < 0x401) {
//     sVar1 = -0x400;
//     if (-0x401 < CamState.DirectionRY) goto LAB_80031d9c;
//   }
//   else {
//     sVar1 = 0x400;
//   }
//   CamState.DirectionRY = sVar1;
// LAB_80031d9c:
//   iVar5 = rsin((int)(pMVar7->rotate).vy);
//   iVar2 = rcos((int)(pMVar7->rotate).vy);
//   local_38.vz = 0x4b0;
//   local_38.vx = 0;
//   local_38.vy = 0;
//   RotateVectorS(&local_38,(int)(pMVar7->rotate).vx + (int)CamState.DirectionRX,
//                 (int)(pMVar7->rotate).vy + (int)CamState.DirectionRY,(int)(pMVar7->rotate).vz);
//   local_30 = (pMVar7->locate).coord.t[0];
//   local_2c = (pMVar7->locate).coord.t[1] + -0x60e;
//   local_28 = (pMVar7->locate).coord.t[2];
//   FUN_80030644(&local_30,1000);
//   iVar3 = local_30 - iVar5 / 6;
//   iVar5 = (int)local_38.vy;
//   iVar2 = local_28 - iVar2 / 6;
//   if (iVar5 < 1) {
//     if (iVar5 < 0) {
//       iVar5 = iVar5 + 3;
//     }
//     iVar5 = iVar5 >> 2;
//   }
//   else {
//     iVar5 = -iVar5;
//   }
//   iVar6 = (iVar3 - local_38.vx) - ViewInfo.vrx;
//   if (iVar6 < 0) {
//     iVar6 = iVar6 + 3;
//   }
//   vDif->vrx = iVar6 >> 2;
//   iVar6 = ((local_2c + iVar5) - (int)local_38.vy) - ViewInfo.vry;
//   if (iVar6 < 0) {
//     iVar6 = iVar6 + 3;
//   }
//   vDif->vry = iVar6 >> 2;
//   iVar6 = (iVar2 - local_38.vz) - ViewInfo.vrz;
//   if (iVar6 < 0) {
//     iVar6 = iVar6 + 3;
//   }
//   vDif->vrz = iVar6 >> 2;
//   iVar3 = (iVar3 + local_38.vx) - ViewInfo.vpx;
//   if (iVar3 < 0) {
//     iVar3 = iVar3 + 7;
//   }
//   vDif->vpx = iVar3 >> 3;
//   iVar5 = (local_2c + iVar5 + (int)local_38.vy) - ViewInfo.vpy;
//   if (iVar5 < 0) {
//     iVar5 = iVar5 + 7;
//   }
//   vDif->vpy = iVar5 >> 3;
//   iVar5 = (iVar2 + local_38.vz) - ViewInfo.vpz;
//   if (iVar5 < 0) {
//     iVar5 = iVar5 + 7;
//   }
//   vDif->vpz = iVar5 >> 3;
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? FUN_80030644(s32 *, ?);                           /* extern */
// ? GetPadXY(?, u16 *, u16 *);                        /* extern */
// ? RotateVectorS(s16 *, s32, s32, s16);              /* extern */
// ? SetCameraMode(?);                                 /* extern */
// s32 rcos(s16);                                      /* extern */
// s32 rsin(s16, u16, ? *);                            /* extern */
// extern ? CamState;
// extern ? ViewInfo;
//
// void CameraDirection(void *arg0, void *arg1) {
//     s32 sp10;
//     s32 sp14;
//     s32 sp18;
//     s32 sp1C;
//     s32 sp20;
//     s32 sp24;
//     s16 sp30;
//     s16 sp32;
//     s16 sp34;
//     s32 sp38;
//     s32 sp3C;
//     s32 sp40;
//     u16 sp48;
//     u16 sp4A;
//     s16 var_v1;
//     s32 temp_s0;
//     s32 temp_s1;
//     s32 temp_v0;
//     s32 var_t1;
//     s32 var_v0_3;
//     s32 var_v0_4;
//     s32 var_v0_5;
//     s32 var_v0_6;
//     s32 var_v0_7;
//     s32 var_v0_8;
//     u16 temp_v1;
//     u16 var_v0;
//     u16 var_v0_2;
//     void *temp_s3;
//
//     temp_s3 = arg0->unk58;
//     GetPadXY(0, &sp48, &sp4A);
//     if (CamState.unk1C == 3) {
//         sp48 = (u16) ((s16) sp48 / 2);
//         sp4A = (u16) ((s16) sp4A / 2);
//     } else if (!(CamState.unk10->unk10 & 4)) {
//         SetCameraMode(0);
//     }
//     temp_v1 = CamState.unk18 - sp4A;
//     CamState.unk18 = temp_v1;
//     CamState.unk1A = (u16) (CamState.unk1A + sp48);
//     if ((s16) temp_v1 >= 0x38F) {
//         var_v0 = 0x38E;
//         goto block_8;
//     }
//     var_v0 = -0x38EU;
//     if ((s16) temp_v1 < -0x38E) {
// block_8:
//         CamState.unk18 = var_v0;
//     }
//     if ((s16) CamState.unk1A >= 0x401) {
//         var_v0_2 = 0x400;
//         goto block_12;
//     }
//     var_v0_2 = -0x400U;
//     if ((s16) CamState.unk1A < -0x400) {
// block_12:
//         CamState.unk1A = var_v0_2;
//     }
//     temp_s1 = rsin(temp_s3->unk52, sp48, &CamState) / 6;
//     temp_s0 = rcos(temp_s3->unk52);
//     sp34 = 0x4B0;
//     sp30 = 0;
//     sp32 = 0;
//     RotateVectorS(&sp30, temp_s3->unk50 + (s16) CamState.unk18, temp_s3->unk52 + (s16) CamState.unk1A, temp_s3->unk54);
//     sp38 = temp_s3->unk18;
//     sp3C = temp_s3->unk1C - 0x60E;
//     sp40 = temp_s3->unk20;
//     FUN_80030644(&sp38, 0x3E8);
//     sp38 -= temp_s1;
//     var_v1 = sp32;
//     sp40 -= temp_s0 / 6;
//     if (var_v1 > 0) {
//         var_v0_3 = sp3C - var_v1;
//     } else {
//         if (var_v1 < 0) {
//             var_v1 += 3;
//         }
//         var_v0_3 = sp3C + (var_v1 >> 2);
//     }
//     sp3C = var_v0_3;
//     temp_v0 = sp38 - sp30;
//     var_t1 = temp_v0 - ViewInfo.unkC;
//     sp1C = temp_v0;
//     sp10 = sp38 + sp30;
//     sp20 = sp3C - sp32;
//     sp24 = sp40 - sp34;
//     sp14 = sp3C + sp32;
//     sp18 = sp40 + sp34;
//     if (var_t1 < 0) {
//         var_t1 += 3;
//     }
//     arg1->unkC = (s32) (var_t1 >> 2);
//     var_v0_4 = sp20 - ViewInfo.unk10;
//     if (var_v0_4 < 0) {
//         var_v0_4 += 3;
//     }
//     arg1->unk10 = (s32) (var_v0_4 >> 2);
//     var_v0_5 = sp24 - ViewInfo.unk14;
//     if (var_v0_5 < 0) {
//         var_v0_5 += 3;
//     }
//     arg1->unk14 = (s32) (var_v0_5 >> 2);
//     var_v0_6 = sp10 - ViewInfo.unk0;
//     if (var_v0_6 < 0) {
//         var_v0_6 += 7;
//     }
//     arg1->unk0 = (s32) (var_v0_6 >> 3);
//     var_v0_7 = sp14 - ViewInfo.unk4;
//     if (var_v0_7 < 0) {
//         var_v0_7 += 7;
//     }
//     arg1->unk4 = (s32) (var_v0_7 >> 3);
//     var_v0_8 = sp18 - ViewInfo.unk8;
//     if (var_v0_8 < 0) {
//         var_v0_8 += 7;
//     }
//     arg1->unk8 = (s32) (var_v0_8 >> 3);
// }
