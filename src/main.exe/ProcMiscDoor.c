#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "misc.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscDoor(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:294, 116 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s1       struct TDoor * param
 *     reg   $s2       int r
 *     reg   $a1       int type
 *     reg   $v0       int t
 *     reg   $v0       int cid
 *     reg   $v0       int dir
 *     reg   $s2       int w
 *     reg   $s0       struct ModelType * model
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct MISC__183fake DoorData[11];
 * END PSX.SYM */

/*
 * Matching notes:
 *  - The message dispatch is a real switch. expand_case emits the target's
 *    destroy/create/pause/resume test order while the bodies remain in their
 *    readable create/destroy/pause/resume/control order.
 *  - Keep the ratan2 result and the door's current rotation as two source
 *    statements (`t = ...; t += ...;`). That real producer/update boundary
 *    gives PSX.SYM's `t` its target allocation priority. Folding both into
 *    one expression previously required a fake unsigned self-identity and an
 *    invented `wrap` local; the wrapped remainder is naturally one expression.
 *  - `__builtin_abs` is intentional.  This build disables ordinary builtin
 *    folding, while the explicit builtin produces the target's inline
 *    bgez/nop/negu sequence and the required DoorData register allocation.
 */

extern char fmt_unknown_door_type[]; /* unknown door type %d */
extern ModelType *LoadModel(u_long *adr);
extern void DisposeModel(ModelType *model);
extern short DrawModel(ModelType *objp);

enum
{
    DOOR_ANGLE_STEP = ANGLE_FULL / 64,
    DOOR_OPEN_ANGLE = ANGLE_QUADRANT - DOOR_ANGLE_STEP,
    DOOR_OPENING_OFFSET_DIVISOR = 5 * ANGLE_HALF
};

void ProcMiscDoor(TMisc *m, TMiscMessage msg)
{
    TDoor *param;

    param = &m->param.door;
    switch (msg)
    {
    case MM_CREATE:
{
    s32 type;
    s32 t;

    type = m->param.hinged_init.type;
    t = m->param.hinged_init.rotation;
    if (type >= N_DOOR_TYPES)
    {
        AdtMessageBox(fmt_unknown_door_type, type);
        type = 0;
    }
    m->mode = DOOR_MODE_IDLE;
    param->r = 0;
    param->type = type;
    param->locate = LoadModel(0);
    param->locate->locate.coord.t[0] = m->x;
    param->locate->locate.coord.t[1] = m->y;
    param->locate->locate.coord.t[2] = m->z;
    param->locate->rotate.vx = 0;
    param->locate->rotate.vy = t;
    param->locate->rotate.vz = 0;
    UpdateCoordinate(param->locate);
    return;
}

    case MM_DESTROY:
        DeleteConflict(param->locate);
        DisposeModel(param->locate);
        return;

    case MM_PAUSE:
        DeleteConflict(param->locate);
        return;

    case MM_RESUME:
{
    s32 cid;
    s32 t;
    s16 w;

    cid = InsertConflict(param->locate);
    ConflictObject[cid].offset.components.x = 0;
    t = DoorData[param->type].HitSize;
    ConflictObject[cid].offset.components.z = 0;
    ConflictObject[cid].offset.components.y = -t / 2;
    w = DoorData[param->type].HitSize;
    ConflictObject[cid].common.tag = CONFLICT_OWNER_DOOR;
    ConflictObject[cid].size.components.class_flags = CONFLICT_SOFT;
    ConflictObject[cid].size.components.y = w;
    w = (w / 3) * 2;
    ConflictObject[cid].size.components.z =
        ConflictObject[cid].size.components.x = w;
    param->r = 0;
    return;
}

    default:
{
    s32 w;
    MiscModelReference model;

    switch (m->mode)
    {
    case DOOR_MODE_IDLE:
        if ((param->locate->attribute & MODEL_ATTR_CONFLICT) != 0)
        {
            s32 cid;

            cid = GetConflictResult(param->locate, CONFLICT_NONE);
            if (ConflictObject[cid].common.tag != CONFLICT_OWNER_DOOR)
            {
                s32 t;
                s32 dir;

                t = ratan2(
                    ConflictObject[cid].position.vz -
                        param->locate->locate.coord.t[2],
                    ConflictObject[cid].position.vx -
                        param->locate->locate.coord.t[0]);
                t += param->locate->rotate.vy;
                /* dir stages the predicate before the speed: byte-required
                 * (a plain if/else puts the store in a1, not v0; measured). */
                dir = ((t + 2 * ANGLE_FULL) % ANGLE_FULL) <= ANGLE_HALF;
                if (dir != 0)
                    dir = DOOR_ANGLE_STEP;
                else
                    dir = -DOOR_ANGLE_STEP;
                param->dr = dir;
                m->mode++;
                if (param->r == 0)
                    SoundEx((VECTOR *)param->locate->locate.coord.t, SE_MECHANISM);
            }
        }
        break;

    case DOOR_MODE_OPENING:
    {
        s32 r;

        r = param->r;
        if (r < 0)
            r = -r;
        if (r < DOOR_OPEN_ANGLE)
            param->r += param->dr;
        else
            m->mode = DOOR_MODE_IDLE;
    }
    break;
    }
    {
        s32 r;

        w = DoorData[param->type].HitSize;
        r = __builtin_abs(param->r);
        w -= (w * r) / DOOR_OPENING_OFFSET_DIVISOR;
        model = DoorData[param->type].Model[0];
    }
    if (model.archive_id != MODEL_ARCHIVE_NONE)
    {
        GsCOORDINATE2 *parent;

        /* Staged parent pointer: byte-required (the direct &->locate
         * store recolors the address; measured). */
        parent = &param->locate->locate;
        model.model->locate.coord.t[0] = -w;
        model.model->locate.coord.t[1] = 0;
        model.model->locate.coord.t[2] = 0;
        model.model->locate.super = parent;
        model.model->rotate.vy = param->r;
        UpdateCoordinate(model.model);
        DrawModel(model.model);
    }

    model = DoorData[param->type].Model[1];
    if (model.archive_id != MODEL_ARCHIVE_NONE)
    {
        GsCOORDINATE2 *parent;

        parent = &param->locate->locate;
        model.model->locate.coord.t[0] = w;
        model.model->locate.coord.t[1] = 0;
        model.model->locate.coord.t[2] = 0;
        model.model->locate.super = parent;
        model.model->rotate.vy = -param->r;
        UpdateCoordinate(model.model);
        DrawModel(model.model);
    }
}
    }
}
