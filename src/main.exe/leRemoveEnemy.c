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

int leRemoveEnemy(void)
{
    enemy_layout_index idx;

    idx = leFindEnemy();
    if (idx == ENEMY_LAYOUT_NONE)
    {
        return 0;
    }
    enemy[idx].type = CHARACTER_KIND_END;
    leLayoutEnemy(ENEMY_LAYOUT_EDIT);
}
