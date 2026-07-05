#include "common.h"
#include "main.exe.h"

/*
 * leRemoveEnemy (0x8003caec, 0x58 bytes) — `le`=layout-enemy family (see
 * leResetPath.c for TEnemyLayout, recovered from the Ghidra type export):
 * finds the currently-latched enemy slot (leFindEnemy) and, if one is
 * latched, marks its type dead (-1) and relays out the enemy set
 * (leLayoutEnemy(0)); otherwise does nothing.
 *
 * LayoutEnemyOption.c (matched) declares/calls this as plain
 * `extern void leRemoveEnemy(void); leRemoveEnemy();` — a void call
 * discarding whatever leLayoutEnemy leaves in $v0. That's this TU's own
 * (real) prototype though: leLayoutEnemy's call result is tail-returned, so
 * no extra move is needed after the jal, and the not-found path explicitly
 * zeroes $v0 (a real `return 0;`, not leftover) — matches the "original TUs
 * disagree with the defining TU's prototype" convention (docs/matching-
 * cookbook.md); the caller's void declaration just discards it.
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

typedef struct
{
    s16 type;
    s16 ThinkType;
    s16 nPath;
    s32 x;
    s32 y;
    s32 z;
    s16 r;
    s16 pad;
    VECTOR path[7];
} TEnemyLayout; /* 0x88 */

extern TEnemyLayout enemy[0x1E];
extern s32 leFindEnemy(void);
extern s32 leLayoutEnemy(s32 n);

s32 leRemoveEnemy(void)
{
    s32 idx;

    idx = leFindEnemy();
    if (idx != -1)
    {
        enemy[idx].type = -1;
        return leLayoutEnemy(0);
    }
    return 0;
}
