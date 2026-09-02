#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"
#include "effect.h"

extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);
extern void DrawFrame(TEffectSlot *ef);

extern SVECTOR svec_y_n60[];

/*
 * Spawns either a napalm request or a body-attached frame and bleed effect.
 * The union reflects mutually exclusive stack scratch used by the two paths.
 * Keeping the body position aliases split across rand(), and retaining a
 * named pool-result `slot`, reproduces the original register lifetimes.
 * svec_y_n60 intentionally has unknown array size: a typed object declaration
 * changes the old compiler's address materialization and instruction schedule.
 */
void spawn_damage_effect_(Humanoid *human, DamageEffectKind kind)
{
    union
    {
        PARAM_ITEM_LAUNCH launch;
        struct
        {
            VECTOR pos;
            union
            {
                VECTOR random_pos;
                SVECTOR direction;
            } scratch;
        } blood;
    } work;

    if (kind != DAMAGE_EFFECT_ATTACHED_FLASH)
    {
        s32 x;
        s32 y;
        s32 z;
        s32 vx;
        s32 vz;

        work.launch.type = ITEM_NAPALM;
        work.launch.user = human;
        /* The start.vy/end.vx/vy/vz double stores below are retail's own
         * (both writes of each pair are in the bytes). */
        x = human->model->locate.coord.t[0];
        work.launch.start.vx = x;
        y = human->model->locate.coord.t[1];
        work.launch.start.vy = y;
        z = human->model->locate.coord.t[2];
        work.launch.start.vz = z;
        work.launch.start.vy = y - 100;
        work.launch.end.vx = x;
        work.launch.end.vy = y - 100;
        work.launch.end.vz = z;
        /* vx/vz staging: byte-required (inlining the reads recolors the
         * store registers; measured). */
        vx = human->vector.vx;
        work.launch.end.vx = x + vx;
        vz = human->vector.vz;
        work.launch.end.vy = y - 115;
        work.launch.end.vz = z + vz;
        ReqItemUse(&work.launch);
    }
    else
    {
        ModelType **objects;
        ModelType *model;
        VECTOR *position_base;
        VECTOR *position;
        short time;
        int idx;
        int count;
        TEffectSlot *slot;
        FrameType *frame;

        objects = human->model->object;
        if (human->model->n > 0)
        {
            objects += rand() % human->model->n;
        }
        model = *objects;

        memset(&work.blood.scratch.random_pos, 0, sizeof(VECTOR));
        work.blood.scratch.random_pos.vx = rand() % 200 - 100;
        work.blood.scratch.random_pos.vy = rand() % 200 - 100;
        work.blood.scratch.random_pos.vz = rand() % 200 - 100;
        work.blood.pos = work.blood.scratch.random_pos;
        position_base = &work.blood.pos;

        work.blood.scratch.direction = svec_y_n60[0];
        time = rand() % 60 + 60;
        position = position_base;

        idx = EFFECT_CURSOR_;
        count = 0;
        do
        {
            idx++;
            if (idx >= N_EFFECT_SLOTS)
            {
                idx = 0;
            }
            count++;
            if (EffectSlot[idx].proc == 0)
            {
                EFFECT_CURSOR_ = idx + 1;
                if (EFFECT_CURSOR_ >= N_EFFECT_SLOTS)
                {
                    EFFECT_CURSOR_ = 0;
                }
                slot = &EffectSlot[idx];
                goto found;
            }
        } while (count < N_EFFECT_SLOTS);
        slot = &dmy;
    found:
        frame = &slot->param.frame;
        frame->px = position->vx;
        frame->py = position->vy;
        frame->pz = position->vz;
        frame->mode = FRAME_MODE_FLASH;
        frame->size = 3 * FIXED_ONE;
        frame->progress.countdown = time;
        frame->super = &model->locate;
        slot->proc = DrawFrame;

        SetBleedsDir(GetAbsolutePosition(model, 0, 0, 0),
                     &work.blood.scratch.direction,
                     100, 10, 30, RGB24(100, 100, 60));
        SoundEx((VECTOR *)human->model->locate.coord.t, SE_LIGHTNING);
    }
}
