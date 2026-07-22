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
 * STATUS: MATCHING — exact 572 bytes.
 *
 * PSX.SYM's four locals are sufficient.  The two valid item modes each
 * contain their natural `flag`/`item.type` writes; jump2 cross-jumps those
 * identical tails after allocation.  Keeping the source arms distinct gives
 * `flag` the target's $a1 allocation without the redundant reset that reorg
 * used to steal into the target's bare delay-slot nop.
 */

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern Humanoid *StagePlayer;
extern s16 SelectedItem;
extern s16 motID;
extern s16 motMODE;

extern void ReqItemUse(PARAM_ITEM_LAUNCH *item);
extern void SetCameraMode(s32 mode);

void ActITEM(void)
{
    VECTOR *p;
    s16 mode;
    s16 flag;
    PARAM_ITEM_LAUNCH item;

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
        if (mode == ITEM_FIRE)
        {
            flag = 1;
            item.type = mode;
        }
        else if (mode == ITEM_SMOKE)
        {
            flag = 1;
            item.type = mode;
        }
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
        {
            motID = 0x501;
            motMODE = 1;
        }
        else
        {
            motID = 0;
            motMODE = 1;
        }
    }
}
