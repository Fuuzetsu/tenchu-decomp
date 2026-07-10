#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemSmoke(struct tag_TItem *item);
 *     ITEM.C:1400, 59 src lines, frame 112 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct Sprite3D * model
 *     reg   $s0       struct param_smoke * param
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct SVECTOR vec
 *     stack sp+24     struct VECTOR pos
 *     stack sp+56     struct TFindItemTarget find
 *     reg   $v1       struct VECTOR * pos
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     reg   $v0       int dist
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern short ActionHalt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemSmoke", ProcItemSmoke);

// triage: HARD — 251 insns, 4 loop, indirect-call, 13 callees, ~0.17 to ProcItemMakibishi
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemSmoke(tag_TItem *item)
//
// {
//   uchar uVar1;
//   Humanoid *human;
//   undefined **ppuVar2;
//   ModelType *pMVar3;
//   int iVar4;
//   MotionDataType *pMVar5;
//   Sprite3D *pSVar6;
//   Humanoid *pHVar7;
//   SVECTOR *pSVar8;
//   undefined4 uVar9;
//   undefined4 uVar10;
//   undefined4 uVar11;
//   Sprite3D *sprt;
//   int iVar12;
//   Humanoid **ppHVar13;
//   Humanoid *local_48;
//   int local_44;
//   undefined1 local_40 [8];
//   long local_38;
//   long local_34;
//   long local_30;
//   int local_2c;
//   long local_28;
//   long local_24;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   MoveKorogari(item,(param_korogari *)&(item->param).launch);
//   if (*(char *)((int)&item->param + 10) == '\x01') {
//     ppuVar2 = item->proc;
//     if (ppuVar2 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   else {
//     UpdateCoordinate(item->locate);
//     pMVar3 = item->locate;
//     pSVar8 = &pMVar3->rotate;
//     pSVar6 = sprt;
//     do {
//       uVar9 = *(undefined4 *)(pMVar3->locate).coord.m[0];
//       uVar10 = *(undefined4 *)((pMVar3->locate).coord.m[0] + 2);
//       uVar11 = *(undefined4 *)((pMVar3->locate).coord.m[1] + 1);
//       (pSVar6->locate).flg = (pMVar3->locate).flg;
//       *(undefined4 *)(pSVar6->locate).coord.m[0] = uVar9;
//       *(undefined4 *)((pSVar6->locate).coord.m[0] + 2) = uVar10;
//       *(undefined4 *)((pSVar6->locate).coord.m[1] + 1) = uVar11;
//       pMVar3 = (ModelType *)((pMVar3->locate).coord.m + 2);
//       pSVar6 = (Sprite3D *)((pSVar6->locate).coord.m + 2);
//     } while (pMVar3 != (ModelType *)pSVar8);
//     DrawSprite(sprt);
//     uVar1 = (item->param).smoke.count;
//     (item->param).smoke.count = uVar1 - 1;
//     if (item->mode == '\0') {
//       if (uVar1 != '\x01') {
//         return;
//       }
//       SoundEx((VECTOR *)(item->locate->locate).coord.t,0x23);
//       (item->param).smoke.count = 'x';
//       item->mode = item->mode + '\x01';
//       return;
//     }
//     if (item->mode != '\x01') {
//       return;
//     }
//     if (uVar1 != '\x01') {
//       if ((uVar1 - 1 & 1) == 0) {
//         local_48 = DAT_80097ad8;
//         local_44 = DAT_80097adc;
//         memset((uchar *)&local_30,'\0',0x10);
//         local_40._0_4_ = (item->locate->locate).coord.t[0];
//         local_40._4_4_ = (item->locate->locate).coord.t[1];
//         local_38 = (item->locate->locate).coord.t[2];
//         local_34 = local_24;
//         local_30 = local_40._0_4_;
//         local_2c = local_40._4_4_;
//         local_28 = local_38;
//         SetSmoke((VECTOR *)local_40,(SVECTOR *)&local_48,1,3);
//       }
//       if ((GameClock & 0xfU) != 0) {
//         return;
//       }
//       pMVar3 = item->locate;
//       local_40._0_4_ = 0;
//       local_40._4_4_ = (pMVar3->locate).coord.t[0];
//       local_38 = (pMVar3->locate).coord.t[1];
//       local_34 = (pMVar3->locate).coord.t[2];
//       local_2c = 2000;
//       do {
//         ppHVar13 = HumanGroup + local_40._0_4_;
//         for (iVar12 = local_40._0_4_; pHVar7 = (Humanoid *)0x0, iVar12 < Humans; iVar12 = iVar12 + 1
//             ) {
//           pHVar7 = *ppHVar13;
//           if ((((0 < pHVar7->life) && (pHVar7->motion->mid != 0x100)) &&
//               ((pHVar7->attribute & 0x80U) == 0)) &&
//              (iVar4 = GetVectorDistance((VECTOR *)(local_40 + 4),pHVar7->locate), iVar4 < local_2c))
//           {
//             local_40._0_4_ = iVar12 + 1;
//             local_48 = pHVar7;
//             local_44 = iVar4;
//             break;
//           }
//           ppHVar13 = ppHVar13 + 1;
//         }
//         human = local_48;
//         if (pHVar7 == (Humanoid *)0x0) {
//           return;
//         }
//         if (((local_48 != item->owner) && (local_48->life != -1)) &&
//            (local_48->motion->mid != 0x100b)) {
//           if ((ActionHalt == 0) && (0 < local_48->life)) {
//             FUN_800270c8(local_48,3);
//             UpdateMotion(human->motion,0x100b);
//             human->status = 0x10;
//             pMVar5 = human->motion->motion;
//             MoveHumanoid(human,(ushort)pMVar5->orderspd,(ushort)pMVar5->sidespd);
//           }
//           Sound(local_48,6);
//         }
//       } while( true );
//     }
//     ppuVar2 = item->proc;
//     if (ppuVar2 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   (*(code *)ppuVar2)(item);
//   DeleteConflict(item->locate);
//   if (item->mode != 0) {
//     AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//   }
//   item->owner = (Humanoid *)0x0;
//   item->proc = (undefined **)0x0;
//   return;
// }
