#include "common.h"
#include "main.exe.h"

/*
 * leClearLayout (0x8003cc08, 0x44 bytes) - `le`=layout-enemy family sibling
 * of leResetEnemyLayout.c: clears the whole enemy-layout table by marking
 * every slot's type dead (-1), counting down from the last slot to the
 * first (a real `for`, per the strength-reduced walking pointer in the
 * asm), then re-lays-out the enemy table via leLayoutEnemy(0).
 *
 * Matching notes (docs/matching-cookbook.md): identical loop-invariant
 * lever to leResetEnemyLayout - the loop-invariant `-1` store value and the
 * `for`-init counter are two separate statements, and their ORDER decides
 * which register loads first: giving the invariant its own named local
 * (`dead`) and assigning it BEFORE the `for` puts the invariant's `li`
 * ahead of the loop counter's `li` in the asm.
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
extern void leLayoutEnemy(s32 num);

void leClearLayout(void)
{
    s16 dead;
    s32 i;

    dead = -1;
    for (i = 0x1D; i >= 0; i--)
    {
        enemy[i].type = dead;
    }
    leLayoutEnemy(0);
}
