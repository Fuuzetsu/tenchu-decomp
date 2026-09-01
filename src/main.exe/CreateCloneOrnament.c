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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

/*
 * CreateCloneOrnament (0x8001873c, 0x80 bytes) — allocate and initialize an
 * OrnamentType (GsCOORDINATE2 locate@0 + GsDOBJ2 object@0x50; sizeof ==
 * 0x60, the exact valloc size). GsInitCoordinate2 hooks the new coordinate
 * under World, the DOBJ2's coord2 points back at the ornament's own locate,
 * the t[] stores place it at the origin, RotMatrixYXZ(&UnitVector, ...)
 * fills the 3x3, and — when cloning an existing ornament
 * (`objp` non-null) — copies its tmd pointer so the clone shares the same
 * 3D model data. PSX.SYM declares `World` as a ModelType; this function
 * addresses its leading `.locate` field.
 */
extern void *valloc(u32 size);

OrnamentType *CreateCloneOrnament(OrnamentType *objp)
{
    OrnamentType *ornament;

    ornament = (OrnamentType *)valloc(sizeof(OrnamentType));
    ornament->object.coord2 = &ornament->locate;
    ornament->object.attribute = 0;
    GsInitCoordinate2(&World.locate, &ornament->locate);
    ornament->locate.coord.t[0] = 0;
    ornament->locate.coord.t[1] = 0;
    ornament->locate.coord.t[2] = 0;
    RotMatrixYXZ(&UnitVector, &ornament->locate.coord);
    ornament->locate.flg = 0;
    if (objp != 0)
    {
        ornament->object.tmd = objp->object.tmd;
    }
    return ornament;
}
