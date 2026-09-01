#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "tmdfast.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawModel(struct ModelType *objp);
 *     3DCTRL.C:297, 11 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct ModelType * objp
 *     stack sp+16     struct MATRIX mat
 *     reg   $s1       struct ModelType * objp
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+48     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * STATUS: MATCHING.
 *
 * DrawModel (0x80017248) — same TU as DrawClip.c/UpdateCoordinate.c/
 * GetAbsolutePosition.c/DrawOrnament.c (3DCTRL.C): DrawClip's full-bodied
 * twin — builds the model's local screen matrix (GsGetLs+GsSetLsMatrix,
 * DrawOrnament's pair) then runs the exact same visibility/clip gauntlet
 * as DrawClip (the MODEL_ATTR_CULL_* gauntlet, UnitVector RotTransPers,
 * DrawTMDmode), and on success actually calls DrawTMD; returns 1 drawn / 0
 * not.
 *
 * Matching constraints:
 *  - The screen-cull block falls into the far-depth test, which falls into
 *    UnitVector projection. Keep ret as the shared draw tail.
 *  - sz is PSX.SYM's one end-to-end value: first projection OTZ, -1 reject
 *    sentinel, and second projection OTZ. iv is the separate transient
 *    absolute box-coordinate value.
 *  - The MODEL_ATTR_CULL_FAR test reads the old sz before assigning -1 and
 *    jumping.
 *    Ghidra's comma rendering reflects a delay-slot store, not source order.
 *  - Preserve two literal tail returns. return sz != -1 materializes an
 *    unwanted boolean.
 *  - Both box-threshold failures go directly to reject; they do not project
 *    UnitVector. The far-depth test remains after the box body so its three
 *    incoming edges share one physical test.
 *  - Spell the fog choice as if (sz >= FOG_DEPTH) fog; else plain. Swapping
 *    the equivalent arms changes which block reaches the following shared
 *    tail without a jump.
 *  - Pin the shared reject assignment with a real reject label inside the
 *    UnitVector depth guard. Attribute-4 and both box failures jump to it.
 *    The far-depth reject stays separate so its -1 assignment can occupy
 *    its own branch delay slot.
 */
extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);

short DrawModel(ModelType *objp)
{
    MATRIX mat;
    ModelAttribute atr;
    long sz;
    s32 iv;
    short rxy[2];

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    atr = objp->attribute;
    sz = -1;
    if ((atr & MODEL_ATTR_HIDDEN) != 0)
        goto ret;
    if ((atr & MODEL_ATTR_NOCULL) == 0)
    {
        sz = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0) >> 2;
        
        if ((atr & MODEL_ATTR_CULL_BEHIND) == 0 || sz != 0)
        {
            if ((atr & MODEL_ATTR_CULL_SCREEN) != 0)
            {
                iv = rxy[0];
                if (iv < 0)
                {
                    iv = -iv;
                }
                if (iv <= MODEL_CULL_X_LIMIT)
                {
                    iv = rxy[1];
                    if (iv < 0)
                    {
                        iv = -iv;
                    }
                    if (iv > MODEL_CULL_Y_LIMIT)
                    {
                        goto reject;
                    }
                }
                else
                {
                    goto reject;
                }
            }
            if ((atr & MODEL_ATTR_CULL_FAR) != 0 && sz > DEPTH_LIMIT)
            {
                sz = -1;
                goto ret;
            }
        }
        else
        {
            goto reject;
        }
    }
    sz = RotTransPers(&UnitVector, 0, 0, 0) >> 2;
    if (sz > DEPTH_LIMIT)
    {
    reject:
        sz = -1;
        goto ret;
    }
    if (sz >= FOG_DEPTH)
    {
        DrawTMDmode = TMD_BANK_FOG;
    }
    else
    {
        DrawTMDmode = TMD_BANK_PLAIN;
    }
ret:
    if (sz == -1)
    {
        return 0;
    }
    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}
