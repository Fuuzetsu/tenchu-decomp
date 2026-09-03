#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"

extern s32 is_humanoid_on_stage_(Humanoid *human);
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNapalm(struct tag_TItem *item);
 *     ITEM.C:3014, 71 src lines, frame 80 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s5       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s0       int ex
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+16     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprNapalm2;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

void ProcItemNapalm(TItem *item)
{
    enum
    {
        NAPALM_MODE_START = 0,
        NAPALM_MODE_EXPAND = 1,
        NAPALM_MODE_FINISH = 2,
        MaxCount = 20
    };
    Sprite3D *model;
    param_napalm *param;
    void (*proc)(TItem *);
    u8 count;
    s32 ex;
    s32 cid;

    model = (Sprite3D *)item->model;
    param = &item->param.napalm;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = NAPALM_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case NAPALM_MODE_START:
        param->count = 0;
        item->mode++;
        return;

    case NAPALM_MODE_EXPAND:
    {
        u8 t;

        ex = param->count;
        ex = ex * ex;
        item->locate->locate.coord.t[0] += param->vec.vx * ex / 100;
        item->locate->locate.coord.t[1] += param->vec.vy * ex / 100;
        item->locate->locate.coord.t[2] += param->vec.vz * ex / 100;

        t = rand() % 25;
        t -= 26;
        t -= param->count * 230 / MaxCount;
        model->sprite.r = t;
        model->sprite.g = model->sprite.r;
        model->sprite.b = model->sprite.r;
        model->sprite.rotate = (rand() % 360) << FIXED_SHIFT;
        model->scale = (ex << FIXED_SHIFT) / 50 + FIXED_ONE;

        sprNapalm2->sprite.r = (ITEM_MODE_DISPOSE - model->sprite.r) / 3;
        sprNapalm2->sprite.g = sprNapalm2->sprite.r;
        sprNapalm2->sprite.b = sprNapalm2->sprite.r;
        sprNapalm2->sprite.rotate = model->sprite.rotate;
        sprNapalm2->scale = model->scale;

        if (param->count == 10)
        {
            s32 conflict_id;

            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            SET_ITEM_COLLISION(conflict_id, 500, CONFLICT_OWNER_ITEM,
                               CONFLICT_HIT);
        }

        count = param->count + 1;
        param->count = count;
        if (count > MaxCount)
        {
            item->mode++;
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = CONFLICT_NONE;
        }
        else
        {
            cid = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (cid != CONFLICT_NONE)
        {
            Humanoid *human;

            human = ConflictObject[cid].common;
            if (is_humanoid_on_stage_(human) != 0)
            {
                ModelType **objects;
                ModelType *model;

                objects = human->model->object;
                if (human->model->n > 0)
                {
                    objects += rand() % human->model->n;
                }
                model = *objects;
                {
                    VECTOR pos = {
                        rand() % 200 - 100,
                        rand() % 200 - 100,
                        rand() % 200 - 100
                    };

                    SetFrame(&pos, 3 * FIXED_ONE, 60,
                             &model->locate);
                }
            }
        }

        if (GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2],
                            AREA_LEVEL_DEFAULT) ==
            LEVEL_NONE)
        {
            proc = item->proc;
            if (proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        break;
    }

    case NAPALM_MODE_FINISH:
        proc = item->proc;
        if (proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }

    UpdateCoordinate(item->locate);
    sprNapalm2->locate = item->locate->locate;
    DrawSprite(sprNapalm2);
    model->locate = item->locate->locate;
    DrawSprite(model);
}
