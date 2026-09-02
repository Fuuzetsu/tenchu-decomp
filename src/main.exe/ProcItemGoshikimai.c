#include "common.h"
#include "main.exe.h"

#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemGoshikimai(struct tag_TItem *item);
 *     ITEM.C:2223, 35 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $s0       struct param_goshikimai * param
 *     reg   $s0       struct Humanoid * human
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 * END PSX.SYM */

void ProcItemGoshikimai(TItem *item)
{
    enum
    {
        GOSHIKIMAI_MODE_START = 0,
        GOSHIKIMAI_MODE_THROW = 1
    };
    param_goshikimai *param;
    Humanoid *human;
    PARAM_ITEM_LAUNCH p;

    param = &item->param.goshikimai;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = GOSHIKIMAI_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case GOSHIKIMAI_MODE_START:
        human = item->owner;
        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_ITEM_PLANT);
            human->status = STAT_STATE;
            MoveHumanoid(human, human->motion->motion->orderspd,
                         human->motion->motion->sidespd);
        }
        item->mode++;
        return;

    case GOSHIKIMAI_MODE_THROW:
        if (item->owner->motion->mid != MOT_ITEM_PLANT)
        {
            item->mode = GOSHIKIMAI_MODE_START;
            return;
        }
        if (item->owner->motion->count != 15)
            return;
        p.type = ITEM_GOSHIKIMAI;
        p.user = item->owner;
        p.start.vx = GetAbsolutePosition(item->owner->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, 0)->vx;
        p.start.vy = GetAbsolutePosition(item->owner->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, 0)->vy;
        p.start.vz = GetAbsolutePosition(item->owner->model->object[MODEL_PART_WEAPON_HAND_0], 0, 0, 0)->vz;
        p.end.vx = param->vec.vx;
        p.end.vy = param->vec.vy;
        p.end.vz = param->vec.vz;
        NowReturnNormal(item->owner);
        if (item->proc != 0)
        {
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != GOSHIKIMAI_MODE_START)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        ReqItemDrop(&p);
        return;
    }
}
