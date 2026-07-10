#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * BreedLife(short type, long x, long y, long z, long r);
 *     APPEAR.C:202, 50 src lines, frame 160 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s4       short type
 *     param $s5       long x
 *     param $s7       long y
 *     param $s6       long z
 *     param stack+16  long r
 *     reg   $v0       long r
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       unsigned long * model
 *     reg   $s0       unsigned long idx
 *     stack sp+16     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/BreedLife", BreedLife);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/BreedLife", create_character__override__prt_8002a100_aee7b64a);

// triage: MEDIUM — 157 insns, 2 loop, 9 callees, ~0.05 to SetupAfterimage
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// Humanoid * BreedLife(short param_1,long param_2,long param_3,long param_4,short param_5)
//
// {
//   short sVar1;
//   int iVar2;
//   Humanoid *human;
//   long lVar3;
//   ModelArchiveType *pMVar4;
//   HumanDataType *pHVar5;
//   int iVar6;
//   ulong *puVar7;
//   char acStack_90 [104];
//
//   iVar6 = 0;
//   if (HumanData[0].type != -1) {
//     pHVar5 = HumanData;
//     sVar1 = HumanData[0].type;
//     do {
//       pHVar5 = pHVar5 + 1;
//       if (sVar1 == param_1) break;
//       sVar1 = pHVar5->type;
//       iVar6 = iVar6 + 1;
//     } while (sVar1 != -1);
//     if (HumanData[iVar6].type != -1) {
//       pHVar5 = HumanData;
//       puVar7 = HumanData[iVar6].model;
//       if (puVar7 == (ulong *)0x0) {
//         sprintf(acStack_90,"%s%s.MAD","K:\\WORK\\CDIMAGE\\HUMAN\\",HumanData[iVar6].name);
//         puVar7 = FileRead(acStack_90);
//         HumanData[iVar6].model = puVar7;
//         sVar1 = HumanData[0].type;
//         while (sVar1 != -1) {
//           iVar2 = strcmp((char *)HumanData[iVar6].name,(char *)pHVar5->name);
//           if (iVar2 == 0) {
//             pHVar5->model = puVar7;
//           }
//           pHVar5 = pHVar5 + 1;
//           sVar1 = pHVar5->type;
//         }
//       }
//       human = CreateHumanoid(param_1,puVar7);
//       puVar7 = GlobalAreaMap;
//       human->point[0] = param_2;
//       (human->model->locate).coord.t[0] = param_2;
//       lVar3 = GetAreaMapLevel(puVar7,param_2,param_3);
//       (human->model->locate).coord.t[1] = lVar3;
//       pMVar4 = human->model;
//       human->point[1] = param_4;
//       (pMVar4->locate).coord.t[2] = param_4;
//       (human->model->rotate).vy = param_5;
//       UpdateCoordinate((ModelType *)human->model);
//       if (param_1 == 0x87) {
//         human->item[3] = '\x01';
//       }
//       if (param_1 < 0x8b) {
//         if ((0x80 < param_1) || ((param_1 < 2 && (-1 < param_1)))) {
//           human->attribute = human->attribute | 2;
//           EquipWeapon(human,1);
//           SetNowMotion(human,0x501,1);
//         }
//       }
//       else if ((param_1 < 0xa8) && (0xa5 < param_1)) {
//         human->attribute = human->attribute | 0x20;
//       }
//       return human;
//     }
//   }
//                     /* WARNING: Subroutine does not return */
//   SystemOut((uchar *)"ILLIGAL CHARACTER TYPE");
// }
