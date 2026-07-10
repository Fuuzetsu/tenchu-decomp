#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void KillHumanoid(struct Humanoid *human);
 *     HUMAN.C:78, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/KillHumanoid", KillHumanoid);

// triage: EASY — 63 insns, 1 loop, 6 callees, ~0.15 to DisposeBG
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void KillHumanoid(Humanoid *human)
//
// {
//   ushort uVar1;
//   int iVar2;
//   short sVar3;
//   int iVar4;
//
//   if (human != (Humanoid *)0x0) {
//     DeleteConflict(*human->model->object);
//     DisposeModelArchive(human->model);
//     DisposeMotionManager(human->motion);
//     DisposeWeapon(human);
//     FUN_800270c8(human,3);
//     vfree((undefined *)human);
//     iVar4 = 0;
//     if (0 < Humans) {
//       iVar2 = 0;
//       do {
//         sVar3 = (short)iVar4;
//         iVar4 = iVar4 + 1;
//         if (*(Humanoid **)((int)HumanGroup + (iVar2 >> 0xe)) == human) break;
//         iVar2 = iVar4 * 0x10000;
//         sVar3 = (short)iVar4;
//       } while (iVar4 * 0x10000 >> 0x10 < (int)Humans);
//       uVar1 = Humans - 1;
//       if ((int)sVar3 < (int)Humans) {
//         Humans = uVar1;
//         HumanGroup[sVar3] = *(Humanoid **)((int)HumanGroup + ((int)((uint)uVar1 << 0x10) >> 0xe));
//       }
//     }
//   }
//   return;
// }
