#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PrepareGetScreenPositionS(void);
 *     EFFECT.C:577, 9 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * PrepareGetScreenPositionS (0x80039684, 0x3c bytes) — zeroes the translation vector of
 * the GTE scratchpad MATRIX (fixed PS1 scratchpad RAM at 0x1F800000, Ghidra
 * names the region "Scratchpad"; only MATRIX.t[0..2] @ 0x14/0x18/0x1c are
 * touched, m[3][3] @ 0 is left alone), installs it as the current transform
 * matrix via SetTransMatrix, then sets the current rotation matrix to the
 * global world-space matrix GsWSMATRIX.
 */
extern void SetTransMatrix(MATRIX *m);
extern void SetRotMatrix(MATRIX *m);
extern MATRIX GsWSMATRIX;

void PrepareGetScreenPositionS(void)
{
    MATRIX *m = (MATRIX *)0x1F800000;

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
}
