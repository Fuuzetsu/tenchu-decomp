#include "common.h"
#include "main.exe.h"

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_80056e30", FUN_80056e30);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_80056e30", FUN_80056e30__override__prt_80056e6c_aee7b64a);

// triage: EASY — 37 insns, 5 callees, ~0.14 to ChkCard
// likely-relevant cookbook sections:
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// int FUN_80056e30(undefined4 param_1)
//
// {
//   char acStack_e0 [200];
//   long lStack_18;
//   long local_14;
//   long lStack_10;
//   int local_c;
//
//   local_14 = MemCardAccept(0);
//   MemCardSync(0,&lStack_18,&local_14);
//   sprintf(acStack_e0,&DAT_80097d08,PTR_s_BISLPS_01901_80097d04,param_1);
//   local_c = MemCardOpen(0,acStack_e0,1);
//   MemCardSync(0,&lStack_10,&local_c);
//   if (local_c == 0) {
//     MemCardClose();
//   }
//   return (int)(short)local_c;
// }
