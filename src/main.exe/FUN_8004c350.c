#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsSPRITE sprFrame[4];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8004c350", FUN_8004c350);

// triage: MEDIUM — 147 insns, mul/div, 5 callees, ~0.11 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8004c350(int param_1,uint param_2)
//
// {
//   uchar uVar1;
//   long lVar2;
//   int iVar3;
//   int iVar4;
//   SVECTOR local_50 [2];
//   VECTOR local_40;
//   VECTOR local_30;
//   long local_20;
//   long local_1c;
//   long local_18;
//   long local_14;
//
//   local_50[0]._0_4_ = DAT_80097c50;
//   local_50[0]._4_4_ = DAT_80097c54;
//   lVar2 = GameClock;
//   if (GameClock < 0) {
//     lVar2 = GameClock + 3;
//   }
//   iVar3 = GameClock + (lVar2 >> 2) * -4;
//   if (param_2 == 0) {
//     *(undefined1 *)(param_1 + 0x15) = 0;
//   }
//   else if ((3 < param_2) && (*(char *)(param_1 + 0x15) == '\0')) {
//     iVar4 = rand();
//     uVar1 = (char)iVar4 + (char)(iVar4 / 100) * -100 + 'd';
//     sprFrame[iVar3].b = uVar1;
//     sprFrame[iVar3].g = uVar1;
//     sprFrame[iVar3].r = uVar1;
//     FUN_8003a2a8(sprFrame + iVar3,*(undefined4 *)(param_1 + 4),*(undefined4 *)(param_1 + 8),
//                  *(undefined4 *)(param_1 + 0xc),*(undefined4 *)(param_1 + 0x18));
//     if ((GameClock & 0xfU) == 0) {
//       memset((uchar *)&local_30,'\0',0x10);
//       local_40.vx = *(long *)(param_1 + 4);
//       local_40.vy = *(long *)(param_1 + 8);
//       local_40.vz = *(long *)(param_1 + 0xc);
//       local_40.pad = local_30.pad;
//       local_30.vx = local_40.vx;
//       local_30.vy = local_40.vy;
//       local_30.vz = local_40.vz;
//       SetBleedsDir(&local_40,local_50,100,10);
//     }
//     if (GameClock == (GameClock / 0x4f) * 0x4f) {
//       memset((uchar *)&local_20,'\0',0x10);
//       local_30.vx = *(long *)(param_1 + 4);
//       local_30.vy = *(long *)(param_1 + 8);
//       local_30.vz = *(long *)(param_1 + 0xc);
//       local_30.pad = local_14;
//       local_20 = local_30.vx;
//       local_1c = local_30.vy;
//       local_18 = local_30.vz;
//       SoundEx(&local_30,0x47);
//     }
//   }
//   return;
// }
