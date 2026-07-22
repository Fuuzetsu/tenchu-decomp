#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static int PutLifeBarS(void);
 *     INFOVIEW.C:328, 16 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
    s32 x;

    i = 0;
    x = -0x8C;
    do
    {
        if (LifeBar[i].count > 0)
        {
            PutLifeBar(x, -0x5A, LifeBar[i].life, LifeBar[i].max, LifeBar[i].style);
            LifeBar[i].count = LifeBar[i].count - 1;
        }
        x = x + 0x3C;
        i = i + 1;
    } while (i < nLifeBar);
    return 0;
}
