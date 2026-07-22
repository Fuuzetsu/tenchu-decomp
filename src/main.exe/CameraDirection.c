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

#include "item.h"
#include "padcmd.h"

extern void FUN_80030644(VECTOR *pos, s32 amount);

void CameraDirection(Humanoid *pl, GsRVIEW2 *vDif)
{
    GsRVIEW2 target;
    ModelArchiveType *mad;
    SVECTOR r;
    VECTOR CamLoc;
    s32 rx;
    s32 ry;
    s16 x;
    s16 y;

    mad = pl->model;
    GetPadXY(0, &x, &y);
    if (CamState.OldMode == CMODE_SIGHT) {
        x = x / 2;
        y = y / 2;
    } else if ((CamState.Owner->pad.data & 4) == 0) {
        SetCameraMode(CMODE_NORMAL);
    }

    CamState.DirectionRX = CamState.DirectionRX - y;
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

    rx = rsin(mad->rotate.vy) / 6;
    ry = rcos(mad->rotate.vy) / 6;
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
    CamLoc.vx -= rx;
    CamLoc.vz -= ry;
    if (r.vy > 0) {
        CamLoc.vy -= r.vy;
    } else {
        CamLoc.vy += r.vy / 4;
    }

    target.vrx = CamLoc.vx - r.vx;
    target.vry = CamLoc.vy - r.vy;
    target.vrz = CamLoc.vz - r.vz;
    target.vpy = CamLoc.vy + r.vy;
    target.vpz = CamLoc.vz + r.vz;
    target.vpx = CamLoc.vx + r.vx;

    vDif->vrx = (target.vrx - ViewInfo.vrx) / 4;
    vDif->vry = (target.vry - ViewInfo.vry) / 4;
    vDif->vrz = (target.vrz - ViewInfo.vrz) / 4;
    vDif->vpx = (target.vpx - ViewInfo.vpx) / 8;
    vDif->vpy = (target.vpy - ViewInfo.vpy) / 8;
    vDif->vpz = (target.vpz - ViewInfo.vpz) / 8;
}

// triage: MEDIUM — 215 insns, mul/div, 6 callees, ~0.05 to ProcItemTeleport
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference):
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
