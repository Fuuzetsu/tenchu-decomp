#include "common.h"
#include "main.exe.h"
#include "tmdfile.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void GetCenterAndSize(unsigned long *tmd, struct SVECTOR *center, int *size);
 *     WORLD.C:390, 40 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * tmd
 *     param $a1       struct SVECTOR * center
 *     param $a2       int * size
 *     reg   $a0       struct SVECTOR * vert
 *     reg   $t7       int nVert
 *     reg   $t0       int i
 *     reg   $t6       short minx
 *     reg   $t2       short miny
 *     reg   $t5       short minz
 *     reg   $t4       short maxx
 *     reg   $t1       short maxy
 *     reg   $t3       short maxz
 *     reg   $a0       short dm
 * END PSX.SYM */

static void GetCenterAndSize(TmdObjectRecord *tmd, SVECTOR *center, int *size)
{
    VERT *vert;
    int nVert;
    int i;
    short minx, miny, minz, maxx, maxy, maxz;
    short dz;
    short dm;

    minx = 0;
    miny = minx;
    minz = minx;
    maxx = minx;
    maxy = minx;
    maxz = minx;

    nVert = tmd->linked.vertex_count;
    vert = tmd->linked.vertices;

    for (i = 0; i < nVert; i++)
    {
        if (maxx < vert[i].vx)
            maxx = vert[i].vx;
        else if (vert[i].vx < minx)
            minx = vert[i].vx;

        if (maxy < vert[i].vy)
            maxy = vert[i].vy;
        else if (vert[i].vy < miny)
            miny = vert[i].vy;

        if (maxz < vert[i].vz)
            maxz = vert[i].vz;
        else if (vert[i].vz < minz)
            minz = vert[i].vz;
    }

    dm = (short)(maxx - minx);
    dz = maxz - minz;
    setVector(center, (maxx + minx) / 2, (maxy + miny) / 2, (maxz + minz) / 2);
    if (dz < (short)(maxy - miny))
        dz = maxy - miny;
    if (dz < dm)
        dz = maxx - minx;
    *size = dz / 2;
}
