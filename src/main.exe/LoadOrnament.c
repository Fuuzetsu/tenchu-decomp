#include "common.h"
#include "main.exe.h"

/*
 * LoadOrnament (0x80018644, 0x90 bytes) - near-twin of
 * CreateCloneOrnament.c (same OrnamentType/WorldType locals, same
 * GsInitCoordinate2/RotMatrixYXZ initialization tail): allocate and
 * initialize an OrnamentType, hook it into World's hierarchy, build an
 * identity matrix at the origin - and, when `adr` is non-null, wire the
 * model data in (GsMapModelingData on the modeling-data block, then
 * GsLinkObject4 to attach the object list) instead of
 * CreateCloneOrnament's plain tmd-pointer copy from an existing clone
 * source.
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
extern void GsMapModelingData(u_long *adr);
extern void GsLinkObject4(u_long id, GsDOBJ2 *obj, s32 unk);
extern SVECTOR UnitVector;
extern WorldType World;

OrnamentType *LoadOrnament(u_long *adr)
{
    OrnamentType *base;

    base = (OrnamentType *)valloc(sizeof(OrnamentType));
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
    RotMatrixYXZ(&UnitVector, &base->locate.coord);
    base->locate.flg = 0;
    return base;
}
