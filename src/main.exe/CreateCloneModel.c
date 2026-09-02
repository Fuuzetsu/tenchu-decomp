#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "model.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelType * CreateCloneModel(struct ModelType *objp);
 *     3DCTRL.C:319, 10 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * objp
 * END PSX.SYM */

extern void *valloc(u32 size);

ModelType *CreateCloneModel(ModelType *objp)
{
    ModelType *model;

    model = (ModelType *)valloc(sizeof(ModelType));
    INITIALIZE_MODEL_INSTANCE(model, &World.locate);
    if (objp != 0)
    {
        model->object.tmd = objp->object.tmd;
    }
    return model;
}
