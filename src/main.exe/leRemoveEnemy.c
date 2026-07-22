#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leRemoveEnemy(void);
 *     WORLD.C:1259, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

/*
 * leRemoveEnemy (0x8003caec, 0x58 bytes) — `le`=layout-enemy family (see
 * leResetPath.c for TEnemyLayout, recovered from the Ghidra type export):
 * finds the currently-latched enemy slot (leFindEnemy) and, if one is
 * latched, marks its type dead (-1) and relays out the enemy set
 * (leLayoutEnemy(0)); otherwise does nothing.
 *
 * The not-found path explicitly returns zero. The found path calls the
 * original void leLayoutEnemy API and then falls off the end, leaving that
 * call's residual $v0 untouched as the retail binary does. Both belonged to
 * the same original WORLD.C translation unit, so a conflicting declaration
 * would not have been possible there.
 *
 * m2c over-counts leLayoutEnemy's call as 2-arg (0, -1): $a1 still holds the
 * -1 used for the preceding `type = -1` store and was never reassigned
 * before the jal, read by m2c's basic-block-local view as a second argument
 * — every OTHER matched call site (FileOption.c, LayoutEnemyOption.c,
 * PlayerOption.c) proves leLayoutEnemy takes exactly one arg.
 *
 * enemy[idx] reproduces leResetPath.c's exact addu/shift sequence (same
 * array, same 0x88 stride) byte for byte — proof the plain array-index
 * spelling is right here too.
 */

int leRemoveEnemy(void)
{
    int idx;

    idx = leFindEnemy();
    if (idx == -1)
    {
        return 0;
    }
    enemy[idx].type = -1;
    leLayoutEnemy(0);
}
