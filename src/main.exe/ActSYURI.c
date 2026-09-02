#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSYURI(void);
 *     MOTION.C:1908, 37 src lines, frame 64 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v0       struct VECTOR * p
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void ActSYURI(void)
{
    VECTOR *p;
    PARAM_ITEM_LAUNCH item;

    switch (dtM->mid)
    {
    case MOT_SYURI:
        if (Me_MOTION_C != StagePlayer)
        {
            if (dtM->count != 0)
                return;
            if (dtM->loop == 0)
                return;
            SET_MOTION(MOT_SYURI_RECOVER, MOTION_MOVE_APPLY);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        if (dtM->count == 1)
        {
            item.type = ITEM_SHURIKEN;
            item.user = Me_MOTION_C;
            p = GetAbsolutePosition(
                Me_MOTION_C->model->object[MODEL_PART_HEAD], 0, 0, 0);
            item.start.vx = p->vx;
            item.start.vy = p->vy;
            item.start.vz = p->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, SE_THROW_WEAPON);
        }
        else if (spare_item_slot_(SPARE_ITEM_SLOT_QUERY, Me_MOTION_C) == 0)
        {
            SET_MOTION(MOT_SYURI_RECOVER, MOTION_MOVE_APPLY);
            Sound(Me_MOTION_C, SE_WEAPON_RECOVER);
        }
        else if (Me_MOTION_C->pad.trig & (PADRleft | PADRdown | PADRright))
        {
            spare_item_slot_(SPARE_ITEM_SLOT_CLEAR, 0);
            SELECT_RETURN_MOTION();
        }
        break;
    case MOT_SYURI_RECOVER:
        if (dtM->count == 1 && Me_MOTION_C != StagePlayer)
        {
            ReqItemDefault(Me_MOTION_C, ITEM_SHURIKEN);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SELECT_RETURN_MOTION();
        }
        break;
    }
}
