#include "common.h"
#include "main.exe.h"

/*
 * FUN_80039684 (0x80039684, 0x3c bytes) — zeroes the translation vector of
 * the GTE scratchpad MATRIX (fixed PS1 scratchpad RAM at 0x1F800000, Ghidra
 * names the region "Scratchpad"; only MATRIX.t[0..2] @ 0x14/0x18/0x1c are
 * touched, m[3][3] @ 0 is left alone), installs it as the current transform
 * matrix via SetTransMatrix, then sets the current rotation matrix to the
 * global world-space matrix GsWSMATRIX.
 */
extern void SetTransMatrix(MATRIX *m);
extern void SetRotMatrix(MATRIX *m);
extern MATRIX GsWSMATRIX;

void FUN_80039684(void)
{
    MATRIX *m = (MATRIX *)0x1F800000;

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
}
