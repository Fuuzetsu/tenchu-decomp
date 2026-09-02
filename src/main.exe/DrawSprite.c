#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "tmdfast.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawSprite(struct Sprite3D *sprt);
 *     3DCTRL.C:593, 14 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct Sprite3D * sprt
 *     stack sp+16     struct MATRIX mat
 *     reg   $a2       long sz
 *     reg   $s1       struct ModelType * objp
 *     reg   $s2       long * xy
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+48     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

short DrawSprite(Sprite3D *sprt)
{
    MATRIX mat;
    ModelType *objp;
    ModelAttribute atr;
    long *xy;
    long sz;
    long result;
    long pri;
    s32 iv;
    short rxy[2];

    objp = (ModelType *)sprt;
    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    atr = objp->attribute;
    xy = (long *)&sprt->sprite.x;
    if ((atr & MODEL_ATTR_HIDDEN) != 0)
        goto reject;
    if ((atr & MODEL_ATTR_NOCULL) == 0)
    {
        sz = RotTransPers(&objp->clip, (s32 *)rxy, 0, 0) >> 2;
        if ((atr & MODEL_ATTR_CULL_BEHIND) != 0 && sz == 0)
        {
            result = -1;
            goto ret;
        }
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
                    goto reject;
            }
            else
            {
                result = -1;
                goto ret;
            }
        }
        if ((atr & MODEL_ATTR_CULL_FAR) != 0 && sz > DEPTH_LIMIT)
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
    pri = result - 5;
    if (pri < 1)
    {
        return 0;
    }
    iv = (sprt->scale >> 2) * PROJECTION_DISTANCE;
    sprt->sprite.scalex = sprt->sprite.scaley = (short)(iv / pri);
    GsSortSprite(&sprt->sprite, OTablePt, (u16)pri);
    return 1;
}
