#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8001c730", FUN_8001c730);

// triage: VERY-HARD — 55 insns, 1 GTE CMD — UN-SPLITTABLE (as can't assemble), mul/div, 0 callees, ~0.01 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8001c730(short *param_1,undefined4 *param_2,SVECTOR *param_3)
//
// {
//   short sVar1;
//   short sVar2;
//   ushort uVar3;
//   undefined2 *puVar4;
//   int iVar5;
//   undefined2 *puVar6;
//   undefined4 uVar7;
//
//   gte_ldv0(param_3);
//   puVar6 = (undefined2 *)param_2[1];
//   puVar4 = (undefined2 *)*param_2;
//   gte_ldR11R12(CONCAT22(*puVar6,*puVar4));
//   gte_ldR13R21(CONCAT22(puVar4[1],*(undefined2 *)(param_2 + 2)));
//   gte_ldR22R23(CONCAT22(*(undefined2 *)((int)param_2 + 10),puVar6[1]));
//   uVar3 = puVar4[2];
//   uVar7 = param_2[3];
//   gte_ldR31R32(CONCAT22(puVar6[2],uVar3));
//   gte_ldR33(uVar7);
//   iVar5 = (int)param_3->pad;
//   gte_rtv0_b();
//   sVar1 = *(short *)((int)param_2 + 0x12);
//   sVar2 = *(short *)(param_2 + 5);
//   read_mt(CONCAT22(puVar6[2],uVar3),(uint)uVar3,uVar7);
//   *param_1 = uVar3 + (short)((uint)(iVar5 * *(short *)(param_2 + 4)) >> 0xc);
//   param_1[1] = uVar3 + (short)((uint)(iVar5 * sVar1) >> 0xc);
//   param_1[2] = (short)uVar7 + (short)((uint)(iVar5 * sVar2) >> 0xc);
//   return;
// }
