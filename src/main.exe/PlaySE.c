#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short PlaySE(struct SoundEffect *se, short pt, long dv);
 *     AUDIO.C:72, 18 src lines, frame 40 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct SoundEffect * se
 *     param $a1       short pt
 *     param $a2       long dv
 *     reg   $s0       short d
 *     reg   $v1       short v
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PlaySE", PlaySE);

// triage: EASY — 71 insns, mul/div, 2 callees, ~0.04 to AttackFire
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short PlaySE(SoundEffect *se,short pt,long dv)
//
// {
//   ushort uVar1;
//   uint uVar2;
//   uint uVar3;
//   int iVar4;
//   short voll;
//
//   if (se != (SoundEffect *)0x0) {
//     uVar3 = dv >> 8;
//     uVar2 = (uint)(short)((uint)dv >> 8);
//     if ((int)uVar2 < 1) {
//       if ((int)uVar2 < 0) {
//         if ((int)uVar2 < 0) {
//           uVar2 = -uVar2;
//         }
//         uVar3 = (uVar2 & 0x3ff) >> 4;
//       }
//     }
//     else {
//       uVar3 = -((uVar3 & 0x3ff) >> 4);
//     }
//     iVar4 = (voice + 1) % 0x18;
//     voice = (short)iVar4;
//     voll = (short)((dv & 0x7fU) * (uint)(byte)PersistentState._91_1_ >> 7);
//     uVar1 = SsUtKeyOnV((short)((uint)(iVar4 * 0x10000) >> 0x10),se->VABid,pt >> 4,pt & 0xf,0x24,0,
//                        voll,voll);
//     if (-1 < (int)((uint)uVar1 << 0x10)) {
//       SsUtAutoPan(voice,0x40,(short)((0x40 - uVar3) * 0x10000 >> 0x10),1);
//       return voice;
//     }
//   }
//   return -1;
// }
