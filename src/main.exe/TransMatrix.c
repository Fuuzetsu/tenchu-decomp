#include "common.h"
#include "main.exe.h"

/*
 * Probable handwritten PsyQ assembly: its t0-t2 temporaries and unfilled
 * return delay slot are not produced by the available SDK compilers.
 * Kept parked pending classification in config/handwritten-asm.txt.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/TransMatrix", TransMatrix);
#else
MATRIX *TransMatrix(MATRIX *m, VECTOR *v)
{
    long x = v->vx;
    long y = v->vy;
    long z = v->vz;

    m->t[0] = x;
    m->t[1] = y;
    m->t[2] = z;
    return m;
}
#endif
