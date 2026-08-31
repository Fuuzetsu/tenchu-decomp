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
 *  - The mutually-exclusive particle/drop/explosion/frame aggregates share the
 *    explicit ProcItemFireScratch union.  Its views reproduce the retail
 *    sp+0x18..sp+0x77 overlay and the exact 0x98-byte frame; ordinary block
 *    locals made cc1 reserve an extra 24 bytes.
 *  - Within the particle view, the randomized VECTOR is dead before the
 *    original `vec` SVECTOR is written into the same slot. The typed inner
 *    union records that reuse directly instead of casting the VECTOR.
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
 *    The saved-position pointer supplies the sequential source loads. The
 *    aggregate's `param` member retains PSX.SYM's exact name for the
 *    semantically corresponding demo PARAM_ITEM_LAUNCH object; retail moved
 *    that request to sp+0x50 as part of its revised scratch overlay.
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

/* The retail stack frame overlays the mutually-exclusive mode temporaries.
 * Keep that layout explicit: all four views begin at sp+0x18 and the largest
 * one ends immediately before the saved-register area at sp+0x78. */
typedef union
{
    struct
    {
        VECTOR pos;
        union
        {
            VECTOR random_pos;
            SVECTOR vec;
        } work;
    } particle;
    struct
    {
        PARAM_ITEM_STAY saved;
        u8 pad0[12];
        PARAM_ITEM_STAY rparam;
        u8 pad1[4];
        PARAM_ITEM_LAUNCH param;
    } drop;
    struct
    {
        SVECTOR vec;
        VECTOR pos;
        VECTOR pos_buf;
    } explosion;
    struct
    {
        VECTOR pos;
        VECTOR random_pos;
    } frame;
} ProcItemFireScratch;

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
        nr = 25
    };
    Sprite3D *model;
    param_smoke *param;
    s32 count;
    s32 mode;
    s32 cid;
    ProcItemFireScratch scratch;

    model = (Sprite3D *)item->model;
    param = &item->param.smoke;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = 0;
        return;
    }

    MoveKorogari(item, &param->koro);
    if (param->koro.status == KORO_WATER)
    {
        if (item->proc != 0)
        {
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        return;
    }

    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);

    {
        VECTOR *position;
        SVECTOR *vec;
        s32 random_x;
        s32 random_y;
        s32 random_z;
        s32 base_x;
        s32 base_y;
        s32 base_z;

        vec = &scratch.particle.work.vec;
        memset(&scratch.particle.work.random_pos, 0, sizeof(VECTOR));
        random_x = rand();
        base_x = item->locate->locate.coord.t[0] - nr;
        scratch.particle.work.random_pos.vx = base_x + random_x % (nr * 2);
        random_y = rand();
        base_y = item->locate->locate.coord.t[1] - nr;
        scratch.particle.work.random_pos.vy = base_y + random_y % (nr * 2);
        random_z = rand();
        base_z = item->locate->locate.coord.t[2] - nr;
        scratch.particle.work.random_pos.vz = base_z + random_z % (nr * 2);
        scratch.particle.pos = scratch.particle.work.random_pos;
        *vec = svec_y_n30[0];
        position = &scratch.particle.pos;
        SetBleed(position, vec, rand() % 20, COLOR_YELLOW);
    }

    count = param->count - 1;
    param->count = count;
    mode = item->mode;
    switch (mode)
    {
    case 0:
        if ((u8)count == 0)
        {
            if (rand() % 10 < 2)
            {
                PARAM_ITEM_STAY *saved;
                PARAM_ITEM_LAUNCH *launch;

                memset(&scratch.drop.rparam, 0, sizeof(PARAM_ITEM_STAY));
                scratch.drop.rparam.type = item->type;
                scratch.drop.rparam.locate.vx = model->locate.coord.t[0];
                scratch.drop.rparam.locate.vy = model->locate.coord.t[1];
                scratch.drop.rparam.locate.vz = model->locate.coord.t[2];
                scratch.drop.saved = scratch.drop.rparam;

                if (item->proc != 0)
                {
                    item->mode = ITEM_MODE_DISPOSE;
                    item->proc(item);
                    DeleteConflict(item->locate);
                    if (item->mode != 0)
                    {
                        AdtMessageBox(msg_item_dispose_fail, item->type,
                                      (u32)item->mode);
                    }
                    item->owner = 0;
                    item->proc = 0;
                }

                saved = &scratch.drop.saved;
                launch = &scratch.drop.param;
                scratch.drop.param.type = saved->type;
                launch->user = (Humanoid *)CONFLICT_OWNER_ITEM;
                scratch.drop.param.start.vx = saved->locate.vx;
                scratch.drop.param.start.vy = saved->locate.vy;
                scratch.drop.param.start.vz = saved->locate.vz;
                scratch.drop.param.end.vx = 0;
                scratch.drop.param.end.vy = 0;
                scratch.drop.param.end.vz = 0;
                scratch.drop.param.start.vy = GetAreaMapLevel(
                    GlobalAreaMap, scratch.drop.param.start.vx,
                    scratch.drop.param.start.vy,
                    scratch.drop.param.start.vz, 0);
                ReqItemDrop(launch);
                SetSmokeS(&saved->locate, 0, -100, 0, 10);
                return;
            }
        }
        else
        {
            if ((u8)count == 140)
            {
                s32 n;
                s32 size;
                s32 collision_mode;

                DeleteConflict(item->locate);
                n = InsertConflict(item->locate);
                size = 500;
                collision_mode = 8;
                SET_ITEM_COLLISION(n, size, (void *)1, collision_mode);
            }

            if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
            {
                cid = -1;
            }
            else
            {
                cid = GetConflictResult(item->locate, -1);
            }
            if (cid == -1)
            {
                return;
            }
            if (is_humanoid_on_stage_(
                    (Humanoid *)ConflictObject[cid].common) == 0 &&
                ConflictObject[cid].size.pad != 1)
            {
                return;
            }
        }
        item->mode++;
        return;

    case 1:
    {
        s32 n;

        scratch.explosion.vec = svec_y_n25[0];
        memset(&scratch.explosion.pos_buf, 0, sizeof(VECTOR));
        scratch.explosion.pos_buf.vx = item->locate->locate.coord.t[0];
        scratch.explosion.pos_buf.vy = item->locate->locate.coord.t[1];
        scratch.explosion.pos_buf.vz = item->locate->locate.coord.t[2];
        scratch.explosion.pos = scratch.explosion.pos_buf;
        SetExplosion(&scratch.explosion.pos, &scratch.explosion.vec);

        scratch.explosion.vec.vx = 75;
        scratch.explosion.vec.vy = 120;
        scratch.explosion.vec.vz = 75;
        SetHinoko(&scratch.explosion.pos, &scratch.explosion.vec, 8);
        scratch.explosion.vec.vx = 0;
        scratch.explosion.vec.vy = -200;
        scratch.explosion.vec.vz = 0;
        SetSmoke(&scratch.explosion.pos, &scratch.explosion.vec, 20, 6);
        SoundEx(&scratch.explosion.pos, SE_EXPLOSION);

        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = 1500;
        ConflictObject[n].size.vy = 1500;
        ConflictObject[n].size.vx = 1500;
        /* This arm runs with mode == 1, and retail reuses that register as
         * the owner tag (CONFLICT_OWNER_ITEM == 1), the size pad, and the
         * collision mode below -- the same one-register trick as the
         * file's other box and ProcItemArrow's. Separate named
         * constants load fresh immediates and do not match. */
        ConflictObject[n].common = (void *)(s32)mode;
        ConflictObject[n].size.pad = mode;
        item->collision.size = 1500;
        item->collision.ofsY = 0;
        item->collision.mode = mode;
        item->collision.pause = 0;
        item->mode++;
        param->count = 3;
        reset_alert_duration();
        return;
    }

    case 2:
        if ((u8)count == 0 && item->proc != 0)
        {
            DISPOSE_ITEM(item);
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            cid = -1;
        }
        else
        {
            cid = GetConflictResult(item->locate, -1);
        }
        if (cid != -1)
        {
            Humanoid *human;

            human = (Humanoid *)ConflictObject[cid].common;
            if (is_humanoid_on_stage_(human) != 0)
            {
                ModelType **objects;
                ModelType *frame_model;
                objects = human->model->object;
                if (human->model->n > 0)
                {
                    objects += rand() % human->model->n;
                }
                frame_model = *objects;
                memset(&scratch.frame.random_pos, 0, sizeof(VECTOR));
                scratch.frame.random_pos.vx = rand() % 200 - 100;
                scratch.frame.random_pos.vy = rand() % 200 - 100;
                scratch.frame.random_pos.vz = rand() % 200 - 100;
                scratch.frame.pos = scratch.frame.random_pos;
                SetFrame(&scratch.frame.pos, 3 * FIXED_ONE, 120,
                         (GsCOORDINATE2 *)frame_model);
            }
        }
        return;
    }
}
