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
