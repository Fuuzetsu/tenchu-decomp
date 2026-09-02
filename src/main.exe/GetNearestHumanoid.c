#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * GetNearestHumanoid(struct Humanoid *human, short distance);
 *     HUMAN.C:408, 24 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short distance
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

Humanoid *GetNearestHumanoid(Humanoid *human, short distance)
{
    Humanoid *cur;
    Humanoid *best;
    s32 dx, dz;
    s32 dist;
    s32 best_dist;
    int i;

    best = 0;
    best_dist = 20000;
    if (human->map.height != 0)
    {
        return best;
    }
    for (i = 0; i < Humans; i++)
    {
        cur = HumanGroup[i];
        if (cur != human && cur->status != STAT_DEAD &&
            (cur->attribute & ATTR_SUSPEND) == 0)
        {
            dx = PAGE_CIVILIAN; /* parked in dx before its delta-x role */
            if ((cur->type & PAGE_MASK) != dx && cur->life >= 0)
            {
                dx = __builtin_abs(cur->locate->vx - human->locate->vx);
                dz = __builtin_abs(cur->locate->vz - human->locate->vz);
                dist = SquareRoot0(dx * dx + dz * dz);
                if (dist < distance && dist < best_dist)
                {
                    best_dist = dist;
                    best = cur;
                }
            }
        }
    }
    return best;
}
