#include "common.h"
#include "sound.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscPitfall(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:414, 106 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     reg   $s2       struct TPitfall * param
 *     reg   $s0       struct ModelType * model
 *     reg   $s1       short w
 *     reg   $s0       int r
 *     reg   $a1       int type
 *     reg   $v0       int t
 *
 * Globals it touches, as the original declared them:
 *     extern struct MISC__184fake PitfallData[2];
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

#include "item.h"
#include "misc.h"

extern char fmt_unknown_pitfall_type[]; /* unknown pitfall type %d */
extern ModelType *LoadModel(u_long *adr);
extern void DisposeModel(ModelType *model);
extern short DrawModel(ModelType *objp);

void ProcMiscPitfall(TMisc *m, TMiscMessage msg)
{
    TPitfall *param;
    short w;

    param = &m->param.pitfall;
    switch (msg)
    {
    case MM_CREATE:
    {
        int type;
        int t;

        type = m->param.hinged_init.type;
        t = m->param.hinged_init.rotation;
        if (type >= N_PITFALL_TYPES)
        {
            AdtMessageBox(fmt_unknown_pitfall_type, type);
            type = PITFALL_KIND_OTO_LEFT;
        }
        m->mode = PITFALL_MODE_CLOSED;
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
    }
        return;

    case MM_DESTROY:
        DeleteConflict(param->locate);
        DisposeModel(param->locate);
        return;

    case MM_PAUSE:
        DeleteConflict(param->locate);
        return;

    case MM_RESUME:
    {
        int conflict_id;

        w = PitfallData[param->type].HitSize;
        conflict_id = InsertConflict(param->locate);
        ConflictObject[conflict_id].offset.vx = 0;
        ConflictObject[conflict_id].offset.vy = 0;
        ConflictObject[conflict_id].offset.vz = 0;
        ConflictObject[conflict_id].common = (void *)CONFLICT_OWNER_DOOR;
        ConflictObject[conflict_id].size.pad =
            CONFLICT_SOFT;
        ConflictObject[conflict_id].size.vx = w;
        ConflictObject[conflict_id].size.vy =
            ConflictObject[conflict_id].size.vz = (w / 3) * 2;
    }
        return;

    default:
    {
        ModelType *model;
        ConflictObjectType *conflict;
        int conflict_id;
        int mode;

        /* The promoted temporary selects signed slti after the lbu. */
        mode = m->mode;
        if (mode != PITFALL_MODE_OPENING)
        {
            if (mode < PITFALL_MODE_OPEN)
            {
                if (mode == PITFALL_MODE_CLOSED)
                {
                    if ((param->locate->attribute & MODEL_ATTR_CONFLICT) != 0)
                    {
                        /* Preserve the array base across the call. */
                        conflict = ConflictObject;
                        conflict_id =
                            GetConflictResult(param->locate, CONFLICT_NONE);
                        if (conflict[conflict_id].common !=
                            (void *)CONFLICT_OWNER_DOOR)
                        {
                            m->mode++;
                            SoundEx(MODEL_POSITION(param->locate), SE_MECHANISM);
                        }
                    }
                }
            }
        }
        else
        {
            param->r += 0xaa;
            if (param->r >= ANGLE_QUADRANT)
            {
                param->r = ANGLE_QUADRANT;
                m->mode++;
            }
        }

        model = PitfallData[param->type].Model[0];
        w = PitfallData[param->type].HitSize;
        if (model != (ModelType *)MODEL_ARCHIVE_NONE)
        {
            model->locate.super = &param->locate->locate;
            model->locate.coord.t[0] = -w;
            model->locate.coord.t[1] = 0;
            model->locate.coord.t[2] = 0;
            model->rotate.vz = param->r;
            UpdateCoordinate(model);
            DrawModel(model);
        }
        model = PitfallData[param->type].Model[1];
        if (model != (ModelType *)MODEL_ARCHIVE_NONE)
        {
            model->locate.super = &param->locate->locate;
            model->locate.coord.t[0] = w;
            model->locate.coord.t[1] = 0;
            model->locate.coord.t[2] = 0;
            model->rotate.vz = -param->r;
            UpdateCoordinate(model);
            DrawModel(model);
        }
    }
        return;
    }
}
