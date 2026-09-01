#include "common.h"
#include "main.exe.h"
#include "item.h"
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

/*
 * LoadModel (0x80018448, 0xb0 bytes) - near-twin of CreateCloneModel.c (both
 * ModelType constructors in this TU): allocate+zero-init a ModelType the
 * same way (self-referencing object.coord2, World-rooted
 * GsInitCoordinate2, zeroed translation/rotate/clip, RotMatrixYXZ fed the
 * object's OWN zeroed `rotate` field), but instead of CreateCloneModel's
 * clone-from-existing-instance tail (`objp->object.tmd`), LoadModel wires
 * the model data in from `adr` (when non-null) via GsMapModelingData/
 * GsLinkObject4 - the same "reassign the pointer parameter in place, then
 * use a smaller residual offset for the second call" idiom as
 * LoadOrnament.c (`TMD_FILE_DATA` followed by `TMD_DATA_OBJECTS`).
 */
extern void *valloc(u32 size);

ModelType *LoadModel(u_long *adr)
{
    ModelType *model;

    model = (ModelType *)valloc(sizeof(ModelType));
    if (adr != 0)
    {
        adr = TMD_FILE_DATA(adr);
        GsMapModelingData(adr);
        GsLinkObject4((u_long)TMD_DATA_OBJECTS(adr), &model->object, 0);
    }
    model->object.coord2 = &model->locate;
    model->object.attribute = 0;
    GsInitCoordinate2(&World.locate, &model->locate);
    model->locate.coord.t[0] = 0;
    model->locate.coord.t[1] = 0;
    model->locate.coord.t[2] = 0;
    model->rotate.vx = 0;
    model->rotate.vy = 0;
    model->rotate.vz = 0;
    model->clip.vx = 0;
    model->clip.vy = 0;
    model->clip.vz = 0;
    RotMatrixYXZ(&model->rotate, &model->locate.coord);
    model->locate.flg = 0;
    model->id = CONFLICT_NONE;
    model->attribute = 0;
    return model;
}
