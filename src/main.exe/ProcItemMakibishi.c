#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"

#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemMakibishi(struct tag_TItem *item);
 *     ITEM.C:1171, 60 src lines, frame 48 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s2       struct param_drop * param
 *     reg   $a0       int cid
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

extern void MoveKorogari(TItem *item, param_korogari *pp);
extern s32 is_humanoid_on_stage_(Humanoid *h);

void ProcItemMakibishi(TItem *item)
{
    enum
    {
        MAKIBISHI_MODE_ROLL = 0,
        MAKIBISHI_MODE_ARMED = 1
    };
    Sprite3D *model;
    param_drop *param;
    void (*ppu)(TItem *);
    u8 st;
    s32 i;
    s32 conflict_id;

    model = (Sprite3D *)item->model;
    param = &item->param.drop;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = MAKIBISHI_MODE_ROLL;
        return;
    }
    switch (item->mode)
    {
    case MAKIBISHI_MODE_ROLL:
        MoveKorogari(item, &param->koro);
        st = param->koro.status;
        switch (st)
        {
        case KORO_STAY:
            item->mode += 1;
            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            SET_ITEM_COLLISION(conflict_id, 100, CONFLICT_OWNER_ITEM,
                               CONFLICT_HIT);
            break;

        case KORO_WATER:
            ppu = item->proc;
            if (ppu == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        break;

    case MAKIBISHI_MODE_ARMED:
        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            i = CONFLICT_NONE;
        else
            i = GetConflictResult(item->locate, CONFLICT_NONE);
        if (i != CONFLICT_NONE &&
            is_humanoid_on_stage_(ConflictObject[i].common) != 0)
        {
            SetBleeds(MODEL_POSITION(item->locate), 0, 20, 10, 15, RGB24(127, 0, 0));
            SoundEx(MODEL_POSITION(item->locate), SE_PROJECTILE_HIT);
            ppu = item->proc;
            if (ppu == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        break;
    }
    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
}
