#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawAfterimage(struct AfterimageType *afi, short disp);
 *     EFFECT.C:1717, 49 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct AfterimageType * afi
 *     param $a1       short disp
 *     reg   $s0       struct POLY_GT4 * poly
 *     reg   $s3       short i
 *     reg   $s1       short tplv
 *     stack sp+16     struct MATRIX mat
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

short DrawAfterimage(AfterimageType *afi, short disp)
{
    GpuPolyGT4Packet *poly;
    MATRIX mat;
    short i;
    s32 otz;
    s16 tplv;
    s32 pri;
    long tmp1, tmp2;

    tplv = 0x7f;
    poly = &afi->poly;

    if (disp != 0)
    {
        if (afi->n < afi->maxn - 1)
        {
            afi->n++;
        }
        for (i = afi->n - 1; i > 0; i--)
        {
            afi->p1[i] = afi->p1[i - 1];
            afi->p2[i] = afi->p2[i - 1];
        }
        GsGetLs(&afi->model->locate, &mat);
        GsSetLsMatrix(&mat);
        RotTransPers(&afi->vector1, (s32 *)afi->p1, 0, 0);
        RotTransPers(&afi->vector2, (s32 *)afi->p2, 0, 0);
        afi->sz = RotTransPers(&UnitVector, 0, 0, 0);
        if (afi->sz == 0)
        {
            return 0;
        }
    }
    else
    {
        if (afi->n <= 0)
        {
            return 0;
        }
        afi->n--;
    }

    poly->gpu.vertex[0].screen.word = afi->p1[0].word;
    poly->gpu.vertex[2].screen.word = afi->p2[0].word;

    i = 1;
    while (1)
    {
        if (i >= afi->n)
        {
            break;
        }
        poly->gpu.vertex[1].screen.word = poly->gpu.vertex[0].screen.word;
        tmp1 = afi->p1[i].word;
        poly->gpu.vertex[3].screen.word = poly->gpu.vertex[2].screen.word;
        poly->gpu.vertex[0].screen.word = tmp1;
        tmp2 = afi->p2[i].word;
        poly->packet.r1 = poly->packet.g1 = poly->packet.b1 = tplv;
        poly->packet.r3 = poly->packet.g3 = poly->packet.b3 = tplv;
        poly->gpu.vertex[2].screen.word = tmp2;

        tplv = ((afi->n - i) * 127) / afi->n;
        poly->packet.r0 = poly->packet.g0 = poly->packet.b0 = tplv;
        poly->packet.r2 = poly->packet.g2 = poly->packet.b2 = tplv;

        otz = afi->sz;
        otz = otz >> 2;
        CLAMP_SORT_DEPTH(pri, otz);
        GsSortPoly(&poly->packet, OTablePt, (u16)pri);
        i++;
    }

    return afi->n;
}
