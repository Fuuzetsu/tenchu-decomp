#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MoveKorogari(struct tag_TItem *item, struct param_korogari *param);
 *     ITEM.C:663, 63 src lines, frame 64 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     param $s2       struct param_korogari * param
 *     stack sp+24     struct MapVector mv
 *     stack sp+40     struct SVECTOR vec
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern short RefrectMove[16][2];
 * END PSX.SYM */

/*
 * MoveKorogari (0x8003da08, 1,484 bytes) — advances a rolling item, probes
 * terrain, reflects or damps its velocity, and handles water and bounce
 * state changes.
 *
 * Matching notes:
 *  - GetAreaMapVector writes retail's complete 0x18-byte MapVector here.
 *  - svec_y_n20 is declared as an array even though only element zero is
 *    copied.  That preserves the target's two-register absolute address.
 *  - The final bounce deliberately spells the sum as A + (B + 25).  GCC
 *    2.8.1's fold pass reassociates that tree to (A + 25) + B, placing the
 *    target addiu on the abs()/2 accumulator before expanding rand() % 25.
 */

extern SVECTOR svec_y_n20[]; /* {0,-20,0} */

extern s32 CGetLevel(struct AreaNodeType **hint, s32 x, s32 y, s32 z, u32 flag);

void MoveKorogari(TItem *item, param_korogari *param)
{
    MapVector mv;
    SVECTOR vec;
    s32 level;

    if (param->status == KORO_STAY)
    {
        return;
    }

    item->locate->locate.coord.t[0] += param->vx;
    item->locate->locate.coord.t[1] += param->vy;
    item->locate->locate.coord.t[2] += param->vz;

    level = CGetLevel((struct AreaNodeType **)&param->hint,
                      item->locate->locate.coord.t[0],
                      item->locate->locate.coord.t[1],
                      item->locate->locate.coord.t[2], 0);
    if (level < item->locate->locate.coord.t[1])
    {
        item->locate->locate.coord.t[0] -= param->vx;
        item->locate->locate.coord.t[1] -= param->vy;
        item->locate->locate.coord.t[2] -= param->vz;

        GetAreaMapVector(GlobalAreaMap, &mv,
                         (VECTOR *)item->locate->locate.coord.t, 500,
                         AREA_LEVEL_DEFAULT);
        if (param->hint == 0)
        {
            level = CGetLevel((struct AreaNodeType **)&param->hint,
                              item->locate->locate.coord.t[0],
                              item->locate->locate.coord.t[1],
                              item->locate->locate.coord.t[2], 0);
            if (level == LEVEL_NONE)
            {
                if (param->status == KORO_OUT)
                {
                    param->vx = rand() % 1600 - 800;
                    param->vy = rand() % 1600 - 800;
                    param->vz = rand() % 1600 - 800;
                }
                else
                {
                    setVector(param, 0, 250, 0);
                }
                param->status = KORO_OUT;
                return;
            }
        }

        if (mv.vector != 0 && mv.height > 500)
        {
            param->vx = RefrectMove[mv.vector][0] * (abs(param->vx) / 4);
            param->vy /= 2;
            param->vz = RefrectMove[mv.vector][1] * (abs(param->vz) / 4);
            param->status = KORO_WALL;
            return;
        }

        if (mv.attrib & MAP_WATER)
        {
            param->vx = rand() % 20 - 10;
            param->vz = rand() % 20 - 10;
            if (param->vy > 20)
            {
                /* Dead copy, but retail's own bytes (removal mismatches);
                 * `vec` is in the demo symbols -- an older SetSplash likely
                 * took the splash direction it still loads here. */
                vec = svec_y_n20[0];
                SetSplash((VECTOR *)item->locate->locate.coord.t,
                          2 * FIXED_ONE, 2 * FIXED_ONE, 4);
                param->status = KORO_WATER;
            }
            param->vy = -param->vy / 8;
            return;
        }
        else
        {
            param->vx /= 2;
            param->vz /= 2;
            if (mv.level == LEVEL_NONE || mv.height > 1500)
            {
                goto bounce;
            }

            item->locate->locate.coord.t[1] = mv.level;
            if (param->vy < 46)
            {
                param->status = KORO_STAY;
                return;
            }

            param->vx += RefrectMove[mv.vector][0] * (rand() % 25 + 25);
            param->vz += RefrectMove[mv.vector][1] * (rand() % 25 + 25);
            param->status = KORO_GRAND;
            param->vy = -abs(param->vy) / 2;
            return;
        }
    bounce:
        param->vy = abs(param->vy) / 2 + (rand() % 25 + 25);
        param->status = KORO_WALL;
        return;
    }
    param->status = KORO_NORMAL;
    param->vy += 15;
}
