#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelArchiveType * CreateCloneModelArchive(struct ModelArchiveType *mad);
 *     3DCTRL.C:438, 27 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * ModelArchiveType (0x6c bytes, item.h), copy the source's sub-model COUNT
 * (`n`) and allocate a matching `object` pointer table, hook the archive's
 * own GsCOORDINATE2 into the SOURCE's hierarchy (`mad->locate.super`, not
 * World - this level nests under whatever the clone source was attached
 * to), zero+RotMatrixYXZ its own translation like every other clone
 * constructor in this TU, then for each of the `n` sub-models: valloc a
 * fresh ModelType (item.h, 0x74 bytes), self-reference its `object.coord2`,
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
extern MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern void SystemOut(char *msg);
extern char D_8001107C[]; /* "NO SOURCE MODEL ARCHIVE DATA" */

typedef struct
{
    GsCOORDINATE2 locate; /* 0x00 */
} WorldType;

extern WorldType World;

ModelArchiveType *CreateCloneModelArchive(ModelArchiveType *mad)
{
    ModelArchiveType *newmad;
    short i;
    ModelType *objp;
    ModelType *dim;

    if (mad == 0) {
        SystemOut(D_8001107C);
    }
    newmad = (ModelArchiveType *)valloc(sizeof(ModelArchiveType));
    newmad->n = mad->n;
    newmad->object = (ModelType **)valloc(newmad->n * sizeof(ModelType *));
    GsInitCoordinate2(mad->locate.super, (GsCOORDINATE2 *)newmad);
    newmad->locate.coord.t[0] = 0;
    newmad->locate.coord.t[1] = 0;
    newmad->locate.coord.t[2] = 0;
    newmad->rotate.vx = 0;
    newmad->rotate.vy = 0;
    newmad->rotate.vz = 0;
    newmad->clip.vx = 0;
    newmad->clip.vy = 0;
    newmad->clip.vz = 0;
    RotMatrixYXZ(&newmad->rotate, &newmad->locate.coord);
    newmad->locate.flg = 0;
    newmad->id = -1;
    newmad->attribute = 0;
    i = 0;
    if (0 < newmad->n) {
        do {
            objp = mad->object[i];
            dim = (ModelType *)valloc(sizeof(ModelType));
            dim->object.coord2 = (GsCOORDINATE2 *)dim;
            dim->object.attribute = 0;
            GsInitCoordinate2(&World.locate, (GsCOORDINATE2 *)dim);
            dim->locate.coord.t[0] = 0;
            dim->locate.coord.t[1] = 0;
            dim->locate.coord.t[2] = 0;
            dim->rotate.vx = 0;
            dim->rotate.vy = 0;
            dim->rotate.vz = 0;
            dim->clip.vx = 0;
            dim->clip.vy = 0;
            dim->clip.vz = 0;
            RotMatrixYXZ(&dim->rotate, &dim->locate.coord);
            dim->locate.flg = 0;
            dim->id = -1;
            dim->attribute = 0;
            if (objp != 0) {
                dim->object.tmd = objp->object.tmd;
            }
            newmad->object[i] = dim;
            i = i + 1;
        } while (i < newmad->n);
    }
    newmad->rotate.pad = (short)newmad->object[0]->locate.coord.t[1];
    return newmad;
}
