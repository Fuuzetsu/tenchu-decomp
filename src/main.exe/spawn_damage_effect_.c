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
 * The outer union reflects mutually exclusive stack scratch used by the two
 * paths. In the attached-flash path, the completed random-position vector is
 * reused as the short bleed direction. Keeping the body position aliases
 * split across rand(), and retaining a named pool-result `slot`, reproduces
 * the original register lifetimes.
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
            VECTOR scratch;
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

        memset(&work.blood.scratch, 0, sizeof(VECTOR));
        work.blood.scratch.vx = rand() % 200 - 100;
        work.blood.scratch.vy = rand() % 200 - 100;
        work.blood.scratch.vz = rand() % 200 - 100;
        work.blood.pos = work.blood.scratch;
        position_base = &work.blood.pos;

        *(SVECTOR *)&work.blood.scratch = svec_y_n60[0];
        time = rand() % 60 + 60;
        position = position_base;

        FIND_EFFECT_SLOT(idx, count, slot, found);
    found:
        frame = &slot->param.frame;
        frame->px = position->vx;
        frame->py = position->vy;
        frame->pz = position->vz;
        frame->mode = FRAME_MODE_FLASH;
        frame->size = 3 * FIXED_ONE;
        frame->count = time;
        frame->super = &model->locate;
        slot->proc = DrawFrame;

        SetBleedsDir(GetAbsolutePosition(model, 0, 0, 0),
                     (SVECTOR *)&work.blood.scratch,
                     100, 10, 30, RGB24(100, 100, 60));
        SoundEx((VECTOR *)human->model->locate.coord.t, SE_LIGHTNING);
    }
}
