#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"
#include "sound.h"
#include "tuning.h"

extern SVECTOR svec_y_n150[];

extern s32 is_humanoid_on_stage_(Humanoid *human);
extern s16 Think1sleep(void);

/* The position builder is overwritten with the smoke velocity after its
 * otherwise-dead aggregate copy.  Keep both meanings visible. */
typedef union
{
    VECTOR random_position_build;
    SVECTOR smoke_velocity;
} ProcItemNemuriEffectWork;

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNemuri(struct tag_TItem *item);
 *     ITEM.C:2738, 93 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       struct VECTOR * pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s1       int env
 *     reg   $v0       int bright
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s3       struct Humanoid * human
 *     reg   $s3       struct Humanoid * human
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     stack sp+48     struct VECTOR pos
 *     reg   $s3       struct Humanoid * human
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * Advances the sleeping-powder projectile, draws its pulsing sprite, puts a
 * collided humanoid to sleep, and disposes the item after impact, expiry, or
 * leaving the area map.
 *
 * Matching notes:
 *  - `conflict_class` shares the collision-mode constant between a halfword and a word
 *    store; two literals produce an extra `li`.
 *  - The byte-identical `bleed_count` arms disappear in jump2, but their CFG keeps
 *    the call-count and colour pseudos out of the pulse's register.  `bleed_count`
 *    is initialized, and is overwritten with its real value before the call.
 *  - The nested zero-trip loops emit no instructions.  Their loop notes weight
 *    `bleed_color` to 9 refs / 49 RTL insns (priority 5510), above `pulse`'s 5 / 35
 *    (2857), selecting the target $t0/$t1 allocation.  Splitting the colour
 *    across the two statements then puts its `lui` in the branch delay slot
 *    and its `ori` at the join.
 *  - `bleed_range` is assigned after the duplicated X update so its $a1 copy
 *    fills that update's load delay.  The full-width `rotation_count` similarly
 *    schedules the count load before the scale store without an `andi 0xff`.
 *  - The cleanup paths cache `item_proc` for the null check but call through
 *    `item->proc`; jump2 cross-jumps them into the target shared tail.
 */
void ProcItemNemuri(TItem *item)
{
    enum
    {
        NEMURI_MODE_START = 0,
        NEMURI_MODE_THROW = 1,
        NEMURI_MODE_FLY = 2,
        NEMURI_MODE_FINISH = 3,
        NO_CONFLICT = -1,
        NEMURI_RELEASE_FRAME = 3,
        NEMURI_COLLISION_SIZE = 1000,
        NEMURI_PULSE_STEP = 0x88,
        NEMURI_PULSE_SHIFT = 6,
        NEMURI_BASE_BRIGHTNESS = 0x80,
        NEMURI_BASE_SCALE = 4 * FIXED_ONE,
        NEMURI_ROTATION_STEP = 45 * FIXED_ONE,
        NEMURI_BLEED_RANGE = 300,
        NEMURI_BLEED_COUNT = 2,
        NEMURI_MAX_FLIGHT_COUNT = 100
    };
    Sprite3D *model;
    param_napalm *param;
    void (*item_proc)(TItem *);
    u8 flight_count;
    s32 rotation_count;

    model = (Sprite3D *)item->model;
    param = &item->param.napalm;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = NEMURI_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case NEMURI_MODE_START:
        SetNowMotion(item->owner, MOT_ITEM_THROW, 1);
        SoundEx((VECTOR *)item->owner->model->locate.coord.t, SE_SLEEP_DART_THROW);
        item->mode++;
        return;

    case NEMURI_MODE_THROW:
        if (item->owner->motion->mid == MOT_ITEM_THROW)
        {
            if (item->owner->motion->count != NEMURI_RELEASE_FRAME)
            {
                return;
            }
            {
                VECTOR *pos;
                s32 new_conflict_id;
                s32 conflict_class;

                pos = GetAbsolutePosition(
                    item->owner->model->object[MODEL_PART_WEAPON_HAND_1],
                    0, 0, 0);
                param->count = 0;
                item->mode++;
                item->locate->locate.coord.t[0] = pos->vx;
                item->locate->locate.coord.t[1] = pos->vy;
                item->locate->locate.coord.t[2] = pos->vz;
                DeleteConflict(item->locate);
                new_conflict_id = InsertConflict(item->locate);
                conflict_class = CONFLICT_SOFT;
                SET_ITEM_COLLISION(new_conflict_id, NEMURI_COLLISION_SIZE,
                                   CONFLICT_OWNER_ITEM, conflict_class);
                return;
            }
        }
        item_proc = item->proc;
        if (item_proc == 0)
        {
            return;
        }
        item->mode = ITEM_MODE_DISPOSE;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != NEMURI_MODE_START)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;

    case NEMURI_MODE_FLY:
    {
        s32 pulse;
        s32 brightness;
        s32 conflict_id;
        s32 inactive_sentinel;
        s32 bleed_color;
        s32 bleed_count = 0;
        s32 bleed_range;
        Humanoid *hit_human;

        pulse = rsin(param->count * NEMURI_PULSE_STEP);
        if (pulse < 0)
        {
            pulse += (1 << NEMURI_PULSE_SHIFT) - 1;
        }
        bleed_color = RGB24(110, 0, 0);
        bleed_color |= RGB24(0, 110, 110);
        item->locate->locate.coord.t[0] +=
            item->param.napalm.vec.vx;
        bleed_range = NEMURI_BLEED_RANGE;
        bleed_count = NEMURI_BLEED_COUNT;
        item->locate->locate.coord.t[1] += param->vec.vy;
        item->locate->locate.coord.t[2] += param->vec.vz;
        brightness = (pulse >> NEMURI_PULSE_SHIFT) +
                     NEMURI_BASE_BRIGHTNESS;
        model->sprite.r = brightness;
        model->sprite.g = brightness;
        model->sprite.b = brightness;
        rotation_count = param->count;
        model->scale = brightness * 2 + NEMURI_BASE_SCALE;
        model->sprite.rotate = rotation_count * NEMURI_ROTATION_STEP;
        SetBleeds((VECTOR *)item->locate->locate.coord.t,
                  bleed_range, 10, bleed_count, 10, bleed_color);

        flight_count = param->count + 1;
        param->count = flight_count;
        if (flight_count > NEMURI_MAX_FLIGHT_COUNT)
        {
            item->mode++;
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            conflict_id = NO_CONFLICT;
        }
        else
        {
            conflict_id = GetConflictResult(item->locate, NO_CONFLICT);
        }
        inactive_sentinel = NO_CONFLICT;
        if (conflict_id != inactive_sentinel)
        {
            hit_human = (Humanoid *)ConflictObject[conflict_id].common;
            if (is_humanoid_on_stage_(hit_human) != 0 &&
                hit_human != item->owner)
            {
                VECTOR random_position;
                ProcItemNemuriEffectWork effect_work;
                VECTOR smoke_position;
                VECTOR smoke_position_build;
                SVECTOR *smoke_velocity;
                s16 hit_life;

                if (hit_human->model->n > 0)
                {
                    rand();
                }
                smoke_velocity = &effect_work.smoke_velocity;
                memset(&effect_work.random_position_build, 0, sizeof(VECTOR));
                effect_work.random_position_build.vx = rand() % 200 - 100;
                effect_work.random_position_build.vy = rand() % 200 - 100;
                effect_work.random_position_build.vz = rand() % 200 - 100;
                /* Dead copy, but retail's own: the 12-byte struct copy is in
                 * the shipped bytes (removal measures -32). The jittered
                 * position is computed and then never passed anywhere --
                 * SetSmoke below gets the body position instead. */
                random_position = effect_work.random_position_build;
                SoundEx((VECTOR *)item->locate->locate.coord.t, SE_SMOKE_PUFF);

                *smoke_velocity = svec_y_n150[0];
                memset(&smoke_position_build, 0, sizeof(VECTOR));
                smoke_position_build.vx = hit_human->model->locate.coord.t[0];
                smoke_position_build.vy = hit_human->model->locate.coord.t[1];
                smoke_position_build.vz = hit_human->model->locate.coord.t[2];
                smoke_position = smoke_position_build;
                SetSmoke(&smoke_position, smoke_velocity, 10, 30);

                hit_life = hit_human->life;
                if (hit_life > 0 && hit_human->motion->mid != MOT_ACTION)
                {
                    if ((hit_human->type & PAGE_MASK) != PAGE_BOSS &&
                        hit_life != inactive_sentinel)
                    {
                        EquipWeapon(hit_human, 0);
                        SetNowMotion(hit_human, MOT_STATE_SHEATHE, 1);
                        hit_human->think[0] = Think1sleep;
                        hit_human->attribute &= ~ATTR_PHASE;
                    }
                    SetNowMotion(hit_human, MOT_ACTION, 1);
                    Sound(hit_human, CHAR_VOICE_HURT);
                }

                item_proc = item->proc;
                if (item_proc == 0)
                {
                    return;
                }
                item->mode = ITEM_MODE_DISPOSE;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != NEMURI_MODE_START)
                {
                    AdtMessageBox(msg_item_dispose_fail, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
                return;
            }
        }

        if (GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2], 0) ==
            LEVEL_NONE)
        {
            item_proc = item->proc;
            if (item_proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        break;
    }

    case NEMURI_MODE_FINISH:
        item_proc = item->proc;
        if (item_proc == 0)
        {
            return;
        }
        item->mode = ITEM_MODE_DISPOSE;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != NEMURI_MODE_START)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;
    }

    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
}
