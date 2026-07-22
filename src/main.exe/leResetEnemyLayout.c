#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leResetEnemyLayout(void);
 *     WORLD.C:1054, 7 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

/*
 * leResetEnemyLayout (0x8003cc4c, 0x2c bytes) — `le`=layout-enemy family (see
 * leResetPath.c for TEnemyLayout, recovered from the Ghidra type export):
 * clears the whole enemy-layout table by marking every slot's type dead
 * (-1), counting down from the last slot to the first (a real `for`, per
 * the strength-reduced walking pointer in the asm).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The loop-invariant `-1` store value and the `for`-init counter are two
 *    separate statements, and their ORDER decides which register loads
 *    first: giving the invariant its own named local (`dead`) and assigning
 *    it BEFORE the `for` puts the invariant's `li` ahead of the loop
 *    counter's `li` in the asm. An inline `enemy[i].type = -1;` inside the
 *    loop body still hoists the constant out (loop.c invariant motion), but
 *    hoists it to right after the counter init instead — a pure 2-insn
 *    register swap (same instructions, reordered) with no other effect.
 */

extern TEnemyLayout enemy[0x1E];

void leResetEnemyLayout(void)
{
    s16 dead;
    s32 i;

    dead = -1;
    for (i = 0x1D; i >= 0; i--)
    {
        enemy[i].type = dead;
    }
}
