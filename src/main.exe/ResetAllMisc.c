#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ResetAllMisc(void);
 *     MISC.C:147, 11 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

/*
 * ResetAllMisc (0x8004d514, 0x5c bytes) — walks the misc[] pool (200 entries,
 * tag_TMisc from AddMisc.c's sibling spawner, same TU/proven struct) and force
 * -disposes every live slot: runs `proc(p, 1)` (MM_DESTROY-style message),
 * then clears proc to NULL. Same pool/stride (0x24-byte tag_TMisc, 0xC8
 * entries) and the same "for whose entry test provably folds away" shape as
 * DrawEffect.c/FUN_80039c14.c (cookbook Loops/leResetEnemyLayout): a bottom
 * -test-only do-while with a strength-reduced walking pointer.
 *
 * The null-check and the indirect call both read `p->proc` — cc1's cse
 * reuses the ONE load for both (no separate variable needed): the asm loads
 * proc once into $v0, tests it, and calls through the SAME register
 * (DrawEffect.c's exact shape).
 */

void ResetAllMisc(void)
{
    tag_TMisc *p;
    s32 i;

    for (i = 0; i < 0xC8; i++)
    {
        p = &misc[i];
        if (p->proc != 0)
        {
            p->proc(p, 1);
            p->proc = 0;
        }
    }
}
