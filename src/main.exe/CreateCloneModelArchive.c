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

/*
 * CreateCloneModelArchive (0x80017874, 0x1a4 bytes) - ModelArchiveType's
 * allocate+init constructor, the archive-level sibling of CreateCloneModel.c
 * (single ModelType) and CreateCloneOrnament.c (OrnamentType): valloc a
 * ModelArchiveType (0x6c bytes, game_types.h), copy the source's sub-model COUNT
 * (`n`) and allocate a matching `object` pointer table, hook the archive's
 * own GsCOORDINATE2 into the SOURCE's hierarchy (`mad->locate.super`, not
 * World - this level nests under whatever the clone source was attached
 * to), zero+RotMatrixYXZ its own translation like every other clone
 * constructor in this TU, then for each of the `n` sub-models: valloc a
 * fresh ModelType (game_types.h, 0x74 bytes), self-reference its `object.coord2`,
 * root ITS GsCOORDINATE2 under World this time (sub-models always hang off
 * World, only the archive root inherits the source's own parent), zero+
 * RotMatrixYXZ its translation, and - when the corresponding source
 * sub-model (`mad->object[i]`) is non-null - copy its `tmd` pointer
 * (CreateCloneModel.c's exact same "clone an existing instance's model
 * data" tail). SystemOut is annotated noreturn by Ghidra but falls straight
 * through with no early return, same idiom as LoadTIM.c/InsertConflict.c.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `i` is `short` (PSX.SYM's own `reg $s4 short i`) - a `short` loop
 *    counter suppresses loop.c's strength reduction, keeping the archive's
 *    `object[i]` / clone's `object[i]` indexing as a recompute-from-base
 *    `(i<<16)>>14` (scale-by-4, sizeof(ModelType*)) each iteration instead
 *    of a walking pointer, matching the target exactly (Loops: "a short
 *    loop counter suppresses strength reduction").
 *  - `n` is read via `lhu` (item.h's field is signed `s16`) purely because
 *    the SAME loaded value feeds both a plain truncating store
 *    (`newmad->n = n;`, sign bits dead) and, separately, a signed widen-
 *    and-scale-by-4 for the `object` table's valloc size - one shared load
 *    serves both a narrow store and a wider arithmetic use (Expressions:
 *    the shared-value-across-widths family).
 *  - PSX.SYM names the source sub-model pointer `objp` and the freshly
 *    cloned instance `dim` (both recur under those exact names in
 *    LoadModelArchive.c's own PSX.SYM - an established per-TU convention
 *    for "existing model referenced for cloning" / "new model instance").
 *  - The trailing `newmad->rotate.pad = (short)newmad->object[0]->locate.
 *    coord.t[1];` narrows a `long` (SVECTOR.pad is the struct's own trailing
 *    padding slot, reused as scratch storage) - read via `lhu` since only
 *    the low 16 bits of the store survive (Expressions: narrowing use of a
 *    signed value still loads `lhu`).
 */
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
