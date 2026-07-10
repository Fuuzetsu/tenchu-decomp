#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leLayoutEnemy(int mode);
 *     WORLD.C:1197, 58 src lines, frame 104 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s5       int mode
 *     reg   $s3       int i
 *     reg   $s1       struct Humanoid * target
 *     stack sp+24     struct VECTOR pos
 *     reg   $s0       struct TraceLine * t
 *     reg   $s0       struct TEnemyLayout * en
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct TEnemyLayout * en
 *     reg   $a1       struct TracePoint * tp
 *     reg   $a0       int i
 *     stack sp+40     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern enum TSystemFlag SystemFlag;
 *     extern struct TEnemyLayout enemy[30];
 *     extern long GameClock;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/leLayoutEnemy", leLayoutEnemy);

// triage: MEDIUM — 210 insns, 1 loop, 9 callees, ~0.06 to ProcItemTeleport
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void leLayoutEnemy(int mode)
//
// {
//   Humanoid *pHVar1;
//   int iVar2;
//   TracePoint *point;
//   long lVar3;
//   TracePoint *pTVar4;
//   ModelType *pMVar5;
//   short *psVar6;
//   TraceLine *pt;
//   int iVar7;
//   int iVar8;
//   long local_40;
//   int local_3c;
//   long local_38;
//   undefined4 local_34;
//   long local_30;
//   int local_2c;
//   long local_28;
//   undefined4 local_24;
//
//   FUN_80039c14();
//   while (pHVar1 = HumanGroup[1], iVar8 = 0, 1 < Humans) {
//     if ((SystemFlag & 2) != 0) {
//       memset((uchar *)&local_30,'\0',0x10);
//       local_40 = (pHVar1->model->locate).coord.t[0];
//       local_3c = (pHVar1->model->locate).coord.t[1] + -0x4b0;
//       local_38 = (pHVar1->model->locate).coord.t[2];
//       local_34 = local_24;
//       local_30 = local_40;
//       local_2c = local_3c;
//       local_28 = local_38;
//       SetBleeds((short)&local_40,600,0x14);
//     }
//     pt = pHVar1->trace;
//     if (pt != (TraceLine *)0x0) {
//       vfree((undefined *)pt->point);
//       vfree((undefined *)pt);
//     }
//     KillHumanoid(pHVar1);
//   }
//   iVar7 = 0;
//   for (; iVar8 < 0x1e; iVar8 = iVar8 + 1) {
//     psVar6 = (short *)((int)enemy[0].path + iVar7 + -0x18);
//     if (*psVar6 != -1) {
//       pHVar1 = (Humanoid *)
//                BreedLife((int)*psVar6,*(undefined4 *)((int)(enemy[0].path + -1) + iVar7),
//                          *(undefined4 *)((int)enemy[0].path + iVar7 + -0xc),
//                          *(undefined4 *)((int)enemy[0].path + iVar7 + -8),0);
//       (pHVar1->model->rotate).vy = *(short *)((int)enemy[0].path + iVar7 + -4);
//       pMVar5 = (ModelType *)(CamState.Owner)->model;
//       pHVar1->attribute = pHVar1->attribute | 0x80;
//       pHVar1->target = pMVar5;
//       pMVar5 = *pHVar1->model->object;
//       pMVar5->attribute = pMVar5->attribute & 0xbfff;
//       if (mode == 1) {
//         SetupThinkFunction(pHVar1,*(short *)((int)enemy[0].path + iVar7 + -0x16));
//         iVar2 = (int)*(short *)((int)enemy[0].path + iVar7 + -0x14);
//         if (iVar2 != 0) {
//           point = (TracePoint *)valloc((iVar2 + 1) * 0xc);
//           iVar2 = 0;
//           pTVar4 = point;
//           if (0 < *(short *)((int)enemy[0].path + iVar7 + -0x14)) {
//             do {
//               iVar2 = iVar2 + 1;
//               pTVar4->x = *(long *)(psVar6 + 0xc);
//               lVar3 = *(long *)(psVar6 + 0x10);
//               psVar6 = psVar6 + 8;
//               pTVar4->range = 0x5dc;
//               pTVar4->pad = 0;
//               pTVar4->z = lVar3;
//               pTVar4 = pTVar4 + 1;
//             } while (iVar2 < *(short *)((int)enemy[0].path + iVar7 + -0x14));
//           }
//           point[iVar2].pad = -1;
//           SetupTraceLine(pHVar1,point);
//         }
//         if (pHVar1->trace != (TraceLine *)0x0) {
//           pHVar1->attribute = pHVar1->attribute | 8;
//         }
//       }
//       if ((SystemFlag & 2) != 0) {
//         memset((uchar *)&local_30,'\0',0x10);
//         local_40 = (pHVar1->model->locate).coord.t[0];
//         local_3c = (pHVar1->model->locate).coord.t[1] + -0x4b0;
//         local_38 = (pHVar1->model->locate).coord.t[2];
//         local_34 = local_24;
//         local_30 = local_40;
//         local_2c = local_3c;
//         local_28 = local_38;
//         SetBleeds((short)&local_40,400,0);
//       }
//     }
//     iVar7 = iVar7 + 0x88;
//   }
//   DAT_800979c0 = 0;
//   GameClock = 0;
//   return;
// }
