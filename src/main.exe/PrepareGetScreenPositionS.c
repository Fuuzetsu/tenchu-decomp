#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PrepareGetScreenPositionS(void);
 *     EFFECT.C:577, 9 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

extern MATRIX GsWSMATRIX;

void PrepareGetScreenPositionS(void)
{
    MATRIX *m = (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS;

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
}
