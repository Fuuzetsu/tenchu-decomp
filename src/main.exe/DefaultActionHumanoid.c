#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DefaultActionHumanoid(struct Humanoid *human);
 *     HUMAN.C:155, 175 src lines, frame 120 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
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
 *     param $s1       struct Humanoid * human
 *     reg   $s3       struct MapVector * map
 *     reg   $s5       struct SVECTOR * vector
 *     reg   $s2       struct VECTOR * locate
 *     reg   $s4       struct VECTOR * slocate
 *     reg   $s7       struct ModelType * object
 *     reg   $a2       long i
 *     reg   $s4       long xx
 *     reg   $v1       long yy
 *     reg   $s0       long zz
 *     reg   $s0       long ry
 *     stack sp+32     struct VECTOR position
 *     stack sp+48     struct MapVector mv
 *     stack sp+64     struct VECTOR position
 *
 * Globals it touches, as the original declared them:
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *StagePlayer;
 *     extern short RefrectMove[16][2];
 *     extern struct SVECTOR ConflictDistance;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DefaultActionHumanoid", DefaultActionHumanoid);

// triage: VERY-HARD — 656 insns, mul/div, 7 loop, 10 callees, ~0.05 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Loops: 7 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short DefaultActionHumanoid(Humanoid *human)
//
// {
//   ushort uVar1;
//   ulong *mvp;
//   short sVar2;
//   int iVar3;
//   int iVar4;
//   int iVar5;
//   int iVar6;
//   MotionDataType *pMVar7;
//   int iVar8;
//   int iVar9;
//   VECTOR *wide;
//   int iVar10;
//   long lVar11;
//   uint uVar12;
//   VECTOR *pVVar13;
//   MapVector *pos;
//   ModelType *model;
//   VECTOR local_60;
//   VECTOR local_50;
//   int local_38;
//   int local_34;
//   int local_30;
//   long local_2c;
//
//   pos = &human->map;
//   pVVar13 = human->locate;
//   model = *human->model->object;
//   human->rotate->vy = human->rotate->vy & 0xfff;
//   human->attribute = (ushort)(byte)human->attribute;
//   if (human->type != 0x85) {
//     FieldArea = *(AreaNodeType **)&human->field10_0x30;
//     FieldIndex = *(NodeIndexType **)&human->field14_0x34;
//   }
//   if (human->status == 7) {
// LAB_80028214:
//     sVar2 = (*human->model->object)->id;
//     local_60.vx = ConflictObject[sVar2].position.vx;
//     local_60.vz = ConflictObject[sVar2].position.vz;
//     local_60.pad = ConflictObject[sVar2].position.pad;
//     local_60.vy = pVVar13->vy;
//     wide = &local_60;
//   }
//   else {
//     wide = pVVar13;
//     if ((human->status != 0xb) && ((human->map).height == 0)) {
//       iVar3 = (int)(human->vector).vx;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       if (iVar3 < 0x51) {
//         iVar3 = (int)(human->vector).vz;
//         if (iVar3 < 0) {
//           iVar3 = -iVar3;
//         }
//         if (iVar3 < 0x51) goto LAB_80028290;
//       }
//       goto LAB_80028214;
//     }
//   }
// LAB_80028290:
//   GetAreaMapVector((MapVector *)GlobalAreaMap,(VECTOR *)pos,(long)wide);
//   if (((human->map).attrib & 2U) != 0) {
//     human->attribute = human->attribute | 0x200;
//     if ((human->vector).vy < 0) {
//       (human->vector).vy = 0;
//     }
//     (human->map).height = 1;
//   }
//   if (((human->map).height < 1) || ((human->attribute & 0x20U) != 0)) {
//     if ((0 < (human->vector).vy) || (pos->level == -0x80000000)) {
//       if (((human->map).attrib & 0x200U) == 0) {
//         human->attribute = human->attribute | 0x800;
//       }
//       if (pos->level != -0x80000000) {
//         pVVar13->vy = pos->level;
//       }
//       (human->vector).vy = 0;
//     }
// LAB_8002841c:
//     if (((((human->map).attrib & 0x200U) != 0) && ((human->map).height == 0)) &&
//        (human->status != 0x11)) {
//       SetNowMotion(human,0x1100,1);
//     }
//   }
//   else {
//     human->attribute = human->attribute | 0x100;
//     if ((human->vector).vy < 400) {
//       (human->vector).vy = (human->vector).vy + 0x14;
//     }
//     if (((human->map).attrib & 0x200U) != 0) {
//       if (((human->map).height < 25000) && (human->life != 0)) {
//         if (human == StagePlayer) {
//           Sound(human,0x49);
//           SetCameraMode(CMODE_FALL);
//         }
//         else {
//           Sound(human,8);
//         }
//         human->life = 0;
//         if (((human->type & 0xf0U) != 0x80) && (human != StagePlayer)) {
//           ReqLifeBar(human);
//         }
//       }
//       goto LAB_8002841c;
//     }
//   }
//   uVar12._0_1_ = (human->map).vector;
//   uVar12._1_1_ = (human->map).direct;
//   uVar12._2_1_ = (human->map).angleL;
//   uVar12._3_1_ = (human->map).angleH;
//   if ((uVar12 & 0xffff0000) != 0) {
//     uVar1 = human->attribute;
//     human->attribute = uVar1 | 0x2000;
//     iVar3 = (human->map).height;
//     if (iVar3 < -0x1c2) {
//       human->attribute = uVar1 | 0x3000;
//     }
//     else if (-1 < iVar3) goto LAB_800284b0;
//     pVVar13->vy = pos->level;
//   }
// LAB_800284b0:
//   if ((human->map).vector != '\0') {
//     human->attribute = human->attribute | 0x400;
//     mvp = GlobalAreaMap;
//     if (pos->level == -0x80000000) {
//       sVar2 = (*human->model->object)->id;
//       local_38 = ConflictObject[sVar2].position.vx;
//       local_30 = ConflictObject[sVar2].position.vz;
//       local_2c = ConflictObject[sVar2].position.pad;
//       pVVar13->vx = (human->slocate).vx;
//       pVVar13->vz = (human->slocate).vz;
//       local_34 = (human->slocate).vy;
//       pVVar13->vy = local_34;
//       local_34 = local_34 + -500;
//       GetAreaMapVector((MapVector *)mvp,&local_50,(long)&local_38);
//       iVar3 = local_38 - pVVar13->vx;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       pVVar13->vx = pVVar13->vx + RefrectMove[0][(uint)(byte)local_50.pad * 2] * iVar3;
//       iVar3 = local_30 - pVVar13->vz;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       pVVar13->vz = pVVar13->vz + RefrectMove[0][(uint)(byte)local_50.pad * 2 + 1] * iVar3;
//       if ((local_50.vx == -0x80000000) || (iVar3 = pVVar13->vy, local_50.vx <= iVar3)) {
//         (human->vector).vz = 0;
//         (human->vector).vx = 0;
//       }
//       else {
//         iVar4 = (int)(human->vector).vy;
//         iVar5 = iVar3 + iVar4;
//         if (iVar4 < 1) {
//           iVar5 = iVar3 + 0x14;
//         }
//         pVVar13->vy = iVar5;
//       }
//     }
//     else {
//       uVar12 = (uint)(human->map).vector;
//       iVar3 = (int)RefrectVector[uVar12];
//       if (iVar3 == -1) {
//         iVar4 = (int)(human->vector).vx;
//         if ((iVar4 == 0) && ((human->vector).vz == 0)) {
//           iVar8 = (int)((uint)(ushort)human->width << 0x10) >> 0x12;
//           iVar4 = RefrectMove[0][uVar12 * 2] * iVar8;
//           iVar8 = RefrectMove[0][uVar12 * 2 + 1] * iVar8;
//         }
//         else {
//           iVar4 = -iVar4;
//           iVar8 = -(int)(human->vector).vz;
//         }
//         iVar4 = pVVar13->vx + iVar4;
//         iVar8 = pVVar13->vz + iVar8;
//       }
//       else {
//         iVar5 = rsin(iVar3);
//         iVar5 = iVar5 * human->width;
//         iVar4 = iVar5 >> 0xe;
//         iVar9 = rcos(iVar3);
//         iVar9 = iVar9 * human->width;
//         iVar3 = human->rotate->vy - iVar3;
//         iVar10 = iVar3;
//         if (iVar3 < 0) {
//           iVar10 = -iVar3;
//         }
//         iVar8 = iVar9 >> 0xe;
//         iVar6 = iVar3;
//         if ((1999 < iVar10) && (iVar6 = iVar3 + -0x1000, iVar3 < 1)) {
//           iVar6 = iVar3 + 0x1000;
//         }
//         iVar3 = iVar6;
//         if (iVar6 < 0) {
//           iVar3 = -iVar6;
//         }
//         if (((iVar3 < 0x708) || (human != StagePlayer)) || ((human->map).height != 0)) {
//           if (((human->map).angleH == '\0') && ((human->status == 2 || (human->status == 6)))) {
//             if (iVar6 < 1) {
//               sVar2 = 0x20;
//             }
//             else {
//               sVar2 = -0x20;
//             }
//             human->rotate->vy = human->rotate->vy + sVar2;
//             pMVar7 = human->motion->motion;
//             MoveHumanoid(human,(ushort)pMVar7->orderspd,(ushort)pMVar7->sidespd);
//           }
//           iVar4 = iVar5 >> 0xf;
//           iVar8 = iVar9 >> 0xf;
//         }
//         iVar4 = pVVar13->vx - iVar4;
//         iVar8 = pVVar13->vz - iVar8;
//       }
//       pVVar13->vx = iVar4;
//       pVVar13->vz = iVar8;
//     }
//   }
//   if (((int)model->attribute & 0x8000U) != 0) {
//     while( true ) {
//       sVar2 = GetConflictResult(model,-1);
//       uVar12 = (uint)sVar2;
//       if ((int)uVar12 < 0) break;
//       if ((Humanoid *)ConflictObject[uVar12].common != human) {
//         uVar1 = ConflictObject[uVar12].size.pad;
//         if ((uVar1 & 1) == 0) {
//           if (((uVar1 & 8) == 0) && ((human->attribute & 0x20U) == 0)) {
//             human->attribute = human->attribute | 0x8000;
//             iVar3 = pVVar13->vx;
//             lVar11 = pVVar13->vz;
//             if (ConflictDistance.vx < 0) {
//               iVar4 = -(int)human->width;
//             }
//             else {
//               iVar4 = (int)human->width;
//             }
//             if (iVar4 < 0) {
//               iVar4 = iVar4 + 7;
//             }
//             pVVar13->vx = iVar3 - (iVar4 >> 3);
//             if (ConflictDistance.vz < 0) {
//               iVar4 = -(int)human->width;
//             }
//             else {
//               iVar4 = (int)human->width;
//             }
//             if (iVar4 < 0) {
//               iVar4 = iVar4 + 7;
//             }
//             pVVar13->vz = pVVar13->vz - (iVar4 >> 3);
//             iVar10 = (int)ConflictObject[uVar12].size.vy;
//             iVar5 = ConflictObject[uVar12].position.vy;
//             iVar9 = ConflictObject[model->id].position.vy;
//             iVar4 = iVar5 - iVar10;
//             if (iVar9 < iVar4) {
//               pVVar13->vx = iVar3;
//               pVVar13->vz = lVar11;
//               if ((ConflictObject[uVar12].size.pad & 4U) == 0) {
//                 (human->vector).vy = 10;
//               }
//               else {
//                 (human->vector).vy = 0;
//                 pos->level = iVar4;
//                 pVVar13->vy = iVar4;
//                 (human->map).height = 0;
//                 (human->map).attrib = (human->map).attrib & 0xff80;
//                 human->attribute = human->attribute & 0x7eff;
//               }
//             }
//             else {
//               if ((iVar5 + iVar10 < iVar9) &&
//                  ((((sVar2 = human->status, sVar2 == 0 || (sVar2 == 2)) || (sVar2 == 5)) ||
//                   (sVar2 == 6)))) {
//                 sVar2 = GetDirection(ConflictObject[uVar12].position.vx - pVVar13->vx,
//                                      ConflictObject[uVar12].position.vz - pVVar13->vz,
//                                      (short)human->locate->vy);
//                 iVar3 = (int)sVar2;
//                 if (iVar3 < 0) {
//                   iVar3 = -iVar3;
//                 }
//                 sVar2 = 0x1003;
//                 if (iVar3 < 0x44c) {
//                   sVar2 = 0x1000;
//                 }
//                 SetNowMotion(human,sVar2,1);
//                 Sound(human,6);
//               }
//               if ((uVar12 & 1) == 0) {
//                 sVar2 = -human->turn;
//               }
//               else {
//                 sVar2 = human->turn;
//               }
//               human->rotate->vy = human->rotate->vy + sVar2;
//               if (((human->vector).vx != 0) || ((human->vector).vz != 0)) {
//                 pMVar7 = human->motion->motion;
//                 MoveHumanoid(human,(ushort)pMVar7->orderspd,(ushort)pMVar7->sidespd);
//                 if ((human->trace != (TraceLine *)0x0) && ((human->attribute & 8U) != 0)) {
//                   human->trace->count = -0x14;
//                 }
//               }
//             }
//           }
//         }
//         else if (human->status != 0x11) {
//           uVar1 = human->attribute;
//           (human->vector).pad = sVar2;
//           human->attribute = uVar1 | 0x4000;
//         }
//       }
//     }
//   }
//   return human->attribute;
// }
