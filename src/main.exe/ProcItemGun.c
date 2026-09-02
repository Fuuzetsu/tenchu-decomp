#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemGun(struct tag_TItem *item);
 *     ITEM.C:2938, 72 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $s2       struct param_gun * param
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct VECTOR target
 *     reg   $s3       struct Humanoid * IsHuman
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v0       int t
 *     stack sp+48     struct SVECTOR vec
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

#include "item.h"

extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);
extern SVECTOR svec_z_n250[];
extern SVECTOR svec_z_150[];

void ProcItemGun(TItem *item)
{
    enum
    {
        GUN_MODE_FLASH = 0,
        GUN_MODE_FIRE = 1,
        GUN_MODE_FINISH = 2
    };
    param_gun *param;
    SVECTOR vec;
    VECTOR target;

    param = &item->param.gun;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = GUN_MODE_FLASH;
        return;
    }
    switch (item->mode)
    {
    case GUN_MODE_FLASH:
        vec = svec_z_n250[0];
        RotateVectorS(&vec, item->owner->model->rotate.vx, item->owner->model->rotate.vy, 0);
        SetImpact(MODEL_POSITION(item->locate), 2 * FIXED_ONE,
                  IMPACT_SPRITE_GUN);
        SetBleeds(MODEL_POSITION(item->locate), 100, 10, 10, 10, COLOR_GRAY_DARK);
        item->mode++;
        return;

    case GUN_MODE_FIRE:
    {
        s32 rx;
        s32 ry;
        Humanoid *IsHuman;
        s32 conflict_id;

        GetVectorRotation(MODEL_POSITION(item->locate), &param->vec, &rx, &ry);
        vec.vx = rx;
        vec.vy = ry;
        vec.vz = 0;
        IsHuman = SearchItemTarget2(item->owner, &vec, MODEL_POSITION(item->locate), &target);
        item->locate->locate.coord.t[0] = target.vx;
        item->locate->locate.coord.t[1] = target.vy;
        item->locate->locate.coord.t[2] = target.vz;
        DeleteConflict(item->locate);
        conflict_id = InsertConflict(item->locate);
        SET_ITEM_COLLISION(conflict_id, 100, CONFLICT_OWNER_ITEM,
                           CONFLICT_HIT);
        {
            SVECTOR vec;

            vec = svec_z_150[0];
            RotateVectorS(&vec, item->owner->model->rotate.vx, item->owner->model->rotate.vy, 0);
            if (IsHuman != 0)
            {
                SetImpact(&target, 6 * FIXED_ONE, IMPACT_SPRITE_GUN);
                SetBleedsDir(&target, &vec, 100, 15, 10, COLOR_RED);
                SoundEx(&target, SE_GUN_HIT_FLESH);
            }
            else
            {
                SetImpact(&target, 4 * FIXED_ONE, IMPACT_SPRITE_GUN);
                SetBleedsDir(&target, &vec, 100, 15, 10, COLOR_YELLOW);
                SoundEx(&target, SE_GUN_HIT_SOLID);
            }
        }
    }
        item->mode++;
        return;

    case GUN_MODE_FINISH:
        if (item->proc == 0)
            return;
        DISPOSE_ITEM(item);
        return;
    }
}
