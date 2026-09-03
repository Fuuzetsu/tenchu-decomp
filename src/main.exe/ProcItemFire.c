#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemFire(struct tag_TItem *item);
 *     ITEM.C:2586, 116 src lines, frame 176 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s5       struct param_smoke * param
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vec
 *     stack sp+56     struct PARAM_ITEM_STAY rparam
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+104    struct PARAM_ITEM_LAUNCH param
 *     reg   $a0       int cid
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $v0       struct Humanoid * human
 *     stack sp+48     struct SVECTOR vec
 *     stack sp+80     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s2       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+96     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

extern SVECTOR svec_y_n25[]; /* {0,-25,0} */

extern void MoveKorogari(TItem *item, param_korogari *param);
extern s32 is_humanoid_on_stage_(Humanoid *human);
extern void SetSmokeS(VECTOR *pos, short vx, short vy, short vz, unsigned short time);
extern void reset_alert_duration(void);

void ProcItemFire(TItem *item)
{
    enum
    {
        FIRE_MODE_FUSE = 0,
        FIRE_MODE_EXPLODE = 1,
        FIRE_MODE_BLAST = 2,
        nr = 25
    };
    Sprite3D *model;
    param_smoke *param;
    s32 count;
    s32 mode;
    s32 cid;

    model = (Sprite3D *)item->model;
    param = &item->param.smoke;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = FIRE_MODE_FUSE;
        return;
    }

    MoveKorogari(item, &param->koro);
    if (param->koro.status == KORO_WATER)
    {
        if (item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }
        return;
    }

    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);

    {
        {
            VECTOR pos = {
                .vx = item->locate->locate.coord.t[0] +
                    (rand() % (nr * 2) - nr),
                .vy = item->locate->locate.coord.t[1] +
                    (rand() % (nr * 2) - nr),
                .vz = item->locate->locate.coord.t[2] +
                    (rand() % (nr * 2) - nr)
            };
            SVECTOR vec = {
                .vx = 0,
                .vy = -30,
                .vz = 0
            };

            SetBleed(&pos, &vec, rand() % 20, COLOR_YELLOW);
        }
    }

    count = param->count - 1;
    param->count = count;
    mode = item->mode;
    switch (mode)
    {
    case FIRE_MODE_FUSE:
        if ((u8)count == 0)
        {
            if (rand() % 10 < 2)
            {
                PARAM_ITEM_STAY saved_record;
                PARAM_ITEM_STAY rparam;
                PARAM_ITEM_LAUNCH launch_record;
                PARAM_ITEM_STAY *saved;
                PARAM_ITEM_LAUNCH *launch;

                memset(&rparam, 0, sizeof(PARAM_ITEM_STAY));
                rparam.type = item->type;
                rparam.locate.vx = model->locate.coord.t[0];
                rparam.locate.vy = model->locate.coord.t[1];
                rparam.locate.vz = model->locate.coord.t[2];
                saved_record = rparam;

                if (item->proc != 0)
                {
                    DISPOSE_ITEM(item);
                }

                saved = &saved_record;
                launch = &launch_record;
                launch_record.type = saved->type;
                launch->user = (Humanoid *)CONFLICT_OWNER_ITEM;
                launch_record.start.vx = saved->locate.vx;
                launch_record.start.vy = saved->locate.vy;
                launch_record.start.vz = saved->locate.vz;
                launch_record.end.vx = 0;
                launch_record.end.vy = 0;
                launch_record.end.vz = 0;
                launch_record.start.vy = GetAreaMapLevel(
                    GlobalAreaMap, launch_record.start.vx,
                    launch_record.start.vy,
                    launch_record.start.vz, AREA_LEVEL_DEFAULT);
                ReqItemDrop(launch);
                SetSmokeS(&saved->locate, 0, -100, 0, 10);
                return;
            }
        }
        else
        {
            if ((u8)count == 140)
            {
                s32 conflict_id;
                s32 size;
                ConflictClass collision_mode;

                DeleteConflict(item->locate);
                conflict_id = InsertConflict(item->locate);
                size = 500;
                collision_mode = CONFLICT_SOFT;
                SET_ITEM_COLLISION(conflict_id, size, CONFLICT_OWNER_ITEM,
                                   collision_mode);
            }

            if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            {
                cid = CONFLICT_NONE;
            }
            else
            {
                cid = GetConflictResult(item->locate, CONFLICT_NONE);
            }
            if (cid == CONFLICT_NONE)
            {
                return;
            }
            if (is_humanoid_on_stage_(
                    ConflictObject[cid].common) == 0 &&
                ConflictObject[cid].size.pad !=
                    CONFLICT_HIT)
            {
                return;
            }
        }
        item->mode++;
        return;

    case FIRE_MODE_EXPLODE:
    {
        {
            s32 conflict_id;
            SVECTOR vec = svec_y_n25[0];
            VECTOR pos = {
                .vx = item->locate->locate.coord.t[0],
                .vy = item->locate->locate.coord.t[1],
                .vz = item->locate->locate.coord.t[2]
            };

            SetExplosion(&pos, &vec);

            vec.vx = 75;
            vec.vy = 120;
            vec.vz = 75;
            SetHinoko(&pos, &vec, 8);
            vec.vx = 0;
            vec.vy = -200;
            vec.vz = 0;
            SetSmoke(&pos, &vec, 20, 6);
            SoundEx(&pos, SE_EXPLOSION);

            DeleteConflict(item->locate);
            conflict_id = InsertConflict(item->locate);
            ConflictObject[conflict_id].offset.vx = 0;
            ConflictObject[conflict_id].offset.vz = 0;
            ConflictObject[conflict_id].offset.vy = 0;
            ConflictObject[conflict_id].size.vz = 1500;
            ConflictObject[conflict_id].size.vy = 1500;
            ConflictObject[conflict_id].size.vx = 1500;
            ConflictObject[conflict_id].common = (void *)mode;
            ConflictObject[conflict_id].size.pad = mode;
            item->collision.size = 1500;
            item->collision.ofsY = 0;
            item->collision.mode = mode;
            item->collision.pause = 0;
            item->mode++;
            param->count = 3;
            reset_alert_duration();
            return;
        }
    }

    case FIRE_MODE_BLAST:
        if ((u8)count == 0 && item->proc != 0)
        {
            DISPOSE_ITEM(item);
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
                        .vx = rand() % 200 - 100,
                        .vy = rand() % 200 - 100,
                        .vz = rand() % 200 - 100
                    };

                    SetFrame(&pos, 3 * FIXED_ONE, 120,
                             &model->locate);
                }
            }
        }
        return;
    }
}
