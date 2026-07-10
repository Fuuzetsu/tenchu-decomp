#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ItemControl(void);
 *     MOTION.C:896, 13 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * ItemControl (0x80027818) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ItemControl
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", ItemControl);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", FUN_80027820);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ItemControl", switchD_80027854__caseD_4);

/* jump-table pool @ 0x80011628 (12 words; tables at 0x80011628) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 FUN_80027818_jtbl[12] = {
    0x8002789C, 0x80027864, 0x8002785C, 0x8002786C,
    0x800278B8, 0x8002787C, 0x80027874, 0x80027884,
    0x800278B8, 0x800278B8, 0x800278B8, 0x8002789C,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ItemControl(void)
// 
// {
//   switch((int)((DAT_80097b1e + 1) * 0x10000) >> 0x10) {
//   case 0:
//   case 0xb:
//     SoundEx(Me_MOTION_C->locate,0xc);
//     return;
//   case 1:
//     motID = 0x400;
//     break;
//   case 2:
//     motID = 0xe00;
//     break;
//   case 3:
//     motID = 0xf00;
//     break;
//   default:
//     ReqItemDefault(Me_MOTION_C,(int)(short)DAT_80097b1e);
//     return;
//   case 5:
//     motID = 0xf02;
//     break;
//   case 6:
//     motID = 0xf02;
//     break;
//   case 7:
//     motID = 0xf03;
//   }
//   DAT_80097f0e = 1;
//   return;
// }

#endif /* NON_MATCHING */
