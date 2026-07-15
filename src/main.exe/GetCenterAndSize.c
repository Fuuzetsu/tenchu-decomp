#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void GetCenterAndSize(unsigned long *tmd, struct SVECTOR *center, int *size);
 *     WORLD.C:390, 40 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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

/*
 * GetCenterAndSize (0x8003a9bc, 0x1a4 bytes) — same TU as leFindEnemy.c/
 * IsVisible.c (WORLD.C): given a TMD model's vertex table (`tmd[0]` the
 * vertex array pointer, `tmd[1]` the vertex count), finds the bounding box
 * over x/y/z, writes its center to `*center` and half of its largest axis
 * extent to `*size`.
 *
 * Matching notes:
 *  - Each axis is `if (max < v) max = v; else if (v < min) min = v;` — an
 *    `if`/`else if`, NOT two independent ifs (Ghidra's own rendering, an
 *    `||`-chained single `if`, is an SSA merge artifact of the shared
 *    "did we already take the max branch" flag; the raw asm's max-branch
 *    unconditionally jumps past the min-test).
 *  - The field read for the COMPARISON is a signed `lh`, but the read that
 *    feeds the (short-to-short) assignment is an unsigned `lhu` of the SAME
 *    field, at the SAME address, with no intervening store — two un-CSE'd
 *    loads of different machine modes (the narrowing-use-gets-lhu rule).
 *    Reproduced by reading `vert->vN` directly at both the comparison and
 *    the assignment rather than caching it in a temp.
 *  - `maxz - minz` is computed ONCE, immediately after the loop, BEFORE the
 *    center->vx/vy/vz stores — and re-used only after them, for the size
 *    comparison. Statement position, not Ghidra's late placement, is the
 *    real order (the value is textually first but used last).
 *  - The `(short)a < (short)b` axis-extent comparisons compile to a SLT on
 *    the two values shifted left 16 (no `sra`) — cc1 skips the sign-extend
 *    since only the ORDERING of the low 16 bits matters to a signed
 *    comparison of two `<<16`'d operands.
 *  - Indexing `vert[i].vN` (not walking a pointer with `vert++`) is required:
 *    the loop touches THREE fields (vx/vy/vz) off one base, and a walking
 *    pointer biases the induction register toward the last-touched field
 *    (Loops section rule) — confirmed here (permuter-free structural fix).
 *  - `dm = (short)(maxx - minx);` must be computed as a NAMED local right
 *    alongside `dz = maxz - minz;` (before the center->vx/vy/vz stores) and
 *    reused in the final `if (dz < dm)` — recomputing `(short)(maxx-minx)`
 *    inline at the comparison compiles to a different (permuter-found)
 *    register colouring for the whole tail. Matches PSX.SYM's `reg $a0
 *    short dm` local exactly.
 */

static void GetCenterAndSize(u32 *tmd, SVECTOR *center, int *size)
{
    SVECTOR *vert;
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

    nVert = (int)tmd[1];
    vert = (SVECTOR *)tmd[0];

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
    center->vx = (maxx + minx) / 2;
    center->vy = (maxy + miny) / 2;
    center->vz = (maxz + minz) / 2;
    if (dz < (short)(maxy - miny))
        dz = maxy - miny;
    if (dz < dm)
        dz = maxx - minx;
    *size = dz / 2;
}
