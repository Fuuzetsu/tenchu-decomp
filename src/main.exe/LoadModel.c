#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelType * LoadModel(unsigned long *adr);
 *     3DCTRL.C:269, 24 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 * LoadOrnament.c (`adr = adr + 1; GsMapModelingData(adr);
 * GsLinkObject4((u_long)(adr + 2), &base->object, 0);`).
 */
extern void *valloc(u32 size);
extern void GsInitCoordinate2(GsCOORDINATE2 *super, GsCOORDINATE2 *base);
extern MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern void GsMapModelingData(u_long *adr);
extern void GsLinkObject4(u_long id, GsDOBJ2 *obj, s32 unk);

typedef struct
{
    GsCOORDINATE2 locate; /* 0x00 */
} WorldType;

extern WorldType World;

ModelType *LoadModel(u_long *adr)
{
    ModelType *base;

    base = (ModelType *)valloc(sizeof(ModelType));
    if (adr != 0) {
        adr = adr + 1;
        GsMapModelingData(adr);
        GsLinkObject4((u_long)(adr + 2), &base->object, 0);
    }
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
    return base;
}
