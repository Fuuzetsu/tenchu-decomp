#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leFindEnemy(void);
 *     WORLD.C:1099, 31 src lines, frame 88 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *     reg   $s6       long px
 *     reg   $s5       long py
 *     reg   $s4       long pz
 *     reg   $s3       int find
 *     reg   $s2       int r
 *     reg   $v1       int rr
 *     stack sp+16     struct SVECTOR pow
 *     stack sp+24     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

extern SVECTOR svec_y_n100[]; /* {0,-100,0} */

extern void *memset(void *s, s32 c, u32 n);

enemy_layout_index leFindEnemy(void)
{
    int i;
    s32 px, py, pz;
    enemy_layout_index find;
    int r;
    int rr;
    int dx, dy, dz;
    SVECTOR pow;
    VECTOR pos;
    VECTOR epos;

    find = ENEMY_LAYOUT_NONE;
    r = 2000;
    px = CamState.Owner->model->locate.coord.t[0];
    py = CamState.Owner->model->locate.coord.t[1];
    pz = CamState.Owner->model->locate.coord.t[2];

    i = 0;
    while (1)
    {
        if (i >= MAX_ENEMIES)
            break;
        if (enemy[i].type != CHARACTER_KIND_END)
        {
            dx = enemy[i].x - px;
            dy = enemy[i].y - py;
            dz = enemy[i].z - pz;
            rr = SquareRoot0(dx * dx + dy * dy + dz * dz);
            if (rr < r)
            {
                find = i;
                r = rr;
            }
        }
        i++;
    }

    if (find != ENEMY_LAYOUT_NONE)
    {
        pow = svec_y_n100[0];
        memset(&epos, 0, sizeof(epos));
        epos.vx = enemy[find].x;
        epos.vy = enemy[find].y;
        epos.vz = enemy[find].z;
        pos = epos;
        SetExplosion(&pos, &pow);
    }

    return find;
}
