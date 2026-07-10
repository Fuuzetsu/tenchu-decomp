#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005778c", FUN_8005778c);

// triage: EASY — 97 insns, 4 callees, ~0.12 to FUN_80038c0c
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005778c(void *param_1,short param_2,short param_3,uint param_4)
//
// {
//   bool bVar1;
//   ushort uVar2;
//   POLY_GT4 *ply;
//   short sVar3;
//   uint uVar4;
//   GsIMAGE local_38;
//
//   ply = (POLY_GT4 *)GsGetWorkBase();
//   GsSetWorkBase(ply + 1);
//   param_4 = param_4 & 0xff;
//   uVar4 = param_4;
//   if (param_4 == 0x92) {
//     uVar4 = 0x27;
//   }
//   bVar1 = 0xbf < uVar4;
//   if (0x1f < uVar4) {
//     uVar4 = uVar4 - 0x20;
//   }
//   if (bVar1) {
//     uVar4 = uVar4 - 0x40;
//   }
//   uVar2 = (ushort)uVar4;
//   local_38.pmode = DAT_800c2d38;
//   local_38.pixel = DAT_800c2d44;
//   local_38.cx = (short)DAT_800c2d48;
//   local_38.cy = DAT_800c2d48._2_2_;
//   local_38.cw = (ushort)DAT_800c2d4c;
//   local_38.ch = DAT_800c2d4c._2_2_;
//   local_38.clut = DAT_800c2d50;
//   local_38.px = (short)DAT_800c2d3c;
//   local_38.py = (short)((uint)DAT_800c2d3c >> 0x10);
//   if ((int)uVar4 < 0) {
//     uVar4 = uVar4 + 0xf;
//   }
//   local_38.pw = 3;
//   local_38.ph = 0x10;
//   local_38.py = local_38.py + (short)((int)uVar4 >> 4) * 0x10;
//   local_38.px = local_38.px + (uVar2 & 0xf) * 3;
//   if ((0x1f < param_4 - 0xc0) || (sVar3 = -4, param_4 == 199)) {
//     if (param_4 - 0xe0 < 0x20) {
//       sVar3 = -2;
//       if (param_4 == 0xe7) {
//         sVar3 = 3;
//       }
//     }
//     else {
//       sVar3 = 0;
//     }
//   }
//   SetupImageToPolyGT4(&local_38,ply,param_2,param_3 + sVar3);
//   AddPrim(param_1,ply);
//   return;
// }
