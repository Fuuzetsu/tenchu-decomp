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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
extern void GsInitCoordinate2(GsCOORDINATE2 *super, GsCOORDINATE2 *base);
extern MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);

typedef struct
{
    GsCOORDINATE2 locate; /* 0x00 */
} WorldType;

extern WorldType World;

ModelType *CreateCloneModel(ModelType *objp)
{
    ModelType *base;

    base = (ModelType *)valloc(sizeof(ModelType));
    base->object.coord2 = (GsCOORDINATE2 *)base;
    base->object.attribute = 0;
    GsInitCoordinate2(&World.locate, (GsCOORDINATE2 *)base);
    base->locate.coord.t[0] = 0;
    base->locate.coord.t[1] = 0;
    base->locate.coord.t[2] = 0;
    base->rotate.vx = 0;
    base->rotate.vy = 0;
    base->rotate.vz = 0;
    base->clip.vx = 0;
    base->clip.vy = 0;
    base->clip.vz = 0;
    RotMatrixYXZ(&base->rotate, &base->locate.coord);
    base->locate.flg = 0;
    base->id = -1;
    base->attribute = 0;
    if (objp != 0) {
        base->object.tmd = objp->object.tmd;
    }
    return base;
}
