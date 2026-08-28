#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MoveKorogari(struct tag_TItem *item, struct param_korogari *param);
 *     ITEM.C:663, 63 src lines, frame 64 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
                         (VECTOR *)item->locate->locate.coord.t, 500, 0);
        if (param->hint == 0)
        {
            level = CGetLevel((struct AreaNodeType **)&param->hint,
                              item->locate->locate.coord.t[0],
                              item->locate->locate.coord.t[1],
                              item->locate->locate.coord.t[2], 0);
            if (level == (s32)0x80000000)
            {
                if (param->status == 5)
                {
                    param->vx = rand() % 1600 - 800;
                    param->vy = rand() % 1600 - 800;
                    param->vz = rand() % 1600 - 800;
                }
                else
                {
                    param->vx = 0;
                    param->vy = 250;
                    param->vz = 0;
                }
                param->status = 5;
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

        if (mv.attrib & 4)
        {
            param->vx = rand() % 20 - 10;
            param->vz = rand() % 20 - 10;
            if (param->vy > 20)
            {
                vec = svec_y_n20[0];
                SetSplash((VECTOR *)item->locate->locate.coord.t,
                          0x2000, 0x2000, 4);
                param->status = KORO_WATER;
            }
            param->vy = -param->vy / 8;
            return;
        }
        else
        {
            param->vx /= 2;
            param->vz /= 2;
            if (mv.level == (s32)0x80000000 || mv.height > 1500)
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

