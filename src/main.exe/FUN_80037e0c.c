#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);
extern void DrawFrame(TEffectSlot *ef);
extern void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n,
                         int time, long col);

extern u8 D_80097A58[];

/*
 * Spawns either a napalm request or a body-attached frame and bleed effect.
 * The union reflects mutually exclusive stack scratch used by the two paths.
 * Keeping the body position aliases split across rand(), and retaining a
 * separate pool-result pointer, reproduces the original register lifetimes.
 * D_80097A58 intentionally has unknown array size: a typed object declaration
 * changes the old compiler's address materialization and instruction schedule.
 */
void FUN_80037e0c(Humanoid *human, int mode)
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

    if (mode != 0)
    {
        s32 x;
        s32 y;
        s32 z;
        s32 vx;
        s32 vz;

        work.launch.type = 0x16;
        work.launch.user = human;
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
        TEffectSlot *base;
        TEffectSlot *slot;
        TEffectSlot *found_slot;
        FrameType *frame;

        objects = human->model->object;
        if (0 < human->model->n)
        {
            objects = objects + rand() % human->model->n;
        }
        model = *objects;

        memset(&work.blood.scratch.random_pos, 0, sizeof(VECTOR));
        work.blood.scratch.random_pos.vx = rand() % 200 - 100;
        work.blood.scratch.random_pos.vy = rand() % 200 - 100;
        work.blood.scratch.random_pos.vz = rand() % 200 - 100;
        work.blood.pos = work.blood.scratch.random_pos;
        position_base = &work.blood.pos;

        work.blood.scratch.direction = *(SVECTOR *)D_80097A58;
        time = rand() % 60 + 60;
        position = position_base;

        idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
        count = 0;
        base = EffectSlot;
        slot = base + idx;
        do
        {
            idx = idx + 1;
            slot = slot + 1;
            if (199 < idx)
            {
                slot = base;
                idx = 0;
            }
            count = count + 1;
            if (slot->proc == 0)
            {
                CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
                if (199 < idx + 1)
                {
                    CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
                }
                found_slot = slot;
                goto found;
            }
        } while (count < 200);
        found_slot = &dmy;
found:
        idx = 0;
        frame = &found_slot->param.frame;
        frame->px = position->vx;
        count = idx;
        frame->py = position->vy;
        frame->pz = position->vz;
        frame->mode = 0;
        frame->size = 0x3000;
        frame->count = time;
        frame->super = &model->locate;
        found_slot->proc = (void (*)())DrawFrame;

        SetBleedsDir(GetAbsolutePosition(model, idx, count, idx),
                     &work.blood.scratch.direction,
                     100, 10, 30, 0x64643C);
        SoundEx((VECTOR *)human->model->locate.coord.t, 0x39);
    }
}
