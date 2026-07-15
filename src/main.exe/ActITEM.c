#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActITEM(void);
 *     MOTION.C:1949, 36 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $a1       short flag
 *     reg   $a0       short mode
 *     reg   $v0       struct VECTOR * p
 * END PSX.SYM */

/*
 * ActITEM (0x80026074) — trigger motion-timed item use and return to idle.
 *
 * STATUS: NON_MATCHING — full guarded semantic reconstruction. The draft has
 * the exact 572-byte extent with only 26 differing bytes and fuzzy 90.21 (up
 * from the comments-only scaffold's 21.68); all three calls and physical CFG
 * counts match. The residual is a flag/count register-allocation tie plus a
 * two-instruction tail scheduling swap. Build with `NON_MATCHING=ActITEM
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", ActITEM);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_1);

/* jump-table pool @ 0x800115d8 (6 words; tables at 0x800115d8) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 handle_char_state_using_item__jtbl[6] = {
    0x800260C0, 0x800261A4, 0x800260E8, 0x8002614C,
    0x80026174, 0x80026174,
};

#else /* NON_MATCHING */

enum
{
    ITEM_MAKIBISHI = 2,
    ITEM_FIRE = 4,
    ITEM_SMOKE = 5,
    ITEM_JIRAI = 6,
    CMODE_NORMAL = 0
};

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern Humanoid *StagePlayer;
extern s16 SelectedItem;
extern s16 motID;
extern s16 motMODE;

extern void ReqItemUse(PARAM_ITEM_USE *item);
extern void SetCameraMode(s32 mode);

void ActITEM(void)
{
    PARAM_ITEM_USE item;
    VECTOR *p;
    s32 item_type;
    s16 flag;
    s16 mode;

    flag = 0;
    switch ((s16)(dtM->mid - 0xF00))
    {
    case 0:
        if (dtM->count != 10)
            break;
        flag = 1;
        item.type = ITEM_MAKIBISHI;
        break;

    case 2:
        if (dtM->count != 5)
            break;
        mode = ITEM_FIRE;
        if (Me_MOTION_C == StagePlayer)
            mode = SelectedItem;
        item_type = mode;
        if (item_type != ITEM_FIRE)
        {
            flag = 0;
            if (item_type != ITEM_SMOKE)
                break;
        }
        flag = 1;
        item.type = item_type;
        break;

    case 3:
        if (dtM->count != 5)
            break;
        flag = 1;
        item.type = ITEM_JIRAI;
        break;

    case 4:
    case 5:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        dtM->loop = -1;
        return;
    }

    if (flag)
    {
        item.user = Me_MOTION_C;
        p = GetAbsolutePosition(Me_MOTION_C->model->object[2], 0, 0, 0);
        item.start.vx = p->vx;
        item.start.vy = p->vy;
        item.start.vz = p->vz;
        item.end = item.start;
        ReqItemUse(&item);
    }

    if (dtM->count == 0 && dtM->loop != 0)
    {
        if (Me_MOTION_C == StagePlayer)
            SetCameraMode(CMODE_NORMAL);
        if (Me_MOTION_C->attribute & 0x40)
            motID = 0x501;
        else
            motID = 0;
        motMODE = 1;
    }
}

#endif /* NON_MATCHING */
