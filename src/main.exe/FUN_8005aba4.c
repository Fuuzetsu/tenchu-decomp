#include "common.h"
#include "main.exe.h"

/*
 * FUN_8005aba4 (0x8005aba4) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=FUN_8005aba4
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", FUN_8005aba4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_14);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_1e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_25);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_1f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_20);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_23);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_24);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_26);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_5a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_b);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FUN_8005aba4", switchD_8005abf0__caseD_5);

/* jump-table pool @ 0x80013e34 (93 words; tables at 0x80013e34) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 FUN_8005aba4_jtbl[93] = {
    0x8005ABF8, 0x8005AD0C, 0x8005AD0C, 0x8005AC00,
    0x8005AC0C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005ACAC, 0x8005AD84,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005ACB4, 0x8005AD84, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005ACBC, 0x8005ACF4,
    0x8005AD04, 0x8005AD0C, 0x8005AD0C, 0x8005AD14,
    0x8005AD6C, 0x8005ACEC, 0x8005AD74, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD8C, 0x8005AD8C,
    0x8005AD8C, 0x8005AD8C, 0x8005AD7C, 0x8005AD84,
    0x8005ACEC,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// undefined4 FUN_8005aba4(ushort *param_1,undefined2 *param_2)
// 
// {
//   bool bVar1;
//   short sVar2;
//   int iVar3;
//   ushort uVar4;
//   undefined2 uVar5;
//   long lStack_20;
//   int local_1c;
//   
//   uVar4 = *param_1;
//   uVar5 = *param_2;
//   switch(uVar4) {
//   case 0:
//     uVar5 = 0x10;
//   case 1:
//   case 2:
//   case 0x21:
//   case 0x22:
//     uVar4 = uVar4 + 1;
//     break;
//   case 3:
//     DAT_80097d34 = 0;
//     uVar4 = 4;
//     break;
//   case 4:
//     sVar2 = ChkCard();
//     if (sVar2 == 2) {
//       uVar4 = 0x14;
// LAB_8005ac6c:
//       iVar3 = (uint)uVar4 << 0x10;
//     }
//     else if (sVar2 < 3) {
//       uVar4 = 0x28;
//       if (sVar2 == 1) {
//         uVar4 = 10;
//         goto LAB_8005ac6c;
//       }
//       iVar3 = 0x280000;
//     }
//     else {
//       uVar4 = 0x28;
//       if (sVar2 == 4) {
//         DAT_80097d2c = 0;
//         uVar4 = 0x1e;
//         goto LAB_8005ac6c;
//       }
//       iVar3 = 0x280000;
//     }
//     if ((iVar3 >> 0x10 != 0x28) &&
//        (sVar2 = DAT_80097d34 + 1, bVar1 = DAT_80097d34 < 3, DAT_80097d34 = sVar2, bVar1)) {
//       uVar4 = 4;
//     }
//     break;
//   default:
//     return 0;
//   case 10:
//     uVar5 = 0x12;
//     break;
//   case 0xb:
//   case 0x15:
//   case 0x5b:
//     uVar4 = 0xffff;
//     break;
//   case 0x14:
//     uVar5 = 2;
//     break;
//   case 0x1e:
//     local_1c = MemCardExist(0);
//     MemCardSync(0,&lStack_20,&local_1c);
//     uVar5 = 3;
//     if (local_1c == 0) break;
//     uVar5 = 0;
//   case 0x25:
//   case 0x5c:
//     uVar4 = 0;
//     break;
//   case 0x1f:
//     uVar5 = 10;
//     DAT_80097d34 = 0;
//     uVar4 = 0x21;
//     break;
//   case 0x20:
//     uVar4 = 0x5a;
//     break;
//   case 0x23:
//     uVar4 = 0x24;
//     sVar2 = FormatCard();
//     iVar3 = 0x240000;
//     if (sVar2 == 0) {
//       uVar4 = 0x26;
//       iVar3 = 0x260000;
//     }
//     if ((iVar3 >> 0x10 != 0x26) &&
//        (sVar2 = DAT_80097d34 + 1, bVar1 = DAT_80097d34 < 3, DAT_80097d34 = sVar2, bVar1)) {
//       uVar4 = 0x23;
//     }
//     break;
//   case 0x24:
//     uVar5 = 0xc;
//     break;
//   case 0x26:
//     uVar5 = 0xb;
//     break;
//   case 0x5a:
//     uVar5 = 0x14;
//   }
//   *param_1 = uVar4;
//   *param_2 = uVar5;
//   return 0xffffffff;
// }

#endif /* NON_MATCHING */
