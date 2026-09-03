#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leAddPath(int id, long x, long y, long z);
 *     WORLD.C:1303, 17 src lines, frame 56 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int id
 *     param $a1       long x
 *     param $a2       long y
 *     param $a3       long z
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR pow
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

extern void *memset(void *s, s32 c, u32 n);

void leAddPath(enemy_layout_index id, s32 x, s32 y, s32 z)
{
    TEnemyLayout *e;
    VECTOR pos;
    SVECTOR pow;

    if ((u32)id < MAX_ENEMIES)
    {
        e = &enemy[id];
        if (e->nPath < MAX_ENEMY_PATH_POINTS)
        {
            (&e->path[0])[e->nPath].vx = x;
            (&e->path[0])[e->nPath].vy = y;
            (&e->path[0])[e->nPath].vz = z;
            e->nPath++;
            memset(&pos, 0, sizeof(pos));
            pos.vx = x;
            pos.vy = y;
            pos.vz = z;
            pow = (SVECTOR){
                .vx = 0,
                .vy = -100,
                .vz = 0
            };
            SetExplosion(&pos, &pow);
        }
    }
}
