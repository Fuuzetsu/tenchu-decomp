#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

typedef union
{
    struct
    {
        SVECTOR velocity;
        VECTOR position;
        VECTOR position_build;
    } explosion;
    struct
    {
        VECTOR position;
        VECTOR random_position_build;
    } frame;
} ProcItemJiraiScratch;

extern SVECTOR svec_y_n25[]; /* {0,-25,0} */

extern s32 is_humanoid_on_stage_(Humanoid *human);
extern void reset_alert_duration(void);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemJirai(struct tag_TItem *item);
 *     ITEM.C:3527, 78 src lines, frame 104 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_TItem * item
 *     reg   $s6       struct Sprite3D * model
 *     reg   $v1       struct param_smoke * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s4       struct Humanoid * human
 *     reg   $s4       struct Humanoid * human
 *     reg   $s3       int i
 *     reg   $s0       struct ModelType * model
 *     stack sp+24     struct VECTOR pos
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

/*
 * Advances the placed landmine through floor placement, collision arming,
 * detonation, and its ten-frame-effect burst before disposal.
 *
 * Matching notes:
 *  - The explosion and frame-effect aggregates are mutually exclusive and
 *    share ProcItemJiraiScratch.  This reproduces the target's exact
 *    sp+0x18..sp+0x3f working window and 0x60-byte frame.
 *  - `call_item` makes both disposal predecessors materialize the indirect
 *    call argument before entering their shared tail; calling
 *    `item_proc(item)` instead fills the jalr delay slot and removes one of
 *    those moves.
 *  - The zero-trip wrapper around `frame_index = 0` keeps initialization
 *    after the character-state call, where it fills the following branch
 *    delay slot. That loop note initially gave `frame_index` the allocator's
 *    preferred saved register, so the second zero-trip wrapper weights the
 *    three `% 200` expressions more heavily. The generated division constant
 *    then takes $s1 and leaves the target $s3 for `frame_index`, without
 *    emitting extra code.
 *  - This function needs maspsx `--expand-div` for the dynamic model-count
 *    remainder guard; Build.hs and permute.py carry the mirrored flag.
 */

void ProcItemJirai(TItem *item)
{
    enum
    {
        JIRAI_MODE_PLACE = 0,
        JIRAI_MODE_ARMED = 1,
        JIRAI_MODE_EXPLODE = 2,
        JIRAI_MODE_BLAST = 3,
        JIRAI_BLAST_COUNTDOWN_START = 3,
        JIRAI_FRAME_EFFECT_COUNT = 10,
        JIRAI_COUNTDOWN_END = 0xff
    };
    Sprite3D *sprite;
    param_smoke *param;
    void (*item_proc)(TItem *);
    TItem *call_item;
    ProcItemJiraiScratch scratch;

    sprite = item->model.sprite;
    param = &item->param.smoke;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = JIRAI_MODE_PLACE;
        return;
    }

    switch (item->mode)
    {
    case JIRAI_MODE_PLACE:
    {
        s32 trigger_size;
        ConflictClass conflict_class;
        s32 new_conflict_id;

        item->locate->locate.coord.t[1] =
            GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2],
                            AREA_LEVEL_STEP_DOWN);
        if (item->locate->locate.coord.t[1] == LEVEL_NONE ||
            ((u16)FieldArea->attribute & MAP_WATER) != 0)
        {
            u8 item_count;

            item_count = item->owner->item[item->type];
            if (item_count != ITEM_INFINITE)
            {
                item->owner->item[item->type] = item_count + 1;
            }
            item_proc = item->proc;
            if (item_proc == 0)
            {
                return;
            }
            call_item = item;
            item->mode = ITEM_MODE_DISPOSE;
            goto dispose;
        }

        DeleteConflict(item->locate);
        new_conflict_id = InsertConflict(item->locate);
        trigger_size = 500;
        conflict_class = CONFLICT_SOFT;
        SET_ITEM_COLLISION(new_conflict_id, trigger_size,
                           CONFLICT_OWNER_ITEM, conflict_class);
        item->mode++;
        break;
    }

    case JIRAI_MODE_ARMED:
    {
        s32 conflict_id;

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            conflict_id = CONFLICT_NONE;
        }
        else
        {
            conflict_id = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (conflict_id != CONFLICT_NONE &&
            is_humanoid_on_stage_(
                ConflictObject[conflict_id].common.human) != 0)
        {
            s32 new_conflict_id;
            s32 blast_size;

            DeleteConflict(item->locate);
            new_conflict_id = InsertConflict(item->locate);
            blast_size = 1500;
            SET_ITEM_COLLISION(new_conflict_id, blast_size,
                               CONFLICT_OWNER_ITEM, CONFLICT_HIT);
            item->mode++;
        }
        break;
    }

    case JIRAI_MODE_EXPLODE:
        scratch.explosion.velocity = svec_y_n25[0];
        memset(&scratch.explosion.position_build, 0, sizeof(VECTOR));
        scratch.explosion.position_build.vx =
            item->locate->locate.coord.t[0];
        scratch.explosion.position_build.vy =
            item->locate->locate.coord.t[1];
        scratch.explosion.position_build.vz =
            item->locate->locate.coord.t[2];
        scratch.explosion.position = scratch.explosion.position_build;
        SetExplosion(&scratch.explosion.position,
                     &scratch.explosion.velocity);
        scratch.explosion.velocity.vx = 75;
        scratch.explosion.velocity.vy = 200;
        scratch.explosion.velocity.vz = 75;
        SetHinoko(&scratch.explosion.position,
                  &scratch.explosion.velocity, 10);
        scratch.explosion.velocity.vx = 0;
        scratch.explosion.velocity.vy = -400;
        scratch.explosion.velocity.vz = 0;
        SetSmoke(&scratch.explosion.position,
                 &scratch.explosion.velocity, 20, 6);
        SoundEx(&scratch.explosion.position, SE_EXPLOSION);
        item->mode++;
        param->count = JIRAI_BLAST_COUNTDOWN_START;
        reset_alert_duration();
        break;

    case JIRAI_MODE_BLAST:
    {
        s32 conflict_id;
        s32 blast_countdown;

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            conflict_id = CONFLICT_NONE;
        }
        else
        {
            conflict_id = GetConflictResult(item->locate, CONFLICT_NONE);
        }
        if (conflict_id != CONFLICT_NONE)
        {
            Humanoid *hit_human;
            s32 frame_index;
            s32 human_present;

            hit_human = ConflictObject[conflict_id].common.human;
            human_present = is_humanoid_on_stage_(hit_human);
            /* empty 1-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            frame_index = 0;
            if (human_present != 0)
            {
                while (1)
                {
                    ModelType **model_objects;
                    ModelType *model;

                    if (frame_index >= JIRAI_FRAME_EFFECT_COUNT)
                    {
                        break;
                    }
                    model_objects = hit_human->model->object;
                    if (hit_human->model->n > 0)
                    {
                        model_objects += rand() % hit_human->model->n;
                    }
                    model = *model_objects;
                    memset(&scratch.frame.random_position_build, 0,
                           sizeof(VECTOR));
                    frame_index++;
                    do
                    {
                        scratch.frame.random_position_build.vx =
                            rand() % 200 - 100;
                        scratch.frame.random_position_build.vy =
                            rand() % 200 - 100;
                        scratch.frame.random_position_build.vz =
                            rand() % 200 - 100;
                    } while (0);
                    scratch.frame.position =
                        scratch.frame.random_position_build;
                    SetFrame(&scratch.frame.position, 3 * FIXED_ONE,
                             rand() % 60 + 60,
                             &model->locate);
                }
            }
        }

        blast_countdown = param->count - 1;
        param->count = blast_countdown;
        if ((u8)blast_countdown != JIRAI_COUNTDOWN_END)
        {
            return;
        }
        item_proc = item->proc;
        if (item_proc == 0)
        {
            return;
        }
        call_item = item;
        item->mode = ITEM_MODE_DISPOSE;
    dispose:
        item_proc(call_item);
        DeleteConflict(item->locate);
        if (item->mode != JIRAI_MODE_PLACE)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;
    }
    }

    UpdateCoordinate(item->locate);
    sprite->locate = item->locate->locate;
    DrawSprite(sprite);
}
