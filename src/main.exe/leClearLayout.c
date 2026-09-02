#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leClearLayout(void);
 *     WORLD.C:1065, 9 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

void leClearLayout(void)
{
    character_kind dead;
    s32 i;

    dead = CHARACTER_KIND_END;
    for (i = MAX_ENEMIES - 1; i >= 0; i--)
    {
        enemy[i].type = dead;
    }
    leLayoutEnemy(ENEMY_LAYOUT_EDIT);
}
