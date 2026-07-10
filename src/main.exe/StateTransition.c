#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StateTransition(struct Humanoid *human);
 *     THINK.C:99, 109 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct Humanoid * human
 *     reg   $s2       short pad
 *     reg   $s1       short atr0
 *     reg   $s3       long ssr
 *     reg   $s0       short motid
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern long StrainRatio;
 *     extern struct PADtype *Pad;
 *     extern short Attrib;
 *     extern short ActionHalt;
 *     extern long Distance;
 *     extern short Degree;
 *     extern short SR;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short FieldAttrib;
 *     extern short EngageLevel;
 *     extern short Findenemies;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/StateTransition", StateTransition);

// triage: VERY-HARD — 944 insns, mul/div, indirect-call, 15 callees, ~0.04 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void StateTransition(Humanoid *human)
//
// {
//   ModelType **ppMVar1;
//   Humanoid *pHVar2;
//   short sVar3;
//   ushort uVar4;
//   long lVar5;
//   int iVar6;
//   byte *pbVar7;
//   long lVar8;
//   long lVar9;
//   ushort uVar10;
//   code *pcVar11;
//   VECTOR *pVVar12;
//   PADtype *pPVar13;
//   int iVar14;
//   VECTOR *pVVar15;
//   uint uVar16;
//   int iVar17;
//   ushort unaff_s2;
//   ushort uVar18;
//   SVECTOR local_20;
//
//   lVar5 = StrainRatio;
//   uVar4 = human->attribute;
//   Pad = &human->pad;
//   uVar18 = uVar4 & 0xffec;
//   if (human == StagePlayer) {
//     DAT_80097f1c = StrainRatio;
//     StrainRatio = 0x7fffffff;
//   }
//   Me_THINK_C = human;
//   Attrib = uVar4;
//   if ((ushort)human->status - 0x10 < 2) {
//     if (((human != StagePlayer) && (0 < human->life)) && (0 < StrainRatio)) {
//       StrainRatio = -0x8000;
//     }
//     iVar14 = 0;
//     pPVar13 = Pad;
//     goto LAB_8002b968;
//   }
//   if (ActionHalt != 0) {
//     uVar4 = 0;
//     if ((ushort)human->type - 0x10 < 0x70) {
//       iVar17 = (human->target->locate).coord.t[0] - human->locate->vx;
//       iVar14 = (human->target->locate).coord.t[2] - human->locate->vz;
//       sVar3 = GetDirection(iVar17,iVar14,human->rotate->vy);
//       if ((int)Me_THINK_C->turn < (int)sVar3) {
//         uVar4 = 0x2000;
//       }
//       else {
//         uVar4 = 0;
//         if ((int)sVar3 < -(int)Me_THINK_C->turn) {
//           uVar4 = 0x8000;
//         }
//       }
//       lVar5 = SquareRoot0(iVar17 * iVar17 + iVar14 * iVar14);
//       if (lVar5 < 2000) {
//         uVar4 = uVar4 | 0x4000;
//       }
//     }
//     iVar14 = (int)(short)uVar4;
//     pPVar13 = Pad;
//     goto LAB_8002b968;
//   }
//   if ((uVar4 & 4) == 0) {
//     if (((human == StagePlayer) && (iVar14 = DAT_800979c0 + -1, DAT_800979c0 != 0)) &&
//        (DAT_800979c0 = iVar14, iVar14 < 0)) {
//       DAT_800979c0 = 0;
//     }
//     sVar3 = (*(code *)human->think[0])();
//     iVar14 = (int)sVar3;
//     pPVar13 = Pad;
//     goto LAB_8002b968;
//   }
//   SR = SearchTarget(human,&Distance,&Degree);
//   lVar8 = Distance;
//   if (Me_THINK_C->target != (ModelType *)StagePlayer->model) {
//     pVVar12 = StagePlayer->locate;
//     pVVar15 = Me_THINK_C->locate;
//     iVar14 = pVVar12->vx - pVVar15->vx;
//     iVar17 = pVVar12->vy - pVVar15->vy;
//     iVar6 = pVVar12->vz - pVVar15->vz;
//     lVar8 = SquareRoot0(iVar14 * iVar14 + iVar17 * iVar17 + iVar6 * iVar6);
//   }
//   sVar3 = StagePlayer->itmctl;
//   if ((sVar3 == 0xb) || (sVar3 == 0x12)) {
//     uVar10 = Me_THINK_C->type & 0xf0;
//     if (((uVar10 != 0x80) && (uVar10 != 0xa0)) && ((sVar3 != 0x12 || ((Attrib & 3U) != 2)))) {
//       if (DAT_800979c0 != 0) {
//         DAT_800979c0 = 0;
//       }
//       SR = -2;
//     }
//   }
//   else if (DAT_800979c0 != 0) {
//     if (DAT_800979c0 == 1) {
//       SR = -1;
//     }
//     else if ((Attrib & 3U) == 0) {
//       SR = 2;
//     }
//     else if (SR == 2) {
//       SR = 1;
//     }
//   }
//   GetMoveSpeed(&local_20,Me_THINK_C->rotate->vy,
//                (short)((uint)((int)Me_THINK_C->width << 0x11) >> 0x10),0);
//   DAT_80097f10 = GetAreaMapLevel(GlobalAreaMap,Me_THINK_C->locate->vx + (int)local_20.vx,
//                                  Me_THINK_C->locate->vy + -0xbea);
//   DAT_80097f18 = FieldAttrib;
//   DAT_80097f14 = GetAreaMapLevel(GlobalAreaMap,Me_THINK_C->locate->vx - (int)local_20.vx,
//                                  Me_THINK_C->locate->vy + -0xbea);
//   DAT_80097f1a = FieldAttrib;
//   uVar10 = Attrib & 3;
//   if (uVar10 == 1) {
//     if ((DAT_800979c0 == 0) && ((Attrib & 0x10U) == 0)) {
//       if ((0 < StrainRatio) || (StrainRatio < -lVar8)) {
//         StrainRatio = -lVar8;
//       }
//       unaff_s2 = (*(code *)Me_THINK_C->think[1])();
//     }
//     else {
//       if (0 < StrainRatio) {
//         StrainRatio = -0x8000;
//       }
//       if ((Attrib & 0x10U) == 0) {
//         unaff_s2 = Think2confirm();
//       }
//       else {
//         unaff_s2 = FUN_8002c86c();
//       }
//     }
//     if ((SR == 1) || (((Attrib & 0x4000U) != 0 && (0 < SR)))) {
//       Attrib = uVar18 | 2;
//       if ((uVar4 & 0x40) == 0) {
//         SetNowMotion(Me_THINK_C,0x80e,1);
//       }
//       pHVar2 = Me_THINK_C;
//       Me_THINK_C->chase[1] = 0;
//       pHVar2->chase[0] = 0;
//       Sound(pHVar2,0xd);
//       uVar4 = Attrib;
//       if (0 < Me_THINK_C->life) {
//         FUN_8002f7f4();
//         sVar3 = Me_THINK_C->type;
// joined_r0x8002b170:
//         uVar4 = Attrib;
//         if (sVar3 < 0x80) {
// LAB_8002b5a4:
//           uVar4 = Attrib;
//           if (Me_THINK_C->target == (ModelType *)StagePlayer->model) {
//             Findenemies = Findenemies + 1;
//           }
//         }
//       }
//     }
//     else {
//       uVar4 = Attrib;
//       if (((DAT_800979c0 < 2) && ((ushort)(SR + 2U) < 2)) &&
//          (uVar4 = uVar18, (Me_THINK_C->type & 0xf0U) != 0x80)) {
//         SetNowMotion(Me_THINK_C,0x80f,1);
//       }
//     }
// LAB_8002b5d8:
//     Attrib = uVar4;
//     if (Me_THINK_C->field40_0xb0 != (undefined *)0x0) goto LAB_8002b5f0;
//   }
//   else {
//     uVar4 = Attrib;
//     if (uVar10 < 2) {
//       if ((Attrib & 3U) == 0) {
//         if (lVar8 < StrainRatio) {
//           StrainRatio = lVar8;
//         }
//         unaff_s2 = (*(code *)Me_THINK_C->think[0])();
//         uVar4 = Attrib;
//         if (6 < Me_THINK_C->type) {
//           if (SR == 1) {
//             SetNowMotion(Me_THINK_C,0x80e,1);
//             sVar3 = SetNowMotion(Me_THINK_C,0x106,1);
//             if (sVar3 == 0) {
//               Sound(Me_THINK_C,0xd);
//             }
//             pHVar2 = Me_THINK_C;
//             sVar3 = Me_THINK_C->life;
//             Attrib = uVar18 | 2;
//             Me_THINK_C->chase[1] = 0;
//             pHVar2->chase[0] = 0;
//             uVar4 = Attrib;
//             if (0 < sVar3) {
//               FUN_8002f7f4();
//               sVar3 = Me_THINK_C->type;
//               goto joined_r0x8002b170;
//             }
//           }
//           else if (SR == 2) {
//             if (DAT_800979c0 != 0) {
//               SetNowMotion(Me_THINK_C,0x80e,1);
//               pHVar2 = Me_THINK_C;
//               Me_THINK_C->chase[1] = 0;
//               pHVar2->chase[0] = 0;
//             }
//             Attrib = uVar18 | 1;
//             Sound(Me_THINK_C,0xc);
//             uVar4 = Attrib;
//           }
//         }
//       }
//       goto LAB_8002b5d8;
//     }
//     if (uVar10 != 2) {
//       if (uVar10 == 3) {
//         if (0 < StrainRatio) {
//           StrainRatio = -0x8000;
//         }
//         unaff_s2 = (*(code *)Me_THINK_C->think[3])();
//         if (((Attrib & 0x400U) != 0) && (Me_THINK_C->field40_0xb0 == (undefined *)0x0)) {
//           pcVar11 = (code *)&DAT_80000008;
//           if (0 < Degree) {
//             pcVar11 = gte_ldv2;
//           }
//           Me_THINK_C->field40_0xb0 = pcVar11;
//         }
//         uVar4 = Attrib;
//         if ((Attrib & 3U) == 2) {
//           Sound(Me_THINK_C,0xd);
//           FUN_8002f7f4();
//           uVar4 = Attrib;
//           if ((Me_THINK_C->type < 0x80) && ((Attrib & 0x10U) == 0)) goto LAB_8002b5a4;
//         }
//       }
//       goto LAB_8002b5d8;
//     }
//     lVar8 = -1;
//     if ((Attrib & 0x40U) == 0) {
// LAB_8002b228:
//       StrainRatio = lVar8;
//     }
//     else if (Me_THINK_C->target == (ModelType *)StagePlayer->model) {
//       StrainRatio = 0;
//     }
//     else {
//       lVar8 = -0x8000;
//       if (0 < StrainRatio) goto LAB_8002b228;
//     }
//     if ((Attrib & 0x10U) == 0) {
//       if ((Attrib & 0x40U) == 0) {
//         SetNowMotion(Me_THINK_C,0x80e,1);
//       }
//       unaff_s2 = Think3firstattack();
//     }
//     else {
//       unaff_s2 = (*(code *)Me_THINK_C->think[2])();
//     }
//     if ((unaff_s2 & 0x80) != 0) {
//       if (StagePlayer->motion->mid != 0x1009) {
//         iVar14 = (Me_THINK_C->target->locate).coord.t[1] - Me_THINK_C->locate->vy;
//         if (iVar14 < 0) {
//           iVar14 = -iVar14;
//         }
//         if ((iVar14 < 2000) || ((int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14 == 3)) {
//           iVar17 = rand();
//           iVar14 = iVar17;
//           if (iVar17 < 0) {
//             iVar14 = iVar17 + 3;
//           }
//           if ((int)(uint)(byte)PersistentState._88_1_ <= iVar17 + (iVar14 >> 2) * -4 + -2) {
//             unaff_s2 = 0;
//           }
//           goto LAB_8002b348;
//         }
//       }
//       unaff_s2 = unaff_s2 & 0xf000;
//     }
// LAB_8002b348:
//     pHVar2 = Me_THINK_C;
//     if (SR == -2) {
//       ppMVar1 = &Me_THINK_C->target;
//       Attrib = uVar18 | 0x13;
//       Me_THINK_C->chase[0] = ((*ppMVar1)->locate).coord.t[0];
//       lVar8 = ((*ppMVar1)->locate).coord.t[2];
//       pHVar2->actscnt = '\x01';
//       pHVar2->chase[1] = lVar8;
//     }
//     if (Me_THINK_C->field40_0xb0 == (undefined *)0x0) {
//       if (((unaff_s2 & 0x4000) != 0) && (((DAT_80097f1a & 0x204) != 0 || (5000 < DAT_80097f14)))) {
//         Me_THINK_C->field40_0xb0 = (undefined *)0x1000001e;
//       }
//       if (((unaff_s2 & 0x1000) != 0) && (((DAT_80097f18 & 0x204) != 0 || (5000 < DAT_80097f10)))) {
//         uVar4 = FUN_8002b990(0,0);
//         unaff_s2 = uVar4 & 0xa000;
//       }
//       uVar4 = Attrib;
//       if (StagePlayer->motion->mid == 0xe01) {
//         iVar14 = rand();
//         iVar17 = EngageLevel + 1;
//         if (iVar17 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar17 == -1) && (iVar14 == -0x80000000)) {
//           trap(0x1800);
//         }
//         if ((iVar14 % iVar17 == 0) || (uVar4 = Attrib, (Me_THINK_C->type & 0xf0U) == 0x80)) {
//           uVar16 = rand();
//           sVar3 = 4;
//           if ((uVar16 & 1) != 0) {
//             sVar3 = 3;
//           }
//           unaff_s2 = SetCommand(&Me_THINK_C->pad,sVar3);
//           uVar4 = Attrib;
//         }
//       }
//       goto LAB_8002b5d8;
//     }
// LAB_8002b5f0:
//     unaff_s2 = *(ushort *)((int)&Me_THINK_C->field40_0xb0 + 2);
//     uVar16 = *(byte *)&Me_THINK_C->field40_0xb0 - 1;
//     if (uVar16 == 0) {
//       if ((unaff_s2 & 0xa000) == 0) {
//         Me_THINK_C->field40_0xb0 = (undefined *)0x0;
//       }
//       else {
//         iVar14 = rand();
//         Me_THINK_C->field40_0xb0 = (undefined *)((iVar14 % 3 + 1) * 0x1e | 0x10000000);
//       }
//     }
//     else {
//       Me_THINK_C->field40_0xb0 = (undefined *)((uint)unaff_s2 << 0x10 | uVar16);
//     }
//   }
//   if (Me_THINK_C->status == 10) {
//     iVar17 = (int)Degree;
//     iVar14 = iVar17;
//     if (iVar17 < 0) {
//       iVar14 = -iVar17;
//     }
//     unaff_s2 = 0x1000;
//     if (499 < iVar14) {
//       unaff_s2 = 0x4000;
//       if ((Attrib & 3U) == 0) {
//         unaff_s2 = 0x1000;
//       }
//       else {
//         pbVar7 = &DAT_8000000f;
//         if (0 < iVar17) {
//           pbVar7 = gte_ldv3 + 3;
//         }
//         Me_THINK_C->field40_0xb0 = pbVar7;
//       }
//     }
//   }
//   else if (((DAT_80097f18 & 0x204) == 0) || ((unaff_s2 & 0x1000) == 0)) {
//     if (((DAT_80097f1a & 0x204) == 0) || ((unaff_s2 & 0x4000) == 0)) {
//       if (Me_THINK_C->motion->count == 0) {
//         iVar14 = (int)Degree;
//         if (iVar14 < 0) {
//           iVar14 = -iVar14;
//         }
//         if ((iVar14 < 500) &&
//            ((Me_THINK_C->think[0] == (undefined **)Think1ninja ||
//             (((Me_THINK_C->type & 0xf0U) == 0x20 && (PersistentState._88_1_ != '\0')))))) {
//           GetMoveSpeed(&local_20,Me_THINK_C->rotate->vy,Me_THINK_C->width * 5,0);
//           lVar8 = GetAreaMapLevel(GlobalAreaMap,Me_THINK_C->locate->vx,
//                                   Me_THINK_C->locate->vy + -0xbea);
//           lVar9 = GetAreaMapLevel(GlobalAreaMap,Me_THINK_C->locate->vx + (int)local_20.vx,
//                                   Me_THINK_C->locate->vy + -0xbea);
//           if (lVar8 == (Me_THINK_C->map).level) {
//             iVar14 = lVar9;
//             if (lVar9 < 0) {
//               iVar14 = -lVar9;
//             }
//             if (499 < iVar14) goto LAB_8002b878;
//           }
//           else {
// LAB_8002b878:
//             if (lVar9 < 0x17d5) goto LAB_8002b93c;
//           }
//           unaff_s2 = 0x1040;
//           goto LAB_8002b93c;
//         }
//       }
//       if ((GameClock == (GameClock / 0x5a) * 0x5a) &&
//          (((((Me_THINK_C->map).attrib & 0x100U) != 0 ||
//            ((((unaff_s2 & 0x1000) != 0 && (DAT_80097f10 < 0x899)) && (DAT_80097f10 != -0x80000000)))
//            ) || ((((unaff_s2 & 0x4000) != 0 && (DAT_80097f14 < 0x899)) &&
//                  (DAT_80097f14 != -0x80000000)))))) {
//         unaff_s2 = unaff_s2 | 0x40;
//       }
//     }
//     else {
//       unaff_s2 = unaff_s2 & 0xbfff;
//     }
//   }
//   else {
//     unaff_s2 = unaff_s2 & 0xefff;
//   }
// LAB_8002b93c:
//   pPVar13 = Pad;
//   if (Me_THINK_C->life < 0) {
//     StrainRatio = lVar5;
//   }
//   iVar14 = (int)(short)unaff_s2;
//   Me_THINK_C->attribute = Attrib;
// LAB_8002b968:
//   FUN_8001b1a4(pPVar13,iVar14);
//   return;
// }
