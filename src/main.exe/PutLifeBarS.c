#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int PutLifeBarS(void);
 *     INFOVIEW.C:328, 16 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s2       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct INFOVIEW__198fake LifeBar[4];
 * END PSX.SYM */

/*
 * PutLifeBarS (0x8004ad54, 0x90 bytes) — draws up to five life bars from the
 * LifeBar[nLifeBar] pool (five entries, stride 0x14), each entry counting
 * down its own display timer. Bottom-test-only do-while: i=0,i<nLifeBar
 * provably true at entry folds away the top test (cookbook Loops). Indexed
 * as LifeBar[i].f rather than
 * a walking pointer — a p++ walk biases the base to whichever field is
 * touched LAST in the body (here the count field), giving negative
 * displacements for the others; array indexing keeps the natural
 * offsets 4/8/C/10 (cookbook Loops: "Index the table T[i].f ... when the
 * loop touches 2+ fields"). Twin: DrawEffect.c (0.16), same TU as
 * PutItemIcon.c/PutItemCursor.c.
 */
extern void PutLifeBar(s32 x, s32 y, s32 life, s32 lifemax, s32 mode);

s32 PutLifeBarS(void)
{
    s32 i;

    i = 0;
    do
    {
        if (LifeBar[i].count > 0)
        {
            PutLifeBar(i * 60 - 140, -90, LifeBar[i].life,
                       LifeBar[i].max, LifeBar[i].style);
            LifeBar[i].count--;
        }
        i++;
    } while (i < nLifeBar);
    return 0;
}
