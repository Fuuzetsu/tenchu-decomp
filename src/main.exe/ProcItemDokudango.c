#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemDokudango(struct tag_TItem *item);
 *     ITEM.C:1632, 141 src lines, frame 128 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s4       struct tag_TItem * item
 *     reg   $s0       struct Sprite3D * model
 *     reg   $s6       struct param_dokudango * param
 *     reg   $s4       struct tag_TItem * item
 *     stack sp+16     struct TFindItemTarget find
 *     reg   $s5       struct Humanoid * target
 *     reg   $s7       int targetlen
 *     reg   $v0       struct TFindItemTarget * find
 *     reg   $v1       struct VECTOR * pos
 *     reg   $s7       int dist
 *     reg   $s3       struct TFindItemTarget * find
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     reg   $v0       int dist
 *     stack sp+48     struct PARAM_ITEM_LAUNCH param
 *     reg   $s4       struct tag_TItem * item
 *     reg   $s0       struct Humanoid * human
 *     reg   $v0       struct VECTOR * tv
 *     reg   $s6       struct param_korogari * param
 *     reg   $s1       int x
 *     reg   $s0       int y
 *     reg   $v0       int z
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s4       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern short ActionHalt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemDokudango", ProcItemDokudango);

// triage: VERY-HARD — 617 insns, mul/div, 4 loop, indirect-call, 15 callees, ~0.20 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemDokudango(tag_TItem *item)
//
// {
//   byte bVar1;
//   ushort uVar2;
//   int iVar3;
//   ModelType *pMVar4;
//   int iVar5;
//   VECTOR *pVVar6;
//   int iVar7;
//   int iVar8;
//   MotionDataType *pMVar9;
//   undefined **ppuVar10;
//   Sprite3D *pSVar11;
//   ModelArchiveType *pMVar12;
//   Humanoid *pHVar13;
//   SVECTOR *pSVar14;
//   MotionManager *mmp;
//   short sVar15;
//   undefined4 uVar16;
//   undefined4 uVar17;
//   undefined4 uVar18;
//   Sprite3D *sprt;
//   Humanoid **ppHVar19;
//   Humanoid *pHVar20;
//   Humanoid *local_70;
//   int local_6c;
//   int local_68;
//   VECTOR local_64;
//   int local_54;
//   PARAM_ITEM_USE local_50;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//     if ((iVar3 != 0) &&
//        (ppuVar10 = (undefined **)(item->param).dokudango.org_think, ppuVar10 != (undefined **)0x0))
//     {
//       ((item->param).ninken.slave)->think[0] = ppuVar10;
//       ((item->param).ninken.slave)->target = (ModelType *)item->owner->model;
//     }
//     (item->param).gun.vec.pad = 0;
//     item->mode = '\0';
//     return;
//   }
//   if ((item->mode < 2) &&
//      (MoveKorogari(item,(param_korogari *)&(item->param).launch),
//      *(char *)((int)&item->param + 10) == '\x01')) {
//     ppuVar10 = item->proc;
//     if (ppuVar10 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   else {
//     if (item->mode < 3) {
//       UpdateCoordinate(item->locate);
//       pMVar4 = item->locate;
//       pSVar14 = &pMVar4->rotate;
//       pSVar11 = sprt;
//       do {
//         uVar16 = *(undefined4 *)(pMVar4->locate).coord.m[0];
//         uVar17 = *(undefined4 *)((pMVar4->locate).coord.m[0] + 2);
//         uVar18 = *(undefined4 *)((pMVar4->locate).coord.m[1] + 1);
//         (pSVar11->locate).flg = (pMVar4->locate).flg;
//         *(undefined4 *)(pSVar11->locate).coord.m[0] = uVar16;
//         *(undefined4 *)((pSVar11->locate).coord.m[0] + 2) = uVar17;
//         *(undefined4 *)((pSVar11->locate).coord.m[1] + 1) = uVar18;
//         pMVar4 = (ModelType *)((pMVar4->locate).coord.m + 2);
//         pSVar11 = (Sprite3D *)((pSVar11->locate).coord.m + 2);
//       } while (pMVar4 != (ModelType *)pSVar14);
//       DrawSprite(sprt);
//     }
//     bVar1 = item->mode;
//     if (bVar1 == 1) {
//       if ((GameClock & 1U) != 0) {
//         return;
//       }
//       pMVar4 = item->locate;
//       local_68 = 0;
//       local_64.vx = (pMVar4->locate).coord.t[0];
//       local_64.vy = (pMVar4->locate).coord.t[1];
//       local_64.vz = (pMVar4->locate).coord.t[2];
//       local_54 = 10000;
//       pHVar13 = (Humanoid *)0x0;
//       iVar3 = 10000;
//       iVar7 = 10000;
// LAB_8004127c:
//       iVar8 = iVar3;
//       pHVar20 = pHVar13;
//       ppHVar19 = HumanGroup + local_68;
//       for (iVar3 = local_68; pHVar13 = (Humanoid *)0x0, iVar3 < Humans; iVar3 = iVar3 + 1) {
//         pHVar13 = *ppHVar19;
//         if ((((0 < pHVar13->life) && (pHVar13->motion->mid != 0x100)) &&
//             ((pHVar13->attribute & 0x80U) == 0)) &&
//            (iVar5 = GetVectorDistance(&local_64,pHVar13->locate), iVar5 < local_54)) {
//           local_68 = iVar3 + 1;
//           local_70 = pHVar13;
//           local_6c = iVar5;
//           break;
//         }
//         ppHVar19 = ppHVar19 + 1;
//       }
//       if (pHVar13 != (Humanoid *)0x0) {
//         pHVar13 = pHVar20;
//         iVar3 = iVar8;
//         if ((((local_70->type & 0xf0U) != 0x80) && (local_70->life != -1)) &&
//            ((local_6c < iVar8 && (pHVar13 = local_70, iVar3 = local_6c, local_70 == item->owner))))
//         {
//           pHVar13 = pHVar20;
//           iVar3 = iVar8;
//           iVar7 = local_6c;
//         }
//         goto LAB_8004127c;
//       }
//       if (iVar7 < 500) {
//         local_50.type = item->type;
//         local_50.user = item->owner;
//         local_50.start.vx = (item->locate->locate).coord.t[0];
//         local_50.start.vy = (item->locate->locate).coord.t[1];
//         local_50.start.vz = (item->locate->locate).coord.t[2];
//         local_50.end.vx = 0;
//         local_50.end.vy = 0;
//         local_50.end.vz = 0;
//         if (item->proc != (undefined **)0x0) {
//           item->mode = 0xff;
//           (*(code *)item->proc)(item);
//           DeleteConflict(item->locate);
//           if (item->mode != 0) {
//             AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//           }
//           item->owner = (Humanoid *)0x0;
//           item->proc = (undefined **)0x0;
//         }
//         ReqItemDrop(&local_50);
//         return;
//       }
//       if (pHVar20 == (Humanoid *)0x0) {
//         return;
//       }
//       iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//       if ((iVar3 != 0) &&
//          (ppuVar10 = (undefined **)(item->param).dokudango.org_think, ppuVar10 != (undefined **)0x0)
//          ) {
//         ((item->param).ninken.slave)->think[0] = ppuVar10;
//         ((item->param).ninken.slave)->target = (ModelType *)item->owner->model;
//       }
//       (item->param).gun.vec.pad = 0;
//       (item->param).gun.vec.pad = (long)pHVar20;
//       if ((pHVar20->target == (ModelType *)item->owner->model) && ((pHVar20->attribute & 3U) == 0))
//       {
//         (item->param).dokudango.org_think = (undefined *)pHVar20->think[0];
//         pHVar20->target = item->locate;
//         ((item->param).ninken.slave)->think[0] = (undefined **)Think1target;
//       }
//       else {
//         (item->param).dokudango.org_think = (undefined *)0x0;
//       }
//       if (999 < iVar8) {
//         return;
//       }
//       pHVar13 = (item->param).ninken.slave;
//       sVar15 = pHVar13->status;
//       if (sVar15 == 0x10) {
//         return;
//       }
//       if (sVar15 == 8) {
//         return;
//       }
//       if (sVar15 == 7) {
//         return;
//       }
//       if (pHVar13->life < 1) {
//         return;
//       }
//       if (ActionHalt == 0) {
//         FUN_800270c8(pHVar13,3);
//         UpdateMotion(pHVar13->motion,0xf01);
//         pHVar13->status = 0xf;
//         pMVar9 = pHVar13->motion->motion;
//         MoveHumanoid(pHVar13,(ushort)pMVar9->orderspd,(ushort)pMVar9->sidespd);
//       }
//       pMVar12 = ((item->param).ninken.slave)->model;
//       if (pMVar12->n < 0xf) {
//         (item->locate->locate).super = &pMVar12->object[2]->locate;
//         (item->locate->locate).coord.t[0] = 0;
//         (item->locate->locate).coord.t[1] = 0;
//         (item->locate->locate).coord.t[2] = -0x96;
//       }
//       else {
//         (item->locate->locate).super = &pMVar12->object[0xe]->locate;
//         (item->locate->locate).coord.t[0] = 0;
//         (item->locate->locate).coord.t[1] = 0x32;
//         (item->locate->locate).coord.t[2] = 0;
//       }
// LAB_800417f0:
//       item->mode = item->mode + '\x01';
//       return;
//     }
//     if (bVar1 < 2) {
//       if (bVar1 != 0) {
//         return;
//       }
//       uVar2 = (item->param).lightningbolt.rot.vz - 1;
//       (item->param).lightningbolt.rot.vz = uVar2;
//       if (0 < (int)((uint)uVar2 << 0x10)) {
//         return;
//       }
//       goto LAB_800417f0;
//     }
//     if (bVar1 == 2) {
//       iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//       if (iVar3 != 0) {
//         pHVar13 = (item->param).ninken.slave;
//         if (pHVar13->motion->mid != 0xf01) {
//           pVVar6 = GetAbsolutePosition(item->locate,0,0,0);
//           (item->locate->locate).super = (_GsCOORDINATE2 *)0x0;
//           (item->locate->locate).coord.t[0] = pVVar6->vx;
//           (item->locate->locate).coord.t[1] = pVVar6->vy;
//           (item->locate->locate).coord.t[2] = pVVar6->vz;
//           iVar3 = rand();
//           iVar7 = rand();
//           iVar8 = rand();
//           (item->param).lightningbolt.rot.vz = 0x1e;
//           (item->param).napalm.vec.vz = (short)iVar3 + (short)(iVar3 / 200) * -200 + -100;
//           (item->param).napalm.vec.pad = (short)iVar7 + (short)(iVar7 / 100) * -100 + -200;
//           (item->param).smoke.koro.hint = (AreaNodeType *)0x0;
//           *(undefined1 *)((int)&item->param + 10) = 0;
//           *(short *)((int)&item->param + 8) = (short)iVar8 + (short)(iVar8 / 200) * -200 + -100;
//           item->mode = '\0';
//           return;
//         }
//         if (pHVar13->motion->count != 0x37) {
//           if ((pHVar13->attribute & 0x40U) == 0) {
//             return;
//           }
//           if ((pHVar13->type & 0xf0U) == 0xa0) {
//             return;
//           }
//           NowReturnNormal(pHVar13);
//           return;
//         }
//         iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//         if ((iVar3 != 0) &&
//            (ppuVar10 = (undefined **)(item->param).dokudango.org_think,
//            ppuVar10 != (undefined **)0x0)) {
//           ((item->param).ninken.slave)->think[0] = ppuVar10;
//           ((item->param).ninken.slave)->target = (ModelType *)item->owner->model;
//         }
//         (item->param).gun.vec.pad = 0;
//         (item->param).ninken.slave = pHVar13;
//         NowReturnNormal(pHVar13);
//         (item->param).lightningbolt.rot.vz = 600;
//         goto LAB_800417f0;
//       }
//     }
//     else {
//       if (bVar1 != 3) {
//         return;
//       }
//       iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//       if (((iVar3 != 0) &&
//           (sVar15 = (item->param).lightningbolt.rot.vz,
//           (item->param).lightningbolt.rot.vz = sVar15 + -1, sVar15 != 1)) &&
//          (pHVar13 = (item->param).ninken.slave, 0 < pHVar13->life)) {
//         sVar15 = pHVar13->status;
//         if (sVar15 == 0x10) {
//           return;
//         }
//         if (sVar15 == 8) {
//           return;
//         }
//         if (sVar15 == 7) {
//           return;
//         }
//         if (sVar15 == 0xf) {
//           return;
//         }
//         iVar3 = rand();
//         if (2 < iVar3 % 0x1e) {
//           return;
//         }
//         pHVar13 = (item->param).ninken.slave;
//         if ((pHVar13->type & 0xf0U) == 0xa0) {
//           if ((ActionHalt != 0) || (pHVar13->life < 1)) goto LAB_800419f8;
//           FUN_800270c8(pHVar13,3);
//           mmp = pHVar13->motion;
//           sVar15 = 0x1000;
//         }
//         else {
//           if ((ActionHalt != 0) || (pHVar13->life < 1)) goto LAB_800419f8;
//           FUN_800270c8(pHVar13,3);
//           mmp = pHVar13->motion;
//           sVar15 = 0x100b;
//         }
//         UpdateMotion(mmp,sVar15);
//         pHVar13->status = 0xf;
//         pMVar9 = pHVar13->motion->motion;
//         MoveHumanoid(pHVar13,(ushort)pMVar9->orderspd,(ushort)pMVar9->sidespd);
// LAB_800419f8:
//         Sound((item->param).ninken.slave,6);
//         return;
//       }
//     }
//     ppuVar10 = item->proc;
//     if (ppuVar10 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   (*(code *)ppuVar10)(item);
//   DeleteConflict(item->locate);
//   if (item->mode != 0) {
//     AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//   }
//   item->owner = (Humanoid *)0x0;
//   item->proc = (undefined **)0x0;
//   return;
// }
