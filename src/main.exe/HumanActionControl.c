#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void HumanActionControl(struct Humanoid *human);
 *     MOTION.C:145, 21 src lines, frame 24 bytes, saved-reg mask 0x80000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short dtPAD;
 *     extern short dtCMD;
 *     extern short motID;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/HumanActionControl", HumanActionControl);

// triage: EASY — 73 insns, indirect-call, 6 callees, ~0.12 to InitSoundEffect
// likely-relevant cookbook sections:
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void HumanActionControl(Humanoid *human)
//
// {
//   short sVar1;
//
//   dtPAD = (human->pad).data;
//   Me_MOTION_C = human;
//   dtCMD = GetCommand(&human->pad);
//   DAT_80097f0e = -1;
//   dtL = Me_MOTION_C->locate;
//   dtM = Me_MOTION_C->motion;
//   dtR = Me_MOTION_C->rotate;
//   dtV = &Me_MOTION_C->vector;
//   motID = dtM->mid;
//   if ((Me_MOTION_C->attribute & 0x4000U) == 0) {
//     sVar1 = FallCheck();
//     if (sVar1 == 0) {
//       if (((Me_MOTION_C->map).attrib & 4U) != 0) {
//         SwimCheck();
//       }
//     }
//     else {
//       HangCheck();
//     }
//   }
//   else {
//     DamageControl();
//   }
//   if ((dtPAD & 4) != 0) {
//     dtPAD = dtPAD & 0xfff;
//   }
//   (*(code *)ActionFunc[Me_MOTION_C->status])();
//   if (DAT_80097f0e != -1) {
//     MotionAndMove();
//   }
//   return;
// }
