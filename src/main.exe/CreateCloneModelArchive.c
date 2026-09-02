#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "model.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelArchiveType * CreateCloneModelArchive(struct ModelArchiveType *mad);
 *     3DCTRL.C:438, 27 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct ModelArchiveType * mad
 *     reg   $s1       struct ModelArchiveType * newmad
 *     reg   $s4       short i
 *     reg   $s1       struct ModelType * dim
 *     reg   $s2       struct ModelType * objp
 *     reg   $s0       struct ModelType * objp
 *     reg   $s0       struct ModelType * dim
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

extern void *valloc(u32 size);
extern char msg_no_source_model_archive[]; /* NO SOURCE MODEL ARCHIVE DATA */

ModelArchiveType *CreateCloneModelArchive(ModelArchiveType *mad)
{
    ModelArchiveType *newmad;
    short i;
    ModelType *objp;
    ModelType *dim;

    if (mad == 0)
    {
        SystemOut(msg_no_source_model_archive);
    }
    newmad = (ModelArchiveType *)valloc(sizeof(ModelArchiveType));
    newmad->n = mad->n;
    newmad->object = (ModelType **)valloc(newmad->n * sizeof(ModelType *));
    INITIALIZE_MODEL_STATE(newmad, mad->locate.super);
    i = 0;
    if (newmad->n > 0)
    {
        do
        {
            objp = mad->object[i];
            dim = (ModelType *)valloc(sizeof(ModelType));
            INITIALIZE_MODEL_INSTANCE(dim, &World.locate);
            if (objp != 0)
            {
                dim->object.tmd = objp->object.tmd;
            }
            newmad->object[i] = dim;
            i++;
        } while (i < newmad->n);
    }
    /* Read even when n == 0 (object[0] then never written): retail's own. */
    newmad->rotate.pad = (short)newmad->object[0]->locate.coord.t[1];
    return newmad;
}
