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

/*
 * ProcMiscPitfall (0x8004cb6c, 868 bytes) — creates the pitfall collision
 * volume, advances its opening animation, and draws the two trap-door models.
 *
 * The single function-scope `w` is load-bearing.  Although its two switch-arm
 * lifetimes never meet, keeping one source identity makes cc1 allocate it
 * globally and reuse m's $s1 home.  Two block-local `w` declarations instead
 * put the resume value in $s0 before global allocation and displace the
 * shared literal 2 to $s3.
 *
 * Two non-obvious source identities are measured and load-bearing: promoting
 * mode to int changes the lone range test from sltiu to the target's slti
 * (7 -> 6 bytes), and keeping ConflictObject in a block pointer across
 * GetConflictResult changes a 218-instruction draft to the target length.
 */

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

        type = m->param.init.b;
        t = m->param.init.a;
        if (type >= N_PITFALL_TYPES)
        {
            AdtMessageBox(fmt_unknown_pitfall_type, type);
            type = 0;
        }
        m->mode = 0;
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
        int r;

        w = PitfallData[param->type].HitSize;
        r = InsertConflict(param->locate);
        ConflictObject[r].offset.vx = 0;
        ConflictObject[r].offset.vy = 0;
        ConflictObject[r].offset.vz = 0;
        ConflictObject[r].common = CONFLICT_OWNER_DOOR;
        ConflictObject[r].size.pad = CONFLICT_SOFT;
        ConflictObject[r].size.vx = w;
        ConflictObject[r].size.vy = ConflictObject[r].size.vz = (w / 3) * 2;
    }
        return;

    default:
    {
        ModelType *model;
        ConflictObjectType *conflict;
        int r;
        int mode;

        /* The promoted temporary selects signed slti after the lbu. */
        mode = m->mode;
        /* The nested != 1 / < 2 / == 0 tree is byte-required (a flat
         * else-if chain re-shapes the compare tree; measured). */
        if (mode != 1)
        {
            if (mode < 2)
            {
                if (mode == 0)
                {
                    if ((param->locate->attribute & MODEL_ATTR_CONFLICT) != 0)
                    {
                        /* Preserve the array base across the call. */
                        conflict = ConflictObject;
                        r = GetConflictResult(param->locate, -1);
                        if (conflict[r].common != CONFLICT_OWNER_DOOR)
                        {
                            m->mode++;
                            SoundEx((VECTOR *)param->locate->locate.coord.t, SE_MECHANISM);
                        }
                    }
                }
            }
        }
        else
        {
            param->r += 0xaa;
            if (param->r >= 0x400)
            {
                param->r = 0x400;
                m->mode++;
            }
        }

        model = PitfallData[param->type].Model[0];
        w = PitfallData[param->type].HitSize;
        if (model != (ModelType *)-1)
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
        if (model != (ModelType *)-1)
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
