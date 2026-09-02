#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "model.h"
#include "tmdfile.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelType * LoadModel(unsigned long *adr);
 *     3DCTRL.C:269, 24 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

extern void *valloc(u32 size);

ModelType *LoadModel(u_long *adr)
{
    ModelType *model;

    model = (ModelType *)valloc(sizeof(ModelType));
    if (adr != 0)
    {
        adr = (u_long *)&((TMDFile *)adr)->data;
        GsMapModelingData(adr);
        GsLinkObject4((u_long)((TMDData *)adr)->objects, &model->object, 0);
    }
    INITIALIZE_MODEL_INSTANCE(model, &World.locate);
    return model;
}
