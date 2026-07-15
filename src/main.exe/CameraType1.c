#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CameraType1(struct Humanoid *pl, struct GsRVIEW2 *vDif);
 *     CAMERA.C:603, 176 src lines, frame 200 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $s5       struct Humanoid * pl
 *     param $s3       struct GsRVIEW2 * vDif
 *     reg   $s2       struct ModelArchiveType * mad
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vecl
 *     stack sp+48     struct SVECTOR vecr
 *     reg   $s0       int levfl
 *     reg   $s1       int levfr
 *     reg   $v0       int levbl
 *     reg   $s2       int levbr
 *     reg   $s0       int levmap
 *     stack sp+56     struct SVECTOR campos
 *     stack sp+64     struct SVECTOR ref
 *     stack sp+72     struct SVECTOR campos
 *     stack sp+80     struct SVECTOR ref
 *     stack sp+88     struct SVECTOR campos
 *     stack sp+96     struct SVECTOR ref
 *     stack sp+104    struct SVECTOR campos
 *     stack sp+112    struct SVECTOR ref
 *     stack sp+120    struct SVECTOR campos
 *     stack sp+128    struct SVECTOR ref
 *     stack sp+136    struct SVECTOR campos
 *     stack sp+144    struct SVECTOR ref
 *     stack sp+152    struct SVECTOR campos
 *     stack sp+160    struct SVECTOR ref
 *     stack sp+56     struct SVECTOR campos
 *     stack sp+64     struct SVECTOR ref
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

/*
 * CameraType1 (0x80030c74) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=CameraType1
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", CameraType1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__caseD_9);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__caseD_8);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__caseD_d);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80030d60__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_9);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_b);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_8);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_10);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_11);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CameraType1", switchD_80031048__caseD_5);

/* jump-table pool @ 0x80011be0 (28 words; tables at 0x80011be0, 0x80011c18) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 CameraType1_jtbl[28] = {
    0x80030F4C, 0x80031018, 0x80031018, 0x80030F6C,
    0x80031018, 0x80030F9C, 0x80031018, 0x80030F5C,
    0x80030F3C, 0x80030D68, 0x80031018, 0x80031018,
    0x80031018, 0x80030FCC, 0x80031050, 0x80031674,
    0x8003107C, 0x80031110, 0x800313F4, 0x800311A4,
    0x80031238, 0x800312CC, 0x80031360, 0x80031674,
    0x80031674, 0x80031674, 0x80031488, 0x800315C8,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// /* WARNING: Type propagation algorithm not settling */
// 
// void CameraType1(Humanoid *pl,GsRVIEW2 *vDif)
// 
// {
//   ushort uVar1;
//   TCameraMode TVar2;
//   long lVar3;
//   long lVar4;
//   long lVar5;
//   long lVar6;
//   PersistentState *pPVar7;
//   SVECTOR *pSVar8;
//   byte bVar9;
//   ModelArchiveType *pMVar10;
//   VECTOR local_80;
//   SVECTOR local_70;
//   SVECTOR local_68;
//   SVECTOR local_60;
//   undefined4 local_58;
//   undefined4 local_54;
//   undefined4 local_50;
//   undefined4 local_4c;
//   undefined4 local_48;
//   undefined4 local_44;
//   SVECTOR local_40;
//   undefined4 local_38;
//   undefined4 local_34;
//   undefined4 local_30;
//   undefined4 local_2c;
//   undefined4 local_28;
//   undefined4 local_24;
//   
//   pMVar10 = pl->model;
//   memset((uchar *)&local_70,'\0',0x10);
//   local_80.vx = (pl->model->locate).coord.t[0];
//   local_80.vy = (pl->model->locate).coord.t[1] + -0x60e;
//   local_80.vz = (pl->model->locate).coord.t[2];
//   local_80.pad._0_2_ = local_68.vz;
//   local_80.pad._2_2_ = local_68.pad;
//   pMVar10->attribute = pMVar10->attribute | 2;
//   TVar2 = CamState.Mode;
//   local_70._0_4_ = local_80.vx;
//   local_70._4_4_ = local_80.vy;
//   local_68._0_4_ = local_80.vz;
//   switch((int)(((ushort)(CamState.Owner)->status - 3) * 0x10000) >> 0x10) {
//   case 0:
//     CamState.Mode = CMODE_SWIM;
//     TVar2 = CamState.Mode;
//     break;
//   case 3:
//     if ((CamState.Owner)->motion->mid == 0x600) {
//       TVar2 = CMODE_RUN;
//       break;
//     }
//     goto LAB_8003101c;
//   case 5:
//     if ((CamState.Owner)->motion->mid != 0x801) goto LAB_8003101c;
//     TVar2 = CMODE_RUN;
//     break;
//   case 7:
//     CamState.Mode = 0x11;
//     TVar2 = CamState.Mode;
//     break;
//   case 8:
//     CamState.Mode = CMODE_CROUCH;
//     TVar2 = CamState.Mode;
//     break;
//   case 9:
//     local_70.vx = (undefined2)DAT_80097a28;
//     local_70.vy = DAT_80097a28._2_2_;
//     local_70.vz = (undefined2)DAT_80097a2c;
//     local_70.pad = DAT_80097a2c._2_2_;
//     local_68.vx = (undefined2)DAT_80097a30;
//     local_68.vy = DAT_80097a30._2_2_;
//     local_68.vz = (undefined2)DAT_80097a34;
//     local_68.pad = DAT_80097a34._2_2_;
//     RotateVectorS(&local_70,(int)(pMVar10->rotate).vx,(int)(pMVar10->rotate).vy,0);
//     RotateVectorS(&local_68,(int)(pMVar10->rotate).vx,(int)(pMVar10->rotate).vy,0);
//     lVar3 = GetAreaMapLevel(GlobalAreaMap,(pMVar10->locate).coord.t[0] + (int)local_70.vx,
//                             (pMVar10->locate).coord.t[1]);
//     lVar4 = GetAreaMapLevel(GlobalAreaMap,(pMVar10->locate).coord.t[0] + (int)local_68.vx,
//                             (pMVar10->locate).coord.t[1]);
//     lVar5 = GetAreaMapLevel(GlobalAreaMap,(pMVar10->locate).coord.t[0] - (int)local_70.vx,
//                             (pMVar10->locate).coord.t[1]);
//     lVar6 = GetAreaMapLevel(GlobalAreaMap,(pMVar10->locate).coord.t[0] - (int)local_68.vx,
//                             (pMVar10->locate).coord.t[1]);
//     bVar9 = lVar3 == -0x80000000;
//     if (lVar4 == -0x80000000) {
//       bVar9 = bVar9 | 2;
//     }
//     if (lVar6 == -0x80000000) {
//       bVar9 = bVar9 | 4;
//     }
//     if (lVar5 == -0x80000000) {
//       bVar9 = bVar9 | 8;
//     }
//     if (bVar9 == 4) {
//       CamState.Mode = CMODE_PEEP_R;
//       TVar2 = CamState.Mode;
//     }
//     else if (bVar9 == 8) {
//       CamState.Mode = CMODE_PEEP_L;
//       TVar2 = CamState.Mode;
//     }
//     else if ((bVar9 & 3) == 1) {
//       CamState.Mode = CMODE_STICK_R;
//       TVar2 = CamState.Mode;
//     }
//     else if ((bVar9 & 3) == 2) {
//       CamState.Mode = CMODE_STICK_L;
//       TVar2 = CamState.Mode;
//     }
//     else {
//       CamState.Mode = CMODE_NORMAL;
//       TVar2 = CamState.Mode;
//     }
//     break;
//   case 0xd:
//     uVar1 = (CamState.Owner)->motion->mid;
//     TVar2 = 0x10;
//     if (4 < uVar1 - 0x1005) {
//       if (uVar1 != 0x100c) goto LAB_8003101c;
//       TVar2 = 0x10;
//     }
//   }
//   CamState.Mode = TVar2;
// LAB_8003101c:
//   switch(CamState.Mode) {
//   case CMODE_CRITICAL_HIT:
//     pMVar10 = pl->model;
//     pSVar8 = (SVECTOR *)(&DAT_80089f50 + (uint)(byte)(undefined1)CamState.OldMode * 0x20);
//     goto LAB_8003168c;
//   default:
//                     /* globals are CamY and CamZ in PSX.EXE? */
//     pSVar8 = (SVECTOR *)&DAT_80089f30;
//     break;
//   case CMODE_STICK_L:
//     local_60.vx = (short)DAT_80011a64;
//     local_60.vy = DAT_80011a64._2_2_;
//     local_60.vz = (short)DAT_80011a68;
//     local_60.pad = DAT_80011a68._2_2_;
//     local_58 = DAT_80011a6c;
//     local_54 = DAT_80011a70;
//     local_50 = DAT_80011a74;
//     local_4c = DAT_80011a78;
//     local_48 = DAT_80011a7c;
//     local_44 = DAT_80011a80;
//     pSVar8 = &local_60;
//     break;
//   case CMODE_STICK_R:
//     local_70.vx = (undefined2)DAT_80011a84;
//     local_70.vy = DAT_80011a84._2_2_;
//     local_70.vz = (undefined2)DAT_80011a88;
//     local_70.pad = DAT_80011a88._2_2_;
//     local_68.vx = (undefined2)DAT_80011a8c;
//     local_68.vy = DAT_80011a8c._2_2_;
//     local_68.vz = (undefined2)DAT_80011a90;
//     local_68.pad = DAT_80011a90._2_2_;
//     local_60.vx = (short)DAT_80011a94;
//     local_60.vy = DAT_80011a94._2_2_;
//     local_60.vz = (short)DAT_80011a98;
//     local_60.pad = DAT_80011a98._2_2_;
//     local_58 = DAT_80011a9c;
//     local_54 = DAT_80011aa0;
//     pSVar8 = &local_70;
//     break;
//   case CMODE_SWIM:
//     local_70.vx = (short)DAT_80011b24;
//     local_70.vy = DAT_80011b24._2_2_;
//     local_70.vz = (short)DAT_80011b28;
//     local_70.pad = DAT_80011b28._2_2_;
//     local_68.vx = (short)DAT_80011b2c;
//     local_68.vy = DAT_80011b2c._2_2_;
//     local_68.vz = (short)DAT_80011b30;
//     local_68.pad = DAT_80011b30._2_2_;
//     local_60.vx = (short)DAT_80011b34;
//     local_60.vy = DAT_80011b34._2_2_;
//     local_60.vz = (short)DAT_80011b38;
//     local_60.pad = DAT_80011b38._2_2_;
//     local_58 = DAT_80011b3c;
//     local_54 = DAT_80011b40;
//     pSVar8 = &local_70;
//     goto LAB_80031658;
//   case CMODE_PEEP_L:
//     local_70.vx = (undefined2)DAT_80011aa4;
//     local_70.vy = DAT_80011aa4._2_2_;
//     local_70.vz = (undefined2)DAT_80011aa8;
//     local_70.pad = DAT_80011aa8._2_2_;
//     local_68.vx = (undefined2)DAT_80011aac;
//     local_68.vy = DAT_80011aac._2_2_;
//     local_68.vz = (undefined2)DAT_80011ab0;
//     local_68.pad = DAT_80011ab0._2_2_;
//     local_60.vx = (short)DAT_80011ab4;
//     local_60.vy = DAT_80011ab4._2_2_;
//     local_60.vz = (short)DAT_80011ab8;
//     local_60.pad = DAT_80011ab8._2_2_;
//     local_58 = DAT_80011abc;
//     local_54 = DAT_80011ac0;
//     pSVar8 = &local_70;
//     break;
//   case CMODE_PEEP_R:
//     local_70.vx = (undefined2)DAT_80011ac4;
//     local_70.vy = DAT_80011ac4._2_2_;
//     local_70.vz = (undefined2)DAT_80011ac8;
//     local_70.pad = DAT_80011ac8._2_2_;
//     local_68.vx = (undefined2)DAT_80011acc;
//     local_68.vy = DAT_80011acc._2_2_;
//     local_68.vz = (undefined2)DAT_80011ad0;
//     local_68.pad = DAT_80011ad0._2_2_;
//     local_60.vx = (short)DAT_80011ad4;
//     local_60.vy = DAT_80011ad4._2_2_;
//     local_60.vz = (short)DAT_80011ad8;
//     local_60.pad = DAT_80011ad8._2_2_;
//     local_58 = DAT_80011adc;
//     local_54 = DAT_80011ae0;
//     pSVar8 = &local_70;
//     break;
//   case CMODE_CROUCH:
//     local_70.vx = (short)DAT_80011ae4;
//     local_70.vy = DAT_80011ae4._2_2_;
//     local_70.vz = (short)DAT_80011ae8;
//     local_70.pad = DAT_80011ae8._2_2_;
//     local_68.vx = (short)DAT_80011aec;
//     local_68.vy = DAT_80011aec._2_2_;
//     local_68.vz = (short)DAT_80011af0;
//     local_68.pad = DAT_80011af0._2_2_;
//     local_60.vx = (short)DAT_80011af4;
//     local_60.vy = DAT_80011af4._2_2_;
//     local_60.vz = (short)DAT_80011af8;
//     local_60.pad = DAT_80011af8._2_2_;
//     local_58 = DAT_80011afc;
//     local_54 = DAT_80011b00;
//     pSVar8 = &local_70;
//     goto LAB_80031658;
//   case CMODE_RUN:
//     local_70.vx = (short)DAT_80011b04;
//     local_70.vy = DAT_80011b04._2_2_;
//     local_70.vz = (short)DAT_80011b08;
//     local_70.pad = DAT_80011b08._2_2_;
//     local_68.vx = (short)DAT_80011b0c;
//     local_68.vy = DAT_80011b0c._2_2_;
//     local_68.vz = (short)DAT_80011b10;
//     local_68.pad = DAT_80011b10._2_2_;
//     local_60.vx = (short)DAT_80011b14;
//     local_60.vy = DAT_80011b14._2_2_;
//     local_60.vz = (short)DAT_80011b18;
//     local_60.pad = DAT_80011b18._2_2_;
//     local_58 = DAT_80011b1c;
//     local_54 = DAT_80011b20;
//     pSVar8 = &local_70;
//     goto LAB_80031658;
//   case 0x10:
//     local_70.vx = (short)DAT_80011b44;
//     local_70.vy = DAT_80011b44._2_2_;
//     local_70.vz = (short)DAT_80011b48;
//     local_70.pad = DAT_80011b48._2_2_;
//     local_68.vx = (short)DAT_80011b4c;
//     local_68.vy = DAT_80011b4c._2_2_;
//     local_68.vz = (short)DAT_80011b50;
//     local_68.pad = DAT_80011b50._2_2_;
//     local_60.vx = (short)DAT_80011b54;
//     local_60.vy = DAT_80011b54._2_2_;
//     local_60.vz = (short)DAT_80011b58;
//     local_60.pad = DAT_80011b58._2_2_;
//     local_58 = DAT_80011b5c;
//     local_54 = DAT_80011b60;
//     pPVar7 = &PersistentState;
//     local_40.vx = (short)DAT_80011b64;
//     local_40.vy = DAT_80011b64._2_2_;
//     local_40.vz = (short)DAT_80011b68;
//     local_40.pad = DAT_80011b68._2_2_;
//     local_38 = DAT_80011b6c;
//     local_34 = DAT_80011b70;
//     local_30 = DAT_80011b74;
//     local_2c = DAT_80011b78;
//     local_28 = DAT_80011b7c;
//     local_24 = DAT_80011b80;
//     MakeCameraPosition(&local_80,&pl->model->rotate,&local_70,(SVECTOR *)vDif);
//     pSVar8 = &local_40;
//     if (0x800 < (int)pPVar7) {
//       CamState.Mode = CMODE_NORMAL;
//       return;
//     }
//     goto LAB_80031658;
//   case 0x11:
//     pSVar8 = &local_70;
//     local_70.vx = (short)DAT_80011b84;
//     local_70.vy = DAT_80011b84._2_2_;
//     local_70.vz = (short)DAT_80011b88;
//     local_70.pad = DAT_80011b88._2_2_;
//     local_68.vx = (short)DAT_80011b8c;
//     local_68.vy = DAT_80011b8c._2_2_;
//     local_68.vz = (short)DAT_80011b90;
//     local_68.pad = DAT_80011b90._2_2_;
//     local_60.vx = (short)DAT_80011b94;
//     local_60.vy = DAT_80011b94._2_2_;
//     local_60.vz = (short)DAT_80011b98;
//     local_60.pad = DAT_80011b98._2_2_;
//     local_58 = DAT_80011b9c;
//     local_54 = DAT_80011ba0;
// LAB_80031658:
//     MakeCameraPosition(&local_80,&pl->model->rotate,pSVar8,(SVECTOR *)vDif);
//     CamState.Mode = CMODE_NORMAL;
//     return;
//   }
//   pMVar10 = pl->model;
// LAB_8003168c:
//   MakeCameraPosition(&local_80,&pMVar10->rotate,pSVar8,(SVECTOR *)vDif);
//   return;
// }

#endif /* NON_MATCHING */
