#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"

#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemLightningBolt(struct tag_TItem *item);
 *     ITEM.C:2856, 57 src lines, frame 56 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct param_lightningbolt * param
 *     stack sp+24     struct VECTOR target
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);

void ProcItemLightningBolt(TItem *item)
{
    /* The bolt re-aims and re-strikes every third frame: STRIKE moves the
     * hit volume onto a fresh target, WAIT counts down to the next one. */
    enum
    {
        LIGHTNING_MODE_START = 0,
        LIGHTNING_MODE_STRIKE = 1,
        LIGHTNING_MODE_WAIT = 2
    };
    param_lightningbolt *param;
    VECTOR target;
    u8 cnt;
    s32 conflict_id;

    param = &item->param.lightningbolt;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = LIGHTNING_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case LIGHTNING_MODE_START:
        param->count = 15;
        item->mode++;
        if (item->owner == CamState.Owner)
        {
            SoundEx((VECTOR *)0, SE_LIGHTNING);
        }
        break;

    case LIGHTNING_MODE_STRIKE:
        SearchItemTarget2(item->owner, &item->param.lightningbolt.rot,
                          &param->start, &target);
        item->locate->locate.coord.t[0] = target.vx;
        item->locate->locate.coord.t[1] = target.vy;
        item->locate->locate.coord.t[2] = target.vz;
        DeleteConflict(item->locate);
        conflict_id = InsertConflict(item->locate);
        SET_ITEM_COLLISION(conflict_id, 100, CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
        item->mode++;
        break;

    case LIGHTNING_MODE_WAIT:
        if (GameClock % 3 == 0)
        {
            item->mode = LIGHTNING_MODE_STRIKE;
        }
        break;
    }
    SetLightning(&param->start, MODEL_POSITION(item->locate),
                 100, 100, 200);
    if ((GameClock & 3) == 0)
    {
        SetBleeds(MODEL_POSITION(item->locate), 200, 20, 10, 20, RGB24(255, 255, 120));
        SetImpact(&param->start, 4 * FIXED_ONE, IMPACT_SPRITE_FLASH);
    }
    cnt = param->count;
    param->count = cnt + 0xff;
    if (cnt == 0 && item->proc != 0)
    {
        DISPOSE_ITEM(item);
    }
}
