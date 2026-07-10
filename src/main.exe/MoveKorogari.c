#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MoveKorogari(struct tag_TItem *item, struct param_korogari *param);
 *     ITEM.C:663, 63 src lines, frame 64 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s1       struct tag_TItem * item
 *     param $s2       struct param_korogari * param
 *     stack sp+24     struct MapVector mv
 *     stack sp+40     struct SVECTOR vec
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern short RefrectMove[16][2];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/MoveKorogari", MoveKorogari);

// triage: HARD — 371 insns, mul/div, 5 callees, ~0.06 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void MoveKorogari(tag_TItem *item,param_korogari *param)
//
// {
//   short sVar1;
//   ModelType *pMVar2;
//   long lVar3;
//   int iVar4;
//   int iVar5;
//   VECTOR local_30;
//   undefined4 local_18;
//   undefined4 local_14;
//
//   if (param->status == '\x04') {
//     return;
//   }
//   (item->locate->locate).coord.t[0] = (item->locate->locate).coord.t[0] + (int)param->vx;
//   (item->locate->locate).coord.t[1] = (item->locate->locate).coord.t[1] + (int)param->vy;
//   (item->locate->locate).coord.t[2] = (item->locate->locate).coord.t[2] + (int)param->vz;
//   pMVar2 = item->locate;
//   lVar3 = CGetLevel(&param->hint,(pMVar2->locate).coord.t[0],(pMVar2->locate).coord.t[1],
//                     (pMVar2->locate).coord.t[2]);
//   pMVar2 = item->locate;
//   if (lVar3 < (pMVar2->locate).coord.t[1]) {
//     (pMVar2->locate).coord.t[0] = (pMVar2->locate).coord.t[0] - (int)param->vx;
//     (item->locate->locate).coord.t[1] = (item->locate->locate).coord.t[1] - (int)param->vy;
//     (item->locate->locate).coord.t[2] = (item->locate->locate).coord.t[2] - (int)param->vz;
//     GetAreaMapVector((MapVector *)GlobalAreaMap,&local_30,(long)(item->locate->locate).coord.t);
//     if (param->hint == (AreaNodeType *)0x0) {
//       pMVar2 = item->locate;
//       lVar3 = CGetLevel(&param->hint,(pMVar2->locate).coord.t[0],(pMVar2->locate).coord.t[1],
//                         (pMVar2->locate).coord.t[2]);
//       if (lVar3 == -0x80000000) {
//         if (param->status == '\x05') {
//           iVar4 = rand();
//           param->vx = (short)iVar4 + (short)(iVar4 / 0x640) * -0x640 + -800;
//           iVar4 = rand();
//           param->vy = (short)iVar4 + (short)(iVar4 / 0x640) * -0x640 + -800;
//           iVar4 = rand();
//           param->vz = (short)iVar4 + (short)(iVar4 / 0x640) * -0x640 + -800;
//         }
//         else {
//           param->vx = 0;
//           param->vy = 0xfa;
//           param->vz = 0;
//         }
//         param->status = '\x05';
//         return;
//       }
//     }
//     if (((byte)local_30.pad != 0) && (500 < local_30.vy)) {
//       iVar4 = abs((int)param->vx);
//       sVar1 = RefrectMove[0][(uint)(byte)local_30.pad * 2];
//       if (iVar4 < 0) {
//         iVar4 = iVar4 + 3;
//       }
//       iVar5 = (uint)(ushort)param->vy << 0x10;
//       param->vy = (short)((iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1);
//       param->vx = sVar1 * (short)(iVar4 >> 2);
//       iVar4 = abs((int)param->vz);
//       sVar1 = RefrectMove[0][(uint)(byte)local_30.pad * 2 + 1];
//       if (iVar4 < 0) {
//         iVar4 = iVar4 + 3;
//       }
//       param->status = '\x03';
//       param->vz = sVar1 * (short)(iVar4 >> 2);
//       return;
//     }
//     if (((ushort)local_30.vz & 4) == 0) {
//       iVar4 = (uint)(ushort)param->vx << 0x10;
//       iVar5 = (uint)(ushort)param->vz << 0x10;
//       param->vx = (short)((iVar4 >> 0x10) - (iVar4 >> 0x1f) >> 1);
//       param->vz = (short)((iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1);
//       if ((local_30.vx == -0x80000000) || (0x5dc < local_30.vy)) {
//         iVar4 = abs((int)param->vy);
//         iVar5 = rand();
//         param->status = '\x03';
//         param->vy = (short)(iVar4 / 2) + 0x19 + (short)iVar5 + (short)(iVar5 / 0x19) * -0x19;
//         return;
//       }
//       (item->locate->locate).coord.t[1] = local_30.vx;
//       if (param->vy < 0x2e) {
//         param->status = '\x04';
//         return;
//       }
//       iVar4 = rand();
//       param->vx = param->vx +
//                   RefrectMove[0][(uint)(byte)local_30.pad * 2] *
//                   ((short)iVar4 + (short)(iVar4 / 0x19) * -0x19 + 0x19);
//       iVar4 = rand();
//       sVar1 = RefrectMove[0][(uint)(byte)local_30.pad * 2 + 1];
//       param->status = '\x02';
//       param->vz = param->vz + sVar1 * ((short)iVar4 + (short)(iVar4 / 0x19) * -0x19 + 0x19);
//       iVar4 = abs((int)param->vy);
//       sVar1 = (short)(-iVar4 / 2);
//     }
//     else {
//       iVar4 = rand();
//       param->vx = (short)iVar4 + (short)(iVar4 / 0x14) * -0x14 + -10;
//       iVar4 = rand();
//       param->vz = (short)iVar4 + (short)(iVar4 / 0x14) * -0x14 + -10;
//       if (0x14 < param->vy) {
//         local_18 = DAT_80097ad0;
//         local_14 = DAT_80097ad4;
//         SetSplash((VECTOR *)(item->locate->locate).coord.t,0x2000,0x2000,4);
//         param->status = '\x01';
//       }
//       iVar4 = -(int)param->vy;
//       if (iVar4 < 0) {
//         iVar4 = iVar4 + 7;
//       }
//       sVar1 = (short)(iVar4 >> 3);
//     }
//   }
//   else {
//     param->status = '\0';
//     sVar1 = param->vy + 0xf;
//   }
//   param->vy = sVar1;
//   return;
// }
