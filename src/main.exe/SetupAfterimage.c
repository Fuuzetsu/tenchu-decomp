#include "common.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>
#include "afterimage.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct AfterimageType * SetupAfterimage(struct ModelType *model, short len);
 *     EFFECT.C:1667, 35 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct ModelType * model
 *     param $a1       short len
 *     reg   $s2       struct GsIMAGE * image
 *     reg   $s1       struct AfterimageType * afi
 *     reg   $a1       short px
 *     reg   $a2       short py
 *     reg   $v1       short ph
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsIMAGE *AfterIMG;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

extern void *valloc(u32 size);

AfterimageType *SetupAfterimage(ModelType *model, short len)
{
    GsIMAGE *image;
    AfterimageType *afi;
    GpuScreenPosition *points;
    s32 size;

    image = AfterIMG;
    afi = (AfterimageType *)valloc(sizeof(AfterimageType));
    size = len * sizeof(*points);
    afi->model = model;
    afi->vector1 = UnitVector;
    afi->vector2 = UnitVector;
    afi->n = 0;
    afi->maxn = len;
    points = (GpuScreenPosition *)valloc(size);
    afi->p1 = points;
    points = (GpuScreenPosition *)valloc(size);
    afi->p2 = points;
    afi->sz = 0;
    SetupImageToPolyGT4(image, &afi->poly.packet, 0, 0);
    SetSemiTrans(&afi->poly.packet, 1);
    return afi;
}
