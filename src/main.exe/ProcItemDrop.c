#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemDrop(struct tag_TItem *item);
 *     ITEM.C:835, 68 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s4       struct param_drop * param
 *     reg   $a0       int cid
 *     reg   $t1       struct Sprite3D * model
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s4       struct param_korogari * param
 *     reg   $s1       int x
 *     reg   $s0       int y
 *     reg   $v0       int z
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern short ActionHalt;
 * END PSX.SYM */

extern void MoveKorogari(TItem *item, param_korogari *pp);
extern s32 is_humanoid_on_stage_(Humanoid *h);

void ProcItemDrop(TItem *item)
{
    enum
    {
        DROP_MODE_ROLL = 0,
        DROP_MODE_WAIT = 1,
        DROP_MODE_TRANSFER = 2
    };
    Sprite3D *model;
    param_drop *param;
    void (*ppu)(TItem *);
    Humanoid *human;
    MotionDataType *md;
    s32 i;
    s32 conflict_id;
    ConflictClass collision_mode;
    s32 x;
    s32 y;
    s32 z;
    u8 cnt;
    u8 count;

    model = (Sprite3D *)item->model;
    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = DROP_MODE_ROLL;
        return;
    }
    model->locate = item->locate->locate;
    DrawSprite(model);
    switch (item->mode)
    {
    case DROP_MODE_ROLL:
        MoveKorogari(item, &param->koro);
        switch (param->koro.status)
        {
        case KORO_WATER:
            ppu = item->proc;
            if (ppu == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        case KORO_GRAND:
        case KORO_STAY:
            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            collision_mode = CONFLICT_SOFT;
            SET_ITEM_COLLISION(conflict_id, 180, CONFLICT_OWNER_ITEM,
                               collision_mode);
            item->mode++;
            return;
        }
        return;

    case DROP_MODE_WAIT:
        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            i = CONFLICT_NONE;
        else
            i = GetConflictResult(item->locate, CONFLICT_NONE);
        if (i == CONFLICT_NONE)
            return;
        human = ConflictObject[i].common;
        if (is_humanoid_on_stage_(human) == 0)
            return;
        if (human->motion->mid == MOT_STATE_PICKUP)
            return;
        if ((human->status != STAT_CHASE) && (human->status != STAT_MOVE))
            return;
        if (ActionHalt == ACTION_HALT_NONE && human->life > 0)
        {
            dispose_weapon_data_of_char_(human, ATTACK_CANCEL_ALL);
            UpdateMotion(human->motion, MOT_STATE_PICKUP);
            human->status = STAT_STATE;
            md = human->motion->motion;
            MoveHumanoid(human, md->orderspd, md->sidespd);
        }
        item->owner = human;
        item->mode++;
        param->count = 0;
        return;

    case DROP_MODE_TRANSFER:
        if (item->owner->motion->mid != MOT_STATE_PICKUP)
        {
            x = rand();
            x = x % 200;
            y = rand();
            y = y % 100;
            z = rand();
            z = z % 200;
            param->koro.vx = x - 100;
            param->koro.vy = y - 200;
            param->koro.hint = 0;
            param->koro.status = KORO_NORMAL;
            param->koro.vz = z - 100;
            item->mode = DROP_MODE_ROLL;
        }
        cnt = param->count + 1;
        param->count = cnt;
        if (cnt == 10)
        {
            SoundEx(item->owner->locate, SE_ITEM_TRANSFER);
            count = item->owner->item[item->type];
            if (count != ITEM_INFINITE)
            {
                item->owner->item[item->type] = count + 1;
            }
            ppu = item->proc;
            if (ppu == 0)
                return;
            DISPOSE_ITEM(item);
        }
        return;
    }
}
