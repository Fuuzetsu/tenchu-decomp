#include "common.h"
#include "main.exe.h"

/*
 * FUN_8005a7a4 (0x8005a7a4) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=FUN_8005a7a4
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", FUN_8005a7a4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_1c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_1e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_21);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_28);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2b);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2d);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_32);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_33);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_1f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_37);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_3c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005a7a4", switchD_8005a7f4__caseD_3);

/* jump-table pool @ 0x80013d34 (63 words; tables at 0x80013d34) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 FUN_8005a7a4_jtbl[63] = {
    0x8005A7FC, 0x8005AAF0, 0x8005AB00, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005A80C, 0x8005AAF0,
    0x8005AB00, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005A81C, 0x8005AB0C, 0x8005A82C, 0x8005A9DC,
    0x8005A9DC, 0x8005A83C, 0x8005AB0C, 0x8005AB0C,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005A884, 0x8005A9DC, 0x8005A9DC, 0x8005A898,
    0x8005A95C, 0x8005A980, 0x8005A990, 0x8005A9CC,
    0x8005AB0C, 0x8005AB0C, 0x8005A9A0, 0x8005A9B0,
    0x8005A9CC, 0x8005A9DC, 0x8005A9DC, 0x8005A9F4,
    0x8005AB0C, 0x8005AB0C, 0x8005AB0C, 0x8005AB0C,
    0x8005AAE0, 0x8005AAF0, 0x8005AB00,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// undefined4 FUN_8005a7a4(undefined4 param_1)
// 
// {
//   ushort uVar1;
//   short sVar2;
//   ushort uVar3;
//   
//   FUN_8005adbc(0);
//   switch((int)((DAT_80097d2e - 10) * 0x10000) >> 0x10) {
//   case 0:
//     DAT_80097d30 = 0x18;
//     break;
//   case 1:
//   case 0xb:
//   case 0x3d:
//     DAT_80097d2e = 0xffff;
//     break;
//   default:
//     sVar2 = FUN_8005aba4(&DAT_80097d2e,&DAT_80097d30);
//     if (sVar2 == 0) {
//       DAT_80097d30 = 0;
//       DAT_80097d32 = 0;
//       FUN_8005adbc(1);
//       if ((short)DAT_80097d2e < 0) {
//         DAT_80097d2e = 3;
//         return 1;
//       }
//       DAT_80097d2e = 3;
//       return 0xffffffff;
//     }
//     break;
//   case 10:
//     DAT_80097d30 = 0x19;
//     break;
//   case 0x1c:
//     DAT_80097d2e = 0x28;
//     break;
//   case 0x1e:
//     DAT_80097d2e = 0x2b;
//     break;
//   case 0x21:
//     sVar2 = FUN_80056e30(PTR_s_TENCHU_80097d18);
//     if (sVar2 == 0) {
//       DAT_80097d2e = 0x3c;
//       break;
//     }
//     if (sVar2 == 5) {
//       DAT_80097d2e = 0x32;
//       break;
//     }
//   case 2:
//   case 0xc:
//   case 0x3e:
//     DAT_80097d2e = 0;
//     break;
//   case 0x28:
//     DAT_80097d30 = 7;
//     DAT_80097d32 = 0;
//   case 0x1f:
//   case 0x20:
//   case 0x29:
//   case 0x2a:
//   case 0x35:
//   case 0x36:
//     DAT_80097d2e = DAT_80097d2e + 1;
//     break;
//   case 0x2b:
//     SaveCard(0,PTR_s_TENCHU_80097d18,(undefined *)&PersistentState,0xe70);
//     sVar2 = SaveCard(0,PTR_s_TENCHU_80097d18,(undefined *)&PersistentState,0xe70);
//     if (sVar2 == 1) {
//       DAT_80097d2e = 10;
//     }
//     else if (sVar2 < 2) {
//       DAT_80097d2e = 0x38;
//       if (sVar2 == 0) {
//         DAT_80097d2e = 0x36;
//       }
//     }
//     else if (sVar2 == 4) {
//       DAT_80097d2e = 0x1e;
//       DAT_80097d2c = 0;
//     }
//     else {
//       DAT_80097d2e = 0x38;
//       if (sVar2 == 7) {
//         DAT_80097d2e = 0x46;
//       }
//     }
//     uVar3 = 0x35;
//     uVar1 = DAT_80097d2e;
//     sVar2 = DAT_80097d32;
//     goto joined_r0x8005aaa4;
//   case 0x2c:
//     if (PersistentState._92_1_ == '\0') {
//       DAT_80097d30 = 9;
//       break;
//     }
//     goto LAB_8005a984;
//   case 0x2d:
// LAB_8005a984:
//     DAT_80097d2e = 99;
//     break;
//   case 0x2e:
//     DAT_80097d30 = 0xe;
//     break;
//   case 0x2f:
//   case 0x34:
//     DAT_80097d2e = 0x5a;
//     break;
//   case 0x32:
//     DAT_80097d30 = 0x2c;
//     break;
//   case 0x33:
//     DAT_80097d30 = 7;
//     DAT_80097d2e = 0x3f;
//     DAT_80097d32 = 0;
//     break;
//   case 0x37:
//     SaveCard(0,PTR_s_TENCHU_80097d18,(undefined *)&PersistentState,0xe70);
//     sVar2 = SaveCard(0,PTR_s_TENCHU_80097d18,(undefined *)&PersistentState,0xe70);
//     if (sVar2 == 1) {
//       DAT_80097d2e = 10;
//     }
//     else if (sVar2 < 2) {
//       DAT_80097d2e = 0x38;
//       if (sVar2 == 0) {
//         DAT_80097d2e = 0x36;
//       }
//     }
//     else if (sVar2 == 4) {
//       DAT_80097d2e = 0x1e;
//       DAT_80097d2c = 0;
//     }
//     else {
//       DAT_80097d2e = 0x38;
//       if (sVar2 == 7) {
//         DAT_80097d2e = 0x46;
//       }
//     }
//     uVar3 = 0x41;
//     uVar1 = DAT_80097d2e;
//     sVar2 = DAT_80097d32;
// joined_r0x8005aaa4:
//     DAT_80097d2e = uVar1;
//     DAT_80097d32 = sVar2;
//     if ((uVar1 != 0x36) && (DAT_80097d32 = sVar2 + 1, DAT_80097d2e = uVar3, 2 < sVar2)) {
//       DAT_80097d2e = uVar1;
//     }
//     break;
//   case 0x3c:
//     DAT_80097d30 = 0x1a;
//   }
//   sVar2 = FUN_8005b17c((int)DAT_80097d30,param_1);
//   DAT_80097d2e = DAT_80097d2e + sVar2;
//   return 0;
// }

#endif /* NON_MATCHING */
