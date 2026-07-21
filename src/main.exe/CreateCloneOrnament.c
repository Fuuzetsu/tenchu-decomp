#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct OrnamentType * CreateCloneOrnament(struct OrnamentType *objp);
 *     3DCTRL.C:518, 10 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $a0       struct OrnamentType * objp
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

/*
 * CreateCloneOrnament (0x8001873c, 0x80 bytes) — allocate and initialize an
 * OrnamentType (GsCOORDINATE2 locate@0 + GsDOBJ2 object@0x50; sizeof ==
 * 0x60, the exact valloc size). Hooks the new coordinate into World's
 * hierarchy (self-referencing coord2), builds an identity matrix at the origin via
 * RotMatrixYXZ(&UnitVector, ...), and — when cloning an existing ornament
 * (`objp` non-null) — copies its tmd pointer so the clone shares the same
 * 3D model data. PSX.SYM declares `World` as a ModelType; this function
 * addresses its leading `.locate` field.
 */
extern void *valloc(u32 size);
extern SVECTOR UnitVector;
extern ModelType World;

OrnamentType *CreateCloneOrnament(OrnamentType *objp)
{
    OrnamentType *base;

    base = (OrnamentType *)valloc(sizeof(OrnamentType));
    base->object.coord2 = (GsCOORDINATE2 *)base;
    base->object.attribute = 0;
    GsInitCoordinate2(&World.locate, (GsCOORDINATE2 *)base);
    base->locate.coord.t[0] = 0;
    base->locate.coord.t[1] = 0;
    base->locate.coord.t[2] = 0;
    RotMatrixYXZ(&UnitVector, &base->locate.coord);
    base->locate.flg = 0;
    if (objp != 0) {
        base->object.tmd = objp->object.tmd;
    }
    return base;
}
