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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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

extern Humanoid *Me_MOTION_C;

extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void ActITEM(void)
{
    VECTOR *p;
    s16 mode;
    s16 flag;
    PARAM_ITEM_LAUNCH item;

    flag = 0;
    switch (dtM->mid)
    {
    case MOT_ITEM: /* scatter makibishi */
        if (dtM->count != 10)
            break;
        flag = 1;
        item.type = ITEM_MAKIBISHI;
        break;

    case MOT_ITEM_THROW: /* throw (fire/smoke/nemuri) */
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

    case MOT_ITEM_PLANT: /* plant (jirai/goshikimai) */
        if (dtM->count != 5)
            break;
        flag = 1;
        item.type = ITEM_JIRAI;
        break;

    case MOT_ITEM_KAENGEKI: /* kaengeki flame */
    case MOT_ITEM_SHINSOKU: /* shinsoku cast */
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        dtM->loop = MOTION_LOOP_DISABLED;
        return;
    }

    if (flag)
    {
        item.user.human = Me_MOTION_C;
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
        if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        }
        else
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
    }
}
