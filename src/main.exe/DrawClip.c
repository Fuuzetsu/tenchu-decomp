#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "tmdfast.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long DrawClip(struct ModelType *objp, long *xy);
 *     3DCTRL.C:240, 23 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * objp
 *     param $a1       long * xy
 *     stack sp+16     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

long DrawClip(ModelType *objp, long *xy)
{
    u16 attr;
    long sz;
    long result;
    s32 iv;
    short rxy[2];

    attr = objp->attribute;
    if ((attr & MODEL_ATTR_HIDDEN) != 0)
        goto reject;
    if ((attr & MODEL_ATTR_NOCULL) == 0)
    {
        sz = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0) >> 2;
        if ((attr & MODEL_ATTR_CULL_BEHIND) != 0 && sz == 0)
        {
            result = -1;
            goto ret;
        }
        if ((attr & MODEL_ATTR_CULL_SCREEN) != 0)
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
                result = -1;
                goto ret;
            }
        }
        if ((attr & MODEL_ATTR_CULL_FAR) != 0 && sz > DEPTH_LIMIT)
        {
            result = -1;
            goto ret;
        }
    }
    sz = RotTransPers(&UnitVector, xy, 0, 0) >> 2;
    if (sz > DEPTH_LIMIT)
    {
    reject:
        result = -1;
        goto ret;
    }
    if (xy != 0)
    {
        result = sz;
        goto ret;
    }
    if (sz >= FOG_DEPTH)
        DrawTMDmode = TMD_BANK_FOG;
    else
        DrawTMDmode = TMD_BANK_PLAIN;
    result = sz;
ret:
    return result;
}
