#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * MATCH.
 *
 * Updates and draws the flattened model used while the humanoid is dying.
 * The timer increment belongs in the non-attribute fallthrough after the
 * returning branch: cc1 then moves it into that branch's delay slot and keeps
 * the following store/clamp sequence in the target order.  A separate timer,
 * signed scale intermediate, projection depth, and full-width model height
 * also preserve the target's register lifetimes and signed `lh` load.
 */

extern short RefrectVector[16];
extern SVECTOR UnitVector;
extern GsOT *OTablePt;
extern s32 DrawTMDmode;
extern ModelType *LOCAL_COORDINATES_;

extern void FUN_80037e0c(Humanoid *human, s32 mode);
extern void DrawTMD(GsDOBJ2 *object, GsOT *ot, s32 mode);

void death_camera_something_(Humanoid *human)
{
    VECTOR scale;
    MATRIX matrix;
    SVECTOR screen;
    s32 p;
    s32 flag;
    VECTOR *position;
    s32 *chase;
    s32 timer;
    s32 height;
    s32 scaled;
    s32 depth;

    chase = &human->chase[0];
    if (human->motion->loop >= 0 || (human->map.attrib & 0xc) != 0 ||
        human->chase[0] < 0)
    {
        *chase = 0;
        return;
    }

    if ((human->map.attrib & 0x100) != 0)
    {
        if ((GameClock & 0xf) == 0)
        {
            FUN_80037e0c(human, 0);
        }
        return;
    }

    timer = human->chase[0] + 0x88;
    human->chase[0] = timer;
    if (0x1000 < timer)
    {
        human->chase[0] = 0x1000;
    }

    position = GetAbsolutePosition(human->model->object[0], 0, 0, 0);
    height = human->model->rotate.pad;
    position->vy = human->model->locate.coord.t[1];
    LOCAL_COORDINATES_->locate.coord.t[0] = position->vx;
    LOCAL_COORDINATES_->locate.coord.t[1] = position->vy;
    LOCAL_COORDINATES_->locate.coord.t[2] = position->vz;

    scaled = human->chase[0] * -height;
    if (scaled < 0)
    {
        scaled += 0x3ff;
    }
    scale.vx = scale.vy = scale.vz =
        (scaled >> 10) - (human->map.height >> 1);

    if (*((u8 *)human + 0x2f) != 0)
    {
        LOCAL_COORDINATES_->rotate.vx = 0x100;
        LOCAL_COORDINATES_->rotate.vy = RefrectVector[*((u8 *)human + 0x2f)];
        LOCAL_COORDINATES_->rotate.vz = 0;
    }
    else
    {
        LOCAL_COORDINATES_->rotate.vx = 0;
        LOCAL_COORDINATES_->rotate.vy = 0;
        LOCAL_COORDINATES_->rotate.vz = 0;
    }

    RotMatrixYXZ(&LOCAL_COORDINATES_->rotate,
                 &LOCAL_COORDINATES_->locate.coord);
    ScaleMatrix(&LOCAL_COORDINATES_->locate.coord, &scale);
    LOCAL_COORDINATES_->locate.flg = 0;
    GsGetLs(&LOCAL_COORDINATES_->locate, &matrix);
    GsSetLsMatrix(&matrix);
    depth = RotTransPers(&UnitVector, (s32 *)&screen, &p, &flag);
    screen.vz = depth;
    if ((depth << 16) >> 18 < 0x4e2)
    {
        DrawTMDmode = 0x20;
        DrawTMD(&LOCAL_COORDINATES_->object, OTablePt, 0);
    }
}

// triage: MEDIUM — 130 insns, mul/div, 8 callees, ~0.06 to ProcItemGoshikimai
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
//
// void FUN_8003818c(int param_1)
//
// {
//   short sVar1;
//   GsCOORDINATE2 *pGVar2;
//   int iVar3;
//   VECTOR *pVVar4;
//   long lVar5;
//   VECTOR local_48;
//   MATRIX MStack_38;
//   long lStack_18;
//   undefined2 local_14;
//   long lStack_10;
//   long lStack_c;
//
//   if ((*(short *)(*(int *)(param_1 + 0x5c) + 4) < 0) && ((*(ushort *)(param_1 + 0x28) & 0xc) == 0))
//   {
//     if (-1 < *(int *)(param_1 + 0x80)) {
//       iVar3 = *(int *)(param_1 + 0x80) + 0x88;
//       if ((*(ushort *)(param_1 + 0x28) & 0x100) != 0) {
//         if ((GameClock & 0xfU) != 0) {
//           return;
//         }
//         FUN_80037e0c(param_1,0);
//         return;
//       }
//       *(int *)(param_1 + 0x80) = iVar3;
//       if (0x1000 < iVar3) {
//         *(undefined4 *)(param_1 + 0x80) = 0x1000;
//       }
//       pVVar4 = GetAbsolutePosition((ModelType *)**(undefined4 **)(*(int *)(param_1 + 0x58) + 0x68),0
//                                    ,0,0);
//       pGVar2 = DAT_80097f38;
//       sVar1 = *(short *)(*(int *)(param_1 + 0x58) + 0x56);
//       pVVar4->vy = *(long *)(*(int *)(param_1 + 0x58) + 0x1c);
//       (pGVar2->coord).t[0] = pVVar4->vx;
//       (pGVar2->coord).t[1] = pVVar4->vy;
//       (pGVar2->coord).t[2] = pVVar4->vz;
//       iVar3 = *(int *)(param_1 + 0x80) * -(int)sVar1;
//       if (iVar3 < 0) {
//         iVar3 = iVar3 + 0x3ff;
//       }
//       local_48.vx = (iVar3 >> 10) - (*(int *)(param_1 + 0x24) >> 1);
//       if (*(char *)(param_1 + 0x2f) == '\0') {
//         *(undefined2 *)&pGVar2[1].flg = 0;
//         *(undefined2 *)((int)&pGVar2[1].flg + 2) = 0;
//         pGVar2[1].coord.m[0][0] = 0;
//       }
//       else {
//         *(undefined2 *)&pGVar2[1].flg = 0x100;
//         sVar1 = RefrectVector[*(byte *)(param_1 + 0x2f)];
//         pGVar2[1].coord.m[0][0] = 0;
//         *(short *)((int)&pGVar2[1].flg + 2) = sVar1;
//       }
//       local_48.vy = local_48.vx;
//       local_48.vz = local_48.vx;
//       RotMatrixYXZ((SVECTOR *)(DAT_80097f38 + 1),&DAT_80097f38->coord);
//       ScaleMatrix(&DAT_80097f38->coord,&local_48);
//       pGVar2 = DAT_80097f38;
//       DAT_80097f38->flg = 0;
//       GsGetLs(pGVar2,&MStack_38);
//       GsSetLsMatrix(&MStack_38);
//       lVar5 = RotTransPers(&UnitVector,&lStack_18,&lStack_10,&lStack_c);
//       local_14 = (undefined2)lVar5;
//       if (0x4e1 < (lVar5 << 0x10) >> 0x12) {
//         return;
//       }
//       _DrawTMDmode = 0x20;
//       DrawTMD(DAT_80097f38[1].coord.m[2] + 2,OTablePt,0);
//       return;
//     }
//   }
//   *(undefined4 *)(param_1 + 0x80) = 0;
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? DrawTMD(void *, s32, ?);                          /* extern */
// ? FUN_80037e0c(void *, ?);                          /* extern */
// void *GetAbsolutePosition(s32, ?, ?, ?);            /* extern */
// ? GsGetLs(void *, ? *);                             /* extern */
// ? GsSetLsMatrix(? *);                               /* extern */
// ? RotMatrixYXZ(void *, void *, void *);             /* extern */
// s16 RotTransPers(? *, ? *, ? *, ? *);               /* extern */
// ? ScaleMatrix(void *, s32 *);                       /* extern */
// extern s32 DrawTMDmode;
// extern s32 GameClock;
// extern void *LOCAL_COORDINATES_;
// extern s32 OTablePt;
// extern ? RefrectVector;
// extern ? UnitVector;
//
// void death_camera_something_(void *arg0) {
//     s32 sp10;
//     s32 sp14;
//     s32 sp18;
//     ? sp20;
//     ? sp40;
//     s16 sp44;
//     ? sp48;
//     ? sp4C;
//     s16 temp_v0_3;
//     s32 temp_v0;
//     s32 temp_v1;
//     s32 temp_v1_3;
//     s32 var_v1;
//     u16 temp_a0;
//     void *temp_v0_2;
//     void *temp_v1_2;
//
//     if ((arg0->unk5C->unk4 >= 0) || (temp_a0 = arg0->unk28, ((temp_a0 & 0xC) != 0)) || (temp_v1 = arg0->unk80, (temp_v1 < 0))) {
//         arg0->unk80 = 0;
//         return;
//     }
//     temp_v0 = temp_v1 + 0x88;
//     if (temp_a0 & 0x100) {
//         if (!(GameClock & 0xF)) {
//             FUN_80037e0c(arg0, 0);
//         }
//     } else {
//         arg0->unk80 = temp_v0;
//         if (temp_v0 >= 0x1001) {
//             arg0->unk80 = 0x1000;
//         }
//         temp_v0_2 = GetAbsolutePosition(*arg0->unk58->unk68, 0, 0, 0);
//         temp_v1_2 = arg0->unk58;
//         temp_v0_2->unk4 = (s32) temp_v1_2->unk1C;
//         LOCAL_COORDINATES_->unk18 = (s32) temp_v0_2->unk0;
//         LOCAL_COORDINATES_->unk1C = (s32) temp_v0_2->unk4;
//         LOCAL_COORDINATES_->unk20 = (s32) temp_v0_2->unk8;
//         var_v1 = arg0->unk80 * -temp_v1_2->unk56;
//         if (var_v1 < 0) {
//             var_v1 += 0x3FF;
//         }
//         temp_v1_3 = (var_v1 >> 0xA) - ((s32) arg0->unk24 >> 1);
//         sp18 = temp_v1_3;
//         sp14 = temp_v1_3;
//         sp10 = temp_v1_3;
//         if (arg0->unk2F != 0) {
//             LOCAL_COORDINATES_->unk50 = 0x100;
//             LOCAL_COORDINATES_->unk54 = 0;
//             LOCAL_COORDINATES_->unk52 = (u16) *((arg0->unk2F * 2) + &RefrectVector);
//         } else {
//             LOCAL_COORDINATES_->unk50 = 0;
//             LOCAL_COORDINATES_->unk52 = 0U;
//             LOCAL_COORDINATES_->unk54 = 0;
//         }
//         RotMatrixYXZ(LOCAL_COORDINATES_ + 0x50, LOCAL_COORDINATES_ + 4, LOCAL_COORDINATES_);
//         ScaleMatrix(LOCAL_COORDINATES_ + 4, &sp10);
//         LOCAL_COORDINATES_->unk0 = 0;
//         GsGetLs(LOCAL_COORDINATES_, &sp20);
//         GsSetLsMatrix(&sp20);
//         temp_v0_3 = RotTransPers(&UnitVector, &sp40, &sp48, &sp4C);
//         sp44 = temp_v0_3;
//         if (((s32) (temp_v0_3 << 0x10) >> 0x12) < 0x4E2) {
//             DrawTMDmode = 0x20;
//             DrawTMD(LOCAL_COORDINATES_ + 0x64, OTablePt, 0);
//         }
//     }
// }
