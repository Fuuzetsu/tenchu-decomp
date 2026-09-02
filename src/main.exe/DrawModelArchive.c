#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "tmdfast.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawModelArchive(struct ModelArchiveType *mad, long gap);
 *     3DCTRL.C:393, 28 src lines, frame 88 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct ModelArchiveType * mad
 *     param $s3       long gap
 *     reg   $s0       struct ModelType * objp
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR pos
 *     reg   $s1       short i
 *     reg   $s2       struct ModelType * objp
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+56     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);

short DrawModelArchive(ModelArchiveType *mad, long gap)
{
    MATRIX mat;
    SVECTOR pos; /* unused in retail, but present in the demo symbols */
    ModelAttribute atr;
    long sz;
    long result;
    s32 iv;
    short i;
    ModelType *objp;
    short rxy[2];

    if (SkipFrame != SKIPFRAME_SKIPPED)
    {
        if (gap >= 0)
        {
            GsGetLs(&mad->locate, &mat);
            GsSetLsMatrix(&mat);
            atr = mad->attribute;
            if ((atr & MODEL_ATTR_HIDDEN) != 0)
                goto reject;
            if ((atr & MODEL_ATTR_NOCULL) == 0)
            {
                sz = RotTransPers(&mad->clip, (s32 *)rxy, 0, 0) >> 2;
                if ((atr & MODEL_ATTR_CULL_BEHIND) != 0 && sz == 0)
                {
                    result = -1;
                    goto tail;
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
                        goto tail;
                    }
                }
                if ((atr & MODEL_ATTR_CULL_FAR) != 0 && sz > DEPTH_LIMIT)
                {
                    result = -1;
                    goto tail;
                }
            }
            sz = RotTransPers(&UnitVector, 0, 0, 0) >> 2;
            if (sz > DEPTH_LIMIT)
            {
            reject:
                result = -1;
                goto tail;
            }
            if (sz >= FOG_DEPTH)
                DrawTMDmode = TMD_BANK_FOG;
            else
                DrawTMDmode = TMD_BANK_PLAIN;
            result = sz;
        tail:
            if (result + gap < 0)
            {
                return 0;
            }
        }
        for (i = 0; i < mad->n; i++)
        {
            objp = mad->object[i];
            if ((objp->attribute & MODEL_ATTR_HIDDEN) == 0)
            {
                GsGetLs(&objp->locate, &mat);
                GsSetLsMatrix(&mat);
                DrawTMD(&objp->object, OTablePt, gap);
            }
        }
    }
    return 1;
}
