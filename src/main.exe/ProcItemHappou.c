#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"

#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemHappou(struct tag_TItem *item);
 *     ITEM.C:2486, 52 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct ModelType * model
 *     reg   $s1       struct param_launch * param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * m
 *     reg   $s1       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType *HappouModel;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

extern void MoveFly(TItem *item, param_fly *param);
extern short DrawModel(ModelType *objp);
extern s32 is_humanoid_on_stage_(Humanoid *h);
/* Explicit binding avoids an eight-byte offset error in the generated
 * data-section symbol. */

void ProcItemHappou(TItem *item)
{
    ModelType *model;
    param_launch *param;
    u8 t;
    fly_mode mode;
    s32 i;
    s32 conflict_id;

    model = HappouModel;
    param = &item->param.launch;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        DisposeAfterimage(param->effect);
        item->mode = ITEM_MODE_START;
        return;
    }
    MoveFly(item, &param->fly);
    t = param->count - 1;
    param->count = t;
    if (t == 0)
    {
        DeleteConflict(item->locate);
        conflict_id = InsertConflict(item->locate);
        SET_ITEM_COLLISION(conflict_id, 300, CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
    }
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawModel(model);
    DrawAfterimage(param->effect, 1);
    mode = param->fly.mode;
    if (mode != FLY_MODE_ARC)
    {
        if (mode == FLY_MODE_ROLL &&
            param->fly.p.koro.status != KORO_NORMAL)
        {
            SetBleeds(MODEL_POSITION(item->locate), 0, 25, 10, 10, COLOR_YELLOW);
            SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_IMPACT);
            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }
        }
    }
    if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        i = CONFLICT_NONE;
    else
        i = GetConflictResult(item->locate, CONFLICT_NONE);
    if (i != CONFLICT_NONE &&
        is_humanoid_on_stage_(ConflictObject[i].common) != 0)
    {
        SetImpact(MODEL_POSITION(item->locate), 4 * FIXED_ONE,
                  IMPACT_SPRITE_HIT);
        SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_HIT);
        DeleteConflict(item->locate);
    }
}
