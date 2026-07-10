#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void WeaponHitWeapon(struct ModelType *hand);
 *     MOTION.C:771, 25 src lines, frame 56 bytes, saved-reg mask 0x803f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s3       struct ModelType * hand
 *     reg   $s2       struct VECTOR * p
 *     reg   $s4       short id
 *     reg   $s0       short i
 *     stack sp+16     struct SVECTOR pv
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct BattleType BattleDB[78];
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/WeaponHitWeapon", WeaponHitWeapon);

// triage: MEDIUM — 161 insns, mul/div, 3 loop, 6 callees, ~0.06 to SetupMotionRegist
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void WeaponHitWeapon(ModelType *hand)
//
// {
//   short sVar1;
//   Humanoid *human;
//   ushort uVar2;
//   int iVar3;
//   int iVar4;
//   SVECTOR local_28;
//
//   if (((int)hand->attribute & 0x8000U) != 0) {
//     do {
//       uVar2 = GetConflictResult(hand,-1);
//       iVar4 = (int)(short)uVar2;
//       if (iVar4 < 0) {
//         return;
//       }
//     } while (((ConflictObject[iVar4].size.pad & 1U) == 0) ||
//             ((Humanoid *)ConflictObject[iVar4].common == Me_MOTION_C));
//     MoveHumanoid(Me_MOTION_C,-0x1e,0);
//     if ((Humanoid *)ConflictObject[iVar4].common != (Humanoid *)&DAT_00000001) {
//       MoveHumanoid((Humanoid *)ConflictObject[iVar4].common,-0x1e,0);
//     }
//     sVar1 = hand->id;
//     iVar4 = 0;
//     do {
//       iVar3 = rand();
//       local_28.vx = (short)iVar3 + (short)(iVar3 / 100) * -100 + -0x32;
//       iVar3 = rand();
//       local_28.vy = (short)iVar3 + (short)(iVar3 / 100) * -100 + -0x32;
//       iVar3 = rand();
//       local_28.vz = (short)iVar3 + (short)(iVar3 / 100) * -100 + -0x32;
//       iVar3 = rand();
//       SetBleed(&ConflictObject[sVar1].position,&local_28,iVar3 % 0x14 + 0x14,0x7fff);
//       iVar4 = iVar4 + 1;
//     } while (iVar4 * 0x10000 >> 0x10 < 10);
//     hand->attribute = hand->attribute & 0xbfff;
//     human = Me_MOTION_C;
//     sVar1 = *(short *)((int)&BattleDB[0].power + ((int)((uint)uVar2 << 0x10) >> 0xc));
//     dtM->loop = ((sVar1 >> 0xf) - (short)((ulonglong)((longlong)(int)sVar1 * 0x55555556) >> 0x20)) +
//                 -1;
//     Sound(human,0x36);
//     if (StagePlayer == Me_MOTION_C) {
//       PadShockAR(0,0xff,10,0);
//     }
//   }
//   return;
// }
