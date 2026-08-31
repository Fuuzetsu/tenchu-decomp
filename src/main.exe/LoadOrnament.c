#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct OrnamentType * LoadOrnament(unsigned long *adr);
 *     3DCTRL.C:471, 21 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

/*
 * LoadOrnament (0x80018644, 0x90 bytes) - near-twin of
 * CreateCloneOrnament.c (same OrnamentType/World setup, same
 * GsInitCoordinate2/RotMatrixYXZ initialization tail): allocate and
 * initialize an OrnamentType, hook it into World's hierarchy, build an
 * identity matrix at the origin - and, when `adr` is non-null, wire the
 * model data in (GsMapModelingData on the modeling-data block, then
 * GsLinkObject4 to attach the object list) instead of
 * CreateCloneOrnament's plain tmd-pointer copy from an existing clone
 * source.
 */
extern void *valloc(u32 size);

OrnamentType *LoadOrnament(u_long *adr)
{
    OrnamentType *base;

    base = (OrnamentType *)valloc(sizeof(OrnamentType));
    if (adr != 0)
    {
        adr++;
        GsMapModelingData(adr);
        GsLinkObject4((u_long)(adr + 2), &base->object, 0);
    }
    base->object.coord2 = &base->locate;
    base->object.attribute = 0;
    GsInitCoordinate2(&World.locate, &base->locate);
    base->locate.coord.t[0] = 0;
    base->locate.coord.t[1] = 0;
    base->locate.coord.t[2] = 0;
    RotMatrixYXZ(&UnitVector, &base->locate.coord);
    base->locate.flg = 0;
    return base;
}
