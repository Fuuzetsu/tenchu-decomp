#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetLightning(struct VECTOR *start, struct VECTOR *end, short r, short g, int b);
 *     EFFECT.C:1562, 3 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * start
 *     param $a1       struct VECTOR * end
 *     param $a2       short r
 *     param $a3       short g
 *     param stack+16  int b
 * END PSX.SYM */

extern void SetLightningI(VECTOR *start, VECTOR *end, int gen,
                          short r, short g, short b);

void SetLightning(VECTOR *start, VECTOR *end, short r, short g, short b)
{
    SetLightningI(start, end, 1, r, g, b);
}
