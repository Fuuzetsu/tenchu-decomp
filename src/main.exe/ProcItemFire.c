#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/*
 * ProcItemFire (0x80044de0) — rolls and draws the fire item, emits a small
 * random bleed every frame, occasionally converts an exhausted item back into
 * a pickup, expands its collision volume when lit, and attaches a frame effect
 * to a collided character until the countdown expires.
 *
 * Matching notes:
 *  - The randomized particle VECTOR is dead before the original `vec`
 *    SVECTOR is written into the same slot.
 *  - The three `rand() % (nr * 2)` expressions use separate single-definition
 *    return temps plus `base_* = coordinate - nr`.  Reusing one rand temp leaves
 *    copies from $v0; inlining the calls lets combine reassociate `-25` with
 *    the remainder and removes each target coordinate-load hazard nop.
 *  - `count` is full-width, with explicit `(u8)` tests.  That keeps the
 *    decrement in one SI pseudo ($v1) and emits the target narrowing at each
 *    switch path; an `u8` local was one instruction short and needed a copy.
 *  - The pickup conversion intentionally mixes pointer and direct spellings:
 *    `launch->user` and ReqItemDrop retain the launch pointer in $s0, while
 *    direct aggregate fields preserve the target stack-relative loads/stores.
 *    The saved-position pointer supplies the sequential source loads.
 *  - Named full-width `size`/`collision_mode` values share 500 and 8 across
 *    halfword and word stores.  Separate literals materialize 8 twice.
 *  - This TU needs maspsx `--expand-div` for the dynamic model-count remainder
 *    guard; Build.hs and permute.py carry the mirrored per-function setting.
 */

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
extern SVECTOR svec_y_n30[];

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
            VECTOR *position;
            SVECTOR *vec;
            s32 random_x;
            s32 random_y;
            s32 random_z;
            s32 base_x;
            s32 base_y;
            s32 base_z;
            VECTOR pos;
            VECTOR work;

            vec = (SVECTOR *)&work;
            memset(&work, 0, sizeof(VECTOR));
            random_x = rand();
            base_x = item->locate->locate.coord.t[0] - nr;
            work.vx = base_x + random_x % (nr * 2);
            random_y = rand();
            base_y = item->locate->locate.coord.t[1] - nr;
            work.vy = base_y + random_y % (nr * 2);
            random_z = rand();
            base_z = item->locate->locate.coord.t[2] - nr;
            work.vz = base_z + random_z % (nr * 2);
            pos = work;
            *vec = svec_y_n30[0];
            position = &pos;
            SetBleed(position, vec, rand() % 20, COLOR_YELLOW);
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
            SVECTOR vec;
            VECTOR pos;
            VECTOR pos_buf;

            vec = svec_y_n25[0];
            memset(&pos_buf, 0, sizeof(VECTOR));
            pos_buf.vx = item->locate->locate.coord.t[0];
            pos_buf.vy = item->locate->locate.coord.t[1];
            pos_buf.vz = item->locate->locate.coord.t[2];
            pos = pos_buf;
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
            /* This arm runs with mode == FIRE_MODE_EXPLODE. Retail reuses that
             * register as the owner tag (CONFLICT_OWNER_ITEM == 1), the conflict
             * class, and the collision mode below -- the same one-register trick
             * as the file's other box and ProcItemArrow's. Separate named
             * constants load fresh immediates and do not match. */
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
                VECTOR pos;
                VECTOR random_pos;
                objects = human->model->object;
                if (human->model->n > 0)
                {
                    objects += rand() % human->model->n;
                }
                model = *objects;
                memset(&random_pos, 0, sizeof(VECTOR));
                random_pos.vx = rand() % 200 - 100;
                random_pos.vy = rand() % 200 - 100;
                random_pos.vz = rand() % 200 - 100;
                pos = random_pos;
                SetFrame(&pos, 3 * FIXED_ONE, 120,
                         &model->locate);
            }
        }
        return;
    }
}
