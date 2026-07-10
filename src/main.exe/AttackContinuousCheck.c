#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short AttackContinuousCheck(struct BattleType *battle);
 *     MOTION.C:755, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       struct BattleType * battle
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AttackContinuousCheck", AttackContinuousCheck);

// triage: EASY — 105 insns, 2 callees, ~0.08 to ProcItemGoshikimai
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short AttackContinuousCheck(BattleType *battle)
//
// {
//   short sVar1;
//   Humanoid *pHVar2;
//   ModelType *model;
//
//   pHVar2 = Me_MOTION_C;
//   if ((int)dtM->count < battle->contfrm + -3) {
//     return 0;
//   }
//   if (battle->contfrm + 3 < (int)dtM->count) {
//     return 0;
//   }
//   sVar1 = Me_MOTION_C->wpatk;
//   (Me_MOTION_C->pad).time = 0;
//   if (sVar1 == 2) {
//     DeleteConflict(pHVar2->model->object[8]);
//     model = Me_MOTION_C->model->object[0xb];
//   }
//   else {
//     if (sVar1 < 3) {
//       if (sVar1 == 0) goto LAB_8001f2a0;
//     }
//     else if (sVar1 == 3) {
//       model = pHVar2->model->object[2];
//       goto LAB_8001f294;
//     }
//     DeleteConflict(Me_MOTION_C->model->object[0xd]);
//     model = Me_MOTION_C->model->object[0xe];
//   }
// LAB_8001f294:
//   DeleteConflict(model);
// LAB_8001f2a0:
//   if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
//     DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
//     Me_MOTION_C->illusion[0] = (undefined *)0x0;
//   }
//   if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
//     DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
//     Me_MOTION_C->illusion[1] = (undefined *)0x0;
//   }
//   dtM->mask = 0x7fff;
//   return 1;
// }
