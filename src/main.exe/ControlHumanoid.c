#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ControlHumanoid(struct Humanoid *human);
 *     HUMAN.C:107, 44 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s0       struct Humanoid * human
 *     reg   $s1       struct ModelArchiveType * model
 *     reg   $s2       long m
 *     stack sp+24     struct MATRIX mat
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern struct Humanoid *StagePlayer;
 *     extern short ActionHalt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ControlHumanoid", ControlHumanoid);

// triage: HARD — 394 insns, mul/div, 13 callees, ~0.05 to ProcItemDrop
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
// void ControlHumanoid(Humanoid *human)
//
// {
//   short sVar1;
//   MotionManager *pMVar2;
//   VECTOR *pVVar3;
//   ModelType **ppMVar4;
//   short *psVar5;
//   int iVar6;
//   long lVar7;
//   long lVar8;
//   long lVar9;
//   int iVar10;
//   ModelType *pMVar11;
//   MATRIX MStack_30;
//
//   pMVar11 = (ModelType *)human->model;
//   iVar10 = 1;
//   if (*(short *)(((pMVar11->object).coord2)->flg + 0x58) < 0) {
//     if (human->status == 0x11) {
//       FUN_8002bcb8(human);
//       FUN_8003818c(human);
//     }
//   }
//   else {
//     DefaultActionHumanoid(human);
//     StateTransition(human);
//     DrawShadow(human);
//   }
//   HumanActionControl(human);
//   if ((SystemFlag & 2) == 0) {
// LAB_80027c6c:
//     if (SkipFrame != 0) goto LAB_80027c80;
//     if (human != StagePlayer) {
//       GsGetLs((GsCOORDINATE2 *)pMVar11,&MStack_30);
//       GsSetLsMatrix(&MStack_30);
//       lVar7 = DrawClip(pMVar11,(long *)0x0);
//       iVar10 = 0;
//       if (-1 < lVar7) {
//         iVar10 = -1;
//       }
//     }
//   }
//   else {
//     if (SkipFrame == 0) {
//       if (human == StagePlayer) {
//         iVar6 = human->locate->vy / 1000;
//         FntPrint("~c800%02x~c888(%d,%d,%d) ",(char *)(int)human->type,
//                  (char *)(human->locate->vx / 1000),iVar6);
//         FntPrint("~c880%04x=%02x ",(char *)(uint)(ushort)human->attribute,
//                  (char *)(uint)(byte)human->status,iVar6);
//         pMVar2 = human->motion;
//         iVar6 = (int)pMVar2->count;
//         FntPrint("~c080%02x/%d%d ",(char *)(uint)(byte)pMVar2->mid,(char *)(int)pMVar2->loop,iVar6);
//         FntPrint("~c088%03x ~c888%d\n",(char *)(int)human->rotate->vy,
//                  (char *)(int)(*human->model->object)->id,iVar6);
//       }
//       goto LAB_80027c6c;
//     }
// LAB_80027c80:
//     iVar10 = 0;
//   }
//   iVar6 = -1;
//   if (human->status != 7) {
//     iVar6 = iVar10;
//   }
//   PlayMotion(human->motion,(short)iVar6);
//   pVVar3 = human->locate;
//   lVar7 = pVVar3->vy;
//   lVar8 = pVVar3->vz;
//   lVar9 = pVVar3->pad;
//   (human->slocate).vx = pVVar3->vx;
//   (human->slocate).vy = lVar7;
//   (human->slocate).vz = lVar8;
//   (human->slocate).pad = lVar9;
//   human->locate->vx = human->locate->vx + (int)(human->vector).vx;
//   human->locate->vz = human->locate->vz + (int)(human->vector).vz;
//   human->locate->vy = human->locate->vy + (int)(human->vector).vy;
//   UpdateCoordinate(pMVar11);
//   if (iVar10 == 0) {
//     return;
//   }
//   iVar10 = (int)DAT_80097726;
//   (&DAT_800be768)[iVar10] = _DrawTMDmode;
//   sVar1 = ActionHalt;
//   *(Humanoid **)(&DAT_800be858 + iVar10 * 4) = human;
//   DAT_80097726 = DAT_80097726 + 1;
//   if (sVar1 != 0) {
//     return;
//   }
//   if (human->life < 1) {
//     return;
//   }
//   if (human == StagePlayer) {
//     if (human->status == 0xc) {
//       return;
//     }
//     pMVar11 = human->model->object[2];
//     if ((CamState.Mode != CMODE_DIRECTION) && (CamState.Mode != CMODE_SIGHT)) {
//       if (human->motion->loop != -1) {
//         return;
//       }
//       psVar5 = *(short **)&human->motion->motion[1].time;
//       if (((pMVar11->rotate).vx == *psVar5) && ((pMVar11->rotate).vy == psVar5[1])) {
//         return;
//       }
//       (pMVar11->rotate).vx = *psVar5;
//       (pMVar11->rotate).vy = *(short *)(*(int *)&human->motion->motion[1].time + 2);
//       goto LAB_800280e8;
//     }
//     ppMVar4 = human->model->object;
//     iVar6 = (int)CamState.DirectionRY -
//             ((int)((*ppMVar4)->rotate).vy + (int)(ppMVar4[1]->rotate).vy);
//     iVar10 = iVar6;
//     if (iVar6 < 0) {
//       iVar10 = -iVar6;
//     }
//     if (iVar10 < 0x385) {
//       (pMVar11->rotate).vy = (short)iVar6;
//     }
//     else {
//       if (iVar6 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar6 == -1) && (iVar10 * 900 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (pMVar11->rotate).vy = (short)((iVar10 * 900) / iVar6);
//     }
//     iVar10 = (int)CamState.DirectionRX;
//     iVar6 = iVar10;
//     if (iVar10 < 0) {
//       iVar6 = -iVar10;
//     }
//     if (500 < iVar6) {
//       if (iVar10 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar10 == -1) && (iVar6 * 500 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (pMVar11->rotate).vx = (short)((iVar6 * 500) / iVar10);
//       goto LAB_800280e8;
//     }
//   }
//   else {
//     if ((human->attribute & 3U) != 2) {
//       if (human->target == (ModelType *)StagePlayer->model) {
//         return;
//       }
//       if (human->motion->mid == 0x100) {
//         return;
//       }
//     }
//     ppMVar4 = human->model->object;
//     sVar1 = GetDirection((human->target->locate).coord.t[0] - human->locate->vx,
//                          (human->target->locate).coord.t[2] - human->locate->vz,
//                          ((*ppMVar4)->rotate).vy + (ppMVar4[1]->rotate).vy + human->rotate->vy);
//     iVar6 = (int)sVar1;
//     iVar10 = iVar6;
//     if (iVar6 < 0) {
//       iVar10 = -iVar6;
//     }
//     if (0x707 < iVar10) {
//       return;
//     }
//     pMVar11 = human->model->object[2];
//     if (iVar10 < 0x385) {
//       (pMVar11->rotate).vy = sVar1;
//     }
//     else {
//       if (iVar6 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar6 == -1) && (iVar10 * 900 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (pMVar11->rotate).vy = (short)((iVar10 * 900) / iVar6);
//     }
//     iVar10 = ((human->target->locate).coord.t[1] - human->locate->vy) / 2;
//     if (iVar10 == 0) goto LAB_800280e8;
//     sVar1 = -500;
//     if ((iVar10 < -500) || (sVar1 = 100, 100 < iVar10)) {
//       (pMVar11->rotate).vx = sVar1;
//       goto LAB_800280e8;
//     }
//   }
//   (pMVar11->rotate).vx = (short)iVar10;
// LAB_800280e8:
//   UpdateCoordinate(pMVar11);
//   return;
// }
