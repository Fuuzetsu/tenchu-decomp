#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetWeaponData(struct Humanoid *human, short body, short wid, short wpid, int wep);
 *     APPEAR.C:270, 25 src lines, frame 144 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       struct Humanoid * human
 *     param $s3       short body
 *     param $t1       short wid
 *     param $a3       short wpid
 *     param stack+16  int wep
 *     reg   $s2       short wep
 *     reg   $a0       short i
 *     stack sp+16     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct WeaponType WeaponDB[28];
 *     extern struct WeaponModelType WeaponModel[41];
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/GetWeaponData", GetWeaponData);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/GetWeaponData", load_weapon_model__override__prt_8002a418_aee7b64a);

// triage: MEDIUM — 125 insns, 2 loop, 4 callees, ~0.09 to SetupMotionRegist
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void GetWeaponData(Humanoid *human,short body,short wid,short wpid)
//
// {
//   int iVar1;
//   int iVar2;
//   ulong *puVar3;
//   OrnamentType *base;
//   int iVar4;
//   ushort uVar5;
//   ushort in_stack_00000010;
//   char acStack_80 [104];
//
//   if (-1 < wpid) {
//     uVar5 = 0;
//     if (WeaponDB[0].ilup1.pad != -1) {
//       iVar1 = 0;
//       do {
//         if (WeaponDB[iVar1 >> 0x10].ilup1.pad == wid) {
//           human->wepid[wpid] = uVar5;
//           break;
//         }
//         uVar5 = uVar5 + 1;
//         iVar1 = (uint)uVar5 << 0x10;
//       } while (WeaponDB[(short)uVar5].ilup1.pad != -1);
//     }
//   }
//   if ((-1 < (int)((uint)in_stack_00000010 << 0x10)) && (iVar1 = 0, WeaponModel[0].wid != -1)) {
//     iVar2 = 0;
//     do {
//       iVar4 = iVar1 << 0x10;
//       if (WeaponModel[iVar2 >> 0x10].wid == wid) goto LAB_8002a3d4;
//       iVar1 = iVar1 + 1;
//       iVar2 = iVar1 * 0x10000;
//     } while (WeaponModel[iVar1 * 0x10000 >> 0x10].wid != -1);
//     iVar4 = iVar1 * 0x10000;
// LAB_8002a3d4:
//     iVar4 = iVar4 >> 0x10;
//     if (WeaponModel[iVar4].wid != -1) {
//       if (WeaponModel[iVar4].model == (ulong *)0x0) {
//         sprintf(acStack_80,"%s%s.TMD","K:\\WORK\\CDIMAGE\\HUMAN\\WEAPON\\",WeaponModel[iVar4].name);
//         puVar3 = FileRead(acStack_80);
//         WeaponModel[iVar4].model = puVar3;
//       }
//       base = LoadOrnament(WeaponModel[iVar4].model);
//       *(OrnamentType **)((int)human->weapon + ((int)((uint)in_stack_00000010 << 0x10) >> 0xe)) =
//            base;
//       GsInitCoordinate2(*(GsCOORDINATE2 **)
//                          (((int)((uint)(ushort)body << 0x10) >> 0xe) + (int)human->model->object),
//                         &base->locate);
//     }
//   }
//   return;
// }
