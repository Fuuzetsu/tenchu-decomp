#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawEffect(void);
 *     EFFECT.C:366, 73 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * DrawEffect (0x80039bbc, 0x58 bytes) — walks the EffectSlot[] pool (200
 * entries) and runs every live slot's `proc` callback, passing the slot itself.
 * Same pool/stride as FUN_80039c14.c (0x4C-byte tag_EffectSlot, 0xC8
 * entries) — a real `for` loop whose i<0xC8 entry test provably folds away,
 * leaving the bottom-test-only do-while shape and a strength-reduced
 * walking pointer (cookbook Loops/leResetEnemyLayout).
 *
 * The null-check and the indirect call both read `p->proc` — cc1's cse
 * reuses the ONE load for both (no separate variable needed): the asm loads
 * proc once into $v0, tests it, and calls through the SAME register.
 */

void DrawEffect(void)
{
    TEffectSlot *p;
    s32 i;

    for (i = 0; i < 0xC8; i++)
    {
        p = &EffectSlot[i];
        if (p->proc != 0)
        {
            p->proc(p);
        }
    }
}
