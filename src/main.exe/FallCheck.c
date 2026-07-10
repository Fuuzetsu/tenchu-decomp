#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short FallCheck(void);
 *     MOTION.C:256, 31 src lines, frame 40 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+24     struct SVECTOR vect
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct MotionManager *dtM;
 *     extern short RefrectMove[16][2];
 *     extern struct VECTOR *dtL;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

/*
 * FallCheck (0x8001cc90) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=FallCheck
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FallCheck", FallCheck);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FallCheck", switchD_8001cd44__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FallCheck", switchD_8001cd44__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FallCheck", switchD_8001cd44__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/FallCheck", switchD_8001cd44__caseD_1);

/* jump-table pool @ 0x80011240 (14 words; tables at 0x80011240) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 FallCheck_jtbl[14] = {
    0x8001CD64, 0x8001CD6C, 0x8001CD6C, 0x8001CD6C,
    0x8001CD6C, 0x8001CD6C, 0x8001CD64, 0x8001CD4C,
    0x8001CD6C, 0x8001CD64, 0x8001CD6C, 0x8001CD6C,
    0x8001CD64, 0x8001CD64,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
// 
// short FallCheck(void)
// 
// {
//   bool bVar1;
//   Humanoid *pHVar2;
//   VECTOR *pVVar3;
//   int iVar4;
//   int iVar5;
//   
//   pHVar2 = Me_MOTION_C;
//   if (motID == 0x803) {
//     return 1;
//   }
//   if (motID != 0x70f) {
//     if ((Me_MOTION_C->status == 9) && (0 < (Me_MOTION_C->map).height)) {
//       return 1;
//     }
//     if ((Me_MOTION_C->attribute & 0x20U) != 0) {
//       return 0;
//     }
//     if ((Me_MOTION_C->map).height < 0x3e9) {
//       return 0;
//     }
//     switch((int)(((ushort)Me_MOTION_C->status - 4) * 0x10000) >> 0x10) {
//     case 0:
//     case 6:
//     case 9:
//     case 0xc:
//     case 0xd:
//       break;
//     default:
// LAB_8001cd70:
//       dtM->mask = 0x7fff;
//       pVVar3 = dtL;
//       dtL->vx = dtL->vx + ((int)pHVar2->width * (int)RefrectMove[0][(uint)(pHVar2->map).angleH * 2]
//                           >> 2);
//       DAT_80097f0e = 0;
//       motID = 0x803;
//       bVar1 = MotionUpdateMode != 0;
//       pVVar3->vz = pVVar3->vz +
//                    ((int)pHVar2->width * (int)RefrectMove[0][(uint)(pHVar2->map).angleH * 2 + 1] >>
//                    2);
//       if (bVar1) {
//         iVar5 = 0;
//         iVar4 = 0;
//         do {
//           iVar5 = iVar5 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar4 >> 0xd)) == pHVar2) goto LAB_8001ce60;
//           iVar4 = iVar5 * 0x10000;
//         } while (iVar5 * 0x10000 >> 0x10 < 5);
//       }
//       SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//       DAT_80097f0e = -1;
// LAB_8001ce60:
//       if (Me_MOTION_C->status == 0xb) {
//         dtM->count = dtM->count >> 2;
//       }
//       AttackCancelControl(3);
//       return -1;
//     case 7:
//       if (dtM->loop != 0) goto LAB_8001cd70;
//     }
//   }
//   return 0;
// }

#endif /* NON_MATCHING */
