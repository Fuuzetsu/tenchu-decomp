#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"
#include "sound.h"
#include "tuning.h"

extern SVECTOR svec_y_n150[];

extern s32 is_humanoid_on_stage_(Humanoid *human);
extern s16 Think1sleep(void);

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

void ProcItemNemuri(TItem *item)
{
    enum
    {
        NEMURI_MODE_START = 0,
        NEMURI_MODE_THROW = 1,
        NEMURI_MODE_FLY = 2,
        NEMURI_MODE_FINISH = 3,
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
        SetNowMotion(item->owner, MOT_ITEM_THROW, MOTION_MOVE_APPLY);
        SoundEx(MODEL_POSITION(item->owner->model), SE_SLEEP_DART_THROW);
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
                ConflictClass conflict_class;

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
        DISPOSE_ITEM(item);
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
        SetBleeds(MODEL_POSITION(item->locate),
                  bleed_range, 10, bleed_count, 10, bleed_color);

        flight_count = param->count + 1;
        param->count = flight_count;
        if (flight_count > NEMURI_MAX_FLIGHT_COUNT)
        {
            item->mode++;
        }

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            conflict_id = CONFLICT_NONE;
        }
        else
        {
            conflict_id = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        inactive_sentinel = CONFLICT_NONE;
        if (conflict_id != inactive_sentinel)
        {
            hit_human = ConflictObject[conflict_id].common;
            if (is_humanoid_on_stage_(hit_human) != 0 &&
                hit_human != item->owner)
            {
                s16 hit_life;

                if (hit_human->model->n > 0)
                {
                    rand();
                }
                {
                    /* Retail computes this jittered position but never uses it. */
                    VECTOR random_position = {
                        rand() % 200 - 100,
                        rand() % 200 - 100,
                        rand() % 200 - 100
                    };

                    SoundEx(MODEL_POSITION(item->locate), SE_SMOKE_PUFF);
                    {
                        SVECTOR smoke_velocity = svec_y_n150[0];
                        VECTOR smoke_position = {
                            hit_human->model->locate.coord.t[0],
                            hit_human->model->locate.coord.t[1],
                            hit_human->model->locate.coord.t[2]
                        };

                        SetSmoke(&smoke_position, &smoke_velocity, 10, 30);
                    }

                    hit_life = hit_human->life;
                    if (hit_life > 0 && hit_human->motion->mid != MOT_ACTION)
                    {
                        if ((hit_human->type & PAGE_MASK) != PAGE_BOSS &&
                            hit_life != inactive_sentinel)
                        {
                            EquipWeapon(hit_human, WEAPON_SHEATHED);
                            SetNowMotion(hit_human, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
                            hit_human->think[0] = Think1sleep;
                            hit_human->attribute &= ~ATTR_PHASE;
                        }
                        SetNowMotion(hit_human, MOT_ACTION, MOTION_MOVE_APPLY);
                        Sound(hit_human, CHAR_VOICE_HURT);
                    }

                    item_proc = item->proc;
                    if (item_proc == 0)
                    {
                        return;
                    }
                    DISPOSE_ITEM(item);
                    return;
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
        DISPOSE_ITEM(item);
        return;
    }

    UpdateCoordinate(item->locate);
    model->locate = item->locate->locate;
    DrawSprite(model);
}
