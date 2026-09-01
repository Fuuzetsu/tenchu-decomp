#include "common.h"
#include "main.exe.h"
#include "item.h"

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

/*
 * CreateCloneModel (0x8001851c, 0xa0 bytes) — ModelType's allocate+init
 * constructor, the sibling of CreateCloneOrnament.c's OrnamentType version
 * (same TU idiom: valloc(sizeof(T)), self-referencing object.coord2, a
 * World-rooted GsInitCoordinate2, a zeroed translation + RotMatrixYXZ, then
 * an optional object.tmd clone from an existing instance).
 *
 * Unlike OrnamentType, ModelType (item.h) owns its own `rotate`/`clip`
 * SVECTORs and an `id`/`attribute` pair, so RotMatrixYXZ is fed the object's
 * OWN (freshly zeroed) `rotate` field instead of the shared `UnitVector`
 * global that CreateCloneOrnament/UpdateOrnament use for OrnamentType (which
 * has no rotate of its own). `id` is initialized to -1 (no model index yet)
 * and `attribute` to 0, matching Ghidra's own independently-built struct
 * exactly (reference/ghidra_types.h:5021) — valloc(sizeof(ModelType)) == 0x74
 * only once item.h's ModelType carries the trailing `object` (GsDOBJ2) field
 * added here.
 */
extern void *valloc(u32 size);

ModelType *CreateCloneModel(ModelType *objp)
{
    ModelType *model;

    model = (ModelType *)valloc(sizeof(ModelType));
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
    model->id = -1;
    model->attribute = 0;
    if (objp != 0)
    {
        model->object.tmd = objp->object.tmd;
    }
    return model;
}
