#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leSetEnemy(int type, short think, long x, long y, long z, int r);
 *     WORLD.C:1134, 22 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int type
 *     param $a1       short think
 *     param $a2       long x
 *     param $a3       long y
 *     param stack+16  long z
 *     param stack+20  int r
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

enemy_layout_index leSetEnemy(s32 type, TThinkType think, s32 x, s32 y,
                              s32 z, s16 r)
{
    enemy_layout_index idx;
    enemy_layout_index result;
    s32 offset;
    TEnemyLayout *e;

    idx = 0;
    do
    {
        if (enemy[idx].type == CHARACTER_KIND_END)
        {
            result = idx;
            goto found;
        }
        idx++;
    } while (idx < MAX_ENEMIES);
    result = ENEMY_LAYOUT_NONE;
found:
    if (result == ENEMY_LAYOUT_NONE)
        return ENEMY_LAYOUT_NONE;
    offset = result * sizeof(*e);
    e = (TEnemyLayout *)(offset + (s32)enemy);
    e->type = (s16)type;
    e->ThinkType = think;
    e->nPath = 0;
    e->x = x;
    e->y = y;
    e->z = z;
    e->r = r;
    return result;
}
