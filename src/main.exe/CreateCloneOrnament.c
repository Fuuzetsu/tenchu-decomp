#include "common.h"
#include "main.exe.h"

/*
 * CreateCloneOrnament (0x8001873c, 0x80 bytes) — allocate and initialize an
 * OrnamentType (GsCOORDINATE2 locate@0 + GsDOBJ2 object@0x50, same local
 * convention as DrawOrnament.c/UpdateOrnament.c; sizeof == 0x60, the exact
 * valloc size). Hooks the new coordinate into World's hierarchy (self-
 * referencing coord2), builds an identity matrix at the origin via
 * RotMatrixYXZ(&UnitVector, ...), and — when cloning an existing ornament
 * (`objp` non-null) — copies its tmd pointer so the clone shares the same
 * 3D model data. `World` (0x80097fa0, config/symbols.main.exe.txt) is only
 * ever addressed here through its `.locate` field (Ghidra's own rendering
 * across every GsInitCoordinate2 call site); declared minimally, like
 * OrnamentType.
 */
typedef struct
{
    GsCOORDINATE2 locate; /* 0x00 */
    GsDOBJ2 object;       /* 0x50 */
} OrnamentType;

typedef struct
{
    GsCOORDINATE2 locate; /* 0x00 */
} WorldType;

extern void *valloc(u32 size);
extern void GsInitCoordinate2(GsCOORDINATE2 *super, GsCOORDINATE2 *base);
extern MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);
extern SVECTOR UnitVector;
extern WorldType World;

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
