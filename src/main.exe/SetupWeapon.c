#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupWeapon(struct Humanoid *human);
 *     APPEAR.C:299, 61 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s0       struct Humanoid * human
 *     reg   $a1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 * END PSX.SYM */

/*
 * SetupWeapon (0x8002a484) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=SetupWeapon
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", SetupWeapon);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_12);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_9);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_14);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_2a);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_16);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_c);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_1f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_30);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_32);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupWeapon", switchD_8002a574__caseD_6);

/* jump-table pool @ 0x80011818 (53 words; tables at 0x80011818) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 SetupWeapon_jtbl[53] = {
    0x8002A57C, 0x8002A57C, 0x8002A598, 0x8002A5D8,
    0x8002A5B4, 0x8002A8DC, 0x8002A5B4, 0x8002A8DC,
    0x8002A664, 0x8002A610, 0x8002A8DC, 0x8002A7E0,
    0x8002A8DC, 0x8002A8DC, 0x8002A8DC, 0x8002A664,
    0x8002A664, 0x8002A648, 0x8002A7E0, 0x8002A67C,
    0x8002A8DC, 0x8002A700, 0x8002A8DC, 0x8002A8DC,
    0x8002A700, 0x8002A8DC, 0x8002A8DC, 0x8002A700,
    0x8002A8DC, 0x8002A8DC, 0x8002A7FC, 0x8002A610,
    0x8002A8DC, 0x8002A664, 0x8002A664, 0x8002A664,
    0x8002A664, 0x8002A664, 0x8002A664, 0x8002A664,
    0x8002A664, 0x8002A6B0, 0x8002A8DC, 0x8002A8DC,
    0x8002A8DC, 0x8002A8DC, 0x8002A8DC, 0x8002A84C,
    0x8002A84C, 0x8002A864, 0x8002A8DC, 0x8002A8DC,
    0x8002A864,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void SetupWeapon(Humanoid *human)
// 
// {
//   int iVar1;
//   short body;
//   int iVar2;
//   short sVar3;
//   short sVar4;
//   
//   human->wepid[1] = -1;
//   human->wepid[0] = -1;
//   iVar2 = 0;
//   do {
//     iVar1 = iVar2 + 1;
//     *(undefined4 *)((int)human->weapon + ((iVar2 << 0x10) >> 0xe)) = 0;
//     iVar2 = iVar1;
//   } while (iVar1 * 0x10000 >> 0x10 < 4);
//   iVar2 = 0;
//   sVar4 = 0;
//   sVar3 = HumanData[0].type;
//   while (sVar3 != human->type) {
//     iVar2 = iVar2 + 1;
//     sVar4 = (short)iVar2;
//     sVar3 = HumanData[iVar2 * 0x10000 >> 0x10].type;
//   }
//   sVar3 = HumanData[sVar4].wepid;
//   human->wpatk = sVar3;
//   switch(sVar3) {
//   case 1:
//   case 2:
//     GetWeaponData(human,0,human->wpatk,1);
//   case 3:
//     body = 0;
//     sVar4 = 0;
//     sVar3 = human->wpatk;
//     break;
//   case 5:
//   case 7:
//     GetWeaponData(human,0,human->wpatk + 1,-1);
//   case 4:
//     GetWeaponData(human,0xd,human->wpatk,0);
//     body = 0xe;
//     sVar4 = 1;
//     sVar3 = human->wpatk;
//     break;
//   default:
//     goto switchD_8002a574_caseD_6;
//   case 10:
//   case 0x20:
//     GetWeaponData(human,0xd,human->wpatk,0);
//     sVar4 = 1;
//     sVar3 = human->wpatk;
//     goto LAB_8002a8cc;
//   case 0x12:
//     GetWeaponData(human,0xe,1,1);
//   case 9:
//   case 0x10:
//   case 0x11:
//   case 0x22:
//   case 0x23:
//   case 0x24:
//   case 0x25:
//   case 0x26:
//   case 0x27:
//   case 0x28:
//   case 0x29:
//     body = 0xd;
//     sVar3 = human->wpatk;
//     sVar4 = 0;
//     break;
//   case 0x14:
//     GetWeaponData(human,0xd,0x14,0);
//     body = 1;
//     sVar3 = 0x15;
//     sVar4 = -1;
//     break;
//   case 0x16:
//   case 0x19:
//   case 0x1c:
//     GetWeaponData(human,0,human->wpatk + 1,-1);
//     GetWeaponData(human,0,human->wpatk + 2,-1);
//     (human->weapon[0]->locate).coord.t[0] = ((int)human->width / 3) * 0x10000 >> 0x10;
//     (human->weapon[0]->locate).coord.t[1] = 0;
//     (human->weapon[0]->locate).coord.t[2] = 0;
//     (human->weapon[1]->locate).coord.t[0] = ((int)human->width / 3) * 0x10000 >> 0x10;
//     (human->weapon[1]->locate).coord.t[1] = 0;
//     (human->weapon[1]->locate).coord.t[2] = 0;
//   case 0xc:
//   case 0x13:
//     body = 0xd;
//     sVar4 = 0;
//     sVar3 = human->wpatk;
//     break;
//   case 0x1f:
//     GetWeaponData(human,0xd,0x1f,0);
//     GetWeaponData(human,0xe,0x1f,1);
//     body = 0;
//     sVar3 = 0x2d;
//     sVar4 = -1;
//     break;
//   case 0x2a:
//     GetWeaponData(human,0xd,0x2a,0);
//     GetWeaponData(human,0xe,0x2b,-1);
//     body = 0xe;
//     sVar3 = 0x2c;
//     sVar4 = -1;
//     break;
//   case 0x30:
//   case 0x31:
//     body = 0xd;
//     sVar3 = human->wpatk;
//     sVar4 = -1;
//     break;
//   case 0x32:
//   case 0x35:
//     sVar3 = 0;
//     if (human->wpatk != 0x35) {
//       sVar3 = -1;
//     }
//     GetWeaponData(human,0xd,human->wpatk,sVar3);
//     GetWeaponData(human,1,human->wpatk + 2,-1);
//     sVar4 = -1;
//     sVar3 = human->wpatk;
// LAB_8002a8cc:
//     sVar3 = sVar3 + 1;
//     body = 0xe;
//   }
//   GetWeaponData(human,body,sVar3,sVar4);
// switchD_8002a574_caseD_6:
//   return;
// }

#endif /* NON_MATCHING */
