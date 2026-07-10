#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005fe34", FUN_8005fe34);

// triage: TRIVIAL — 21 insns, 1 callees, ~0.05 to AdtFntOpen
// likely-relevant cookbook sections:
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005fe34(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)
//
// {
//   int iVar1;
//   undefined4 param_12;
//   undefined4 param_13;
//   undefined4 param_14;
//   undefined4 param_15;
//
//   param_12 = param_1;
//   param_13 = param_2;
//   param_14 = param_3;
//   param_15 = param_4;
//   iVar1 = AdtVsprintf(&stack0x0000007c,PTR_DAT_80097e98,(int)&DAT_800c3eb0 - (int)PTR_DAT_80097e98,
//                       param_1);
//   PTR_DAT_80097e98 = PTR_DAT_80097e98 + iVar1;
//   return;
// }
