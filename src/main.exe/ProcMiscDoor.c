#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscDoor(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:294, 116 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * ProcMiscDoor (0x8004c738) — the misc-object handler for a two-leaf
 * door, dispatched on TMiscMessage. MM_CREATE takes the door type from
 * the init parameters (types above 10 print the unknown-door box and
 * fall back to 0), zeroes the swing angle, and hangs a bare
 * LoadModel(0) locate at the object's x/y/z with the requested yaw.
 * MM_DESTROY deletes the conflict box and disposes that model,
 * MM_PAUSE only deletes the box, and MM_RESUME reinserts it: a
 * CONFLICT_SOFT slot owned by CONFLICT_OWNER_DOOR, as tall as the
 * type's HitSize, offset down by half of it, two thirds as wide and
 * deep, with the angle reset. Every other message runs the control
 * path. Mode 0 waits for a conflict result whose owner is not another
 * door, then uses ratan2 from the door to the pusher plus the door's
 * own yaw, wrapped into one turn, to pick a swing direction of +64 or
 * -64 per frame, steps to mode 1 and plays sound 0x40 if the door was
 * still closed. Mode 1 adds that step until the angle's magnitude
 * reaches 960, then returns to mode 0. Both modes fall into the draw
 * tail, where each leaf's inward offset shrinks from HitSize in
 * proportion to the angle over 10240; leaf 0 is parented to the locate
 * at -offset with +angle and leaf 1 at +offset with -angle, each
 * updated and drawn, and a leaf whose DoorData model pointer is -1 is
 * skipped.
 */

/*
 * Matching notes:
 *  - The message tests follow their physical target order; the labels keep the
 *    create/destroy/pause/resume bodies readable without changing that CFG.
 *  - The one-shot loop around the angle calculation emits no loop at runtime.
 *    Its RTL loop note gives `t` the original allocation priority, allowing
 *    `t`, `angle`, and `dir` to reuse $v0 at their non-overlapping lifetimes.
 *  - `__builtin_abs` is intentional.  This build disables ordinary builtin
 *    folding, while the explicit builtin produces the target's inline
 *    bgez/nop/negu sequence and the required DoorData register allocation.
 */

extern char fmt_unknown_door_type[]; /* unknown door type %d */
extern ModelType *LoadModel(u_long *adr);
extern void DisposeModel(ModelType *model);
extern short DrawModel(ModelType *objp);

void ProcMiscDoor(TMisc *m, TMiscMessage msg)
{
    TDoor *param;

    param = &m->param.door;
    if (msg == MM_DESTROY)
        goto do_destroy;
    if (msg == MM_CREATE)
        goto do_create;
    if (msg == MM_PAUSE)
        goto do_pause;
    if (msg == MM_RESUME)
        goto do_resume;
    goto do_control;

do_create:
{
    s32 type;
    s32 t;

    type = m->param.init.b;
    t = m->param.init.a;
    if (type > 10)
    {
        AdtMessageBox(fmt_unknown_door_type, type);
        type = 0;
    }
    m->mode = 0;
    param->r = 0;
    param->type = (u8)type;
    param->locate = LoadModel(0);
    param->locate->locate.coord.t[0] = m->x;
    param->locate->locate.coord.t[1] = m->y;
    param->locate->locate.coord.t[2] = m->z;
    param->locate->rotate.vx = 0;
    param->locate->rotate.vy = (s16)t;
    param->locate->rotate.vz = 0;
    UpdateCoordinate(param->locate);
    return;
}

do_destroy:
    DeleteConflict(param->locate);
    DisposeModel(param->locate);
    return;

do_pause:
    DeleteConflict(param->locate);
    return;

do_resume:
{
    s32 cid;
    s32 t;
    s16 w;

    cid = InsertConflict(param->locate);
    ConflictObject[cid].offset.vx = 0;
    t = DoorData[param->type].HitSize;
    ConflictObject[cid].offset.vz = 0;
    ConflictObject[cid].offset.vy = -t / 2;
    w = DoorData[param->type].HitSize;
    ConflictObject[cid].common = CONFLICT_OWNER_DOOR;
    ConflictObject[cid].size.pad = CONFLICT_SOFT;
    ConflictObject[cid].size.vy = w;
    w = (s16)(w / 3) * 2;
    ConflictObject[cid].size.vx = w;
    ConflictObject[cid].size.vz = w;
    param->r = 0;
    return;
}

do_control:
{
    s32 w;
    ModelType *model;

    switch (m->mode)
    {
    case 0:
        if ((param->locate->attribute & 0x8000 /* MODEL_ATTR_CONFLICT */) != 0)
        {
            s32 cid;

            cid = GetConflictResult(param->locate, -1);
            if (ConflictObject[cid].common != CONFLICT_OWNER_DOOR)
            {
                s32 t;
                s32 angle;
                s32 wrap;
                s32 dir;

                do
                {
                    t = ratan2(
                            ConflictObject[cid].position.vz - param->locate->locate.coord.t[2],
                            ConflictObject[cid].position.vx - param->locate->locate.coord.t[0]) +
                        param->locate->rotate.vy;
                } while (0);
                wrap = t + 0x2000;
                angle = wrap;
                if (wrap < 0)
                    angle = t + 0x2fff;
                dir = wrap - ((angle >> 12) << 12) < 0x801;
                if (dir != 0)
                    dir = 0x40;
                else
                    dir = -0x40;
                param->dr = dir;
                m->mode++;
                if (param->r == 0)
                    SoundEx((VECTOR *)param->locate->locate.coord.t, 0x40);
            }
        }
        break;

    case 1:
    {
        s32 r;

        r = param->r;
        if (r < 0)
            r = -r;
        if (r < 960)
            param->r += param->dr;
        else
            m->mode = 0;
    }
    break;
    }
{
    s32 r;

    w = DoorData[param->type].HitSize;
    r = __builtin_abs(param->r);
    w = w - (w * r) / 0x2800;
    model = DoorData[param->type].Model[0];
}
    if (model != (ModelType *)-1)
    {
        GsCOORDINATE2 *parent;

        parent = &param->locate->locate;
        model->locate.coord.t[0] = -w;
        model->locate.coord.t[1] = 0;
        model->locate.coord.t[2] = 0;
        model->locate.super = parent;
        model->rotate.vy = param->r;
        UpdateCoordinate(model);
        DrawModel(model);
    }

    model = DoorData[param->type].Model[1];
    if (model != (ModelType *)-1)
    {
        GsCOORDINATE2 *parent;

        parent = &param->locate->locate;
        model->locate.coord.t[0] = w;
        model->locate.coord.t[1] = 0;
        model->locate.coord.t[2] = 0;
        model->locate.super = parent;
        model->rotate.vy = -param->r;
        UpdateCoordinate(model);
        DrawModel(model);
    }
}
}
