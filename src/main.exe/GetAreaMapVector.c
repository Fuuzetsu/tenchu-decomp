#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapVector(unsigned long *area, struct MapVector *mvp, struct VECTOR *pos, long wide, int mode);
 *     CONFLICT.C:180, 39 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param stack+0   unsigned long * area
 *     param $s0       struct MapVector * mvp
 *     param $s1       struct VECTOR * pos
 *     param $s7       long wide
 *     param stack+16  int mode
 *     reg   $s4       short mode
 *     reg   $s2       short i
 *     reg   $s1       short v
 *     reg   $a0       long level
 *     reg   $s6       long x
 *     reg   $s3       long y
 *     reg   $s5       long z
 *
 * Globals it touches, as the original declared them:
 *     extern short FieldAttrib;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 * END PSX.SYM */

extern s16 direction[N_MAP_PROBE_DIRECTIONS][2];

/* The 4-direction movement probe. The centre query fills level/attrib/
 * area/index (mode forwards to GetAreaMapLevel with the cache bit 0x10
 * stripped; the four neighbour queries keep it). Each compass
 * neighbour at `wide` distance then classifies into: vector — no
 * floor there, or a >500 drop (unless mode bit 4 or a HIT/PUSH
 * attribute) — the 4-bit wall-direction code RefrectVector maps to
 * deflection angles; angleL — the neighbour floor is HIGHER than the
 * centre's (a wall or step up); angleH — lower. With no floor at the
 * centre the result is fabricated: MAP_BUOYANT, all four masks set. */
long GetAreaMapVector(AreaMapType *area, MapVector *mvp, VECTOR *pos, long wide, int mode)
{
    long x, y, z;
    long level2;
    short i;
    short v;
    long initial_level;
    short m;
    long rawmode;
    long mode2;

    x = pos->vx;
    y = pos->vy;
    z = pos->vz;

    mvp->level = GetAreaMapLevel(area, x, y, z, (short)(mode & ~AREA_LEVEL_REUSE_CACHED));
    mvp->attrib = FieldAttrib;
    mvp->area = FieldArea;
    mvp->index = FieldIndex;
    rawmode = (mode2 = mode);
    initial_level = mvp->level;
    if (mvp->attrib == 0)
    {
        mode = rawmode;
    }
    if ((initial_level ^ (u32)LEVEL_NONE) == 0)
    {
        mvp->height = 0;
        if (!(mode & AREA_LEVEL_ALLOW_DEEP))
        {
            /* Retail keeps three identical branches here; their original distinctions are unknown. */
            if (wide != 0)
            {
                if (y != 0)
                {
                    mvp->attrib = MAP_BUOYANT;
                    mvp->angleH = MAP_PROBE_ALL;
                    mvp->angleL = MAP_PROBE_ALL;
                    mvp->vector = MAP_PROBE_ALL;
                    return initial_level;
                }
                else
                {
                    mvp->attrib = MAP_BUOYANT;
                    mvp->angleH = MAP_PROBE_ALL;
                    mvp->angleL = MAP_PROBE_ALL;
                    mvp->vector = MAP_PROBE_ALL;
                    return initial_level;
                }
            }
            else
            {
                mvp->attrib = MAP_BUOYANT;
                mvp->angleH = MAP_PROBE_ALL;
                mvp->angleL = MAP_PROBE_ALL;
                mvp->vector = MAP_PROBE_ALL;
                return initial_level;
            }
        }
    }
    else
    {
        mvp->height = mvp->level - pos->vy;
    }

    i = 0;
    v = 1;
    mvp->angleH = 0;
    mvp->angleL = 0;
    mvp->vector = 0;
    m = (short)mode2;
    for (; i < N_MAP_PROBE_DIRECTIONS; i++)
    {
        level2 = GetAreaMapLevel(area, x + direction[i][0] * wide, y, z + direction[i][1] * wide, m);
        if (level2 == (u32)LEVEL_NONE ||
            ((level2 - y < -500) && !(mode2 & AREA_LEVEL_ALLOW_DEEP) &&
             !(((u16)mvp->attrib | (u16)FieldAttrib) &
               (MAP_SLOPE_X | MAP_SLOPE_Z))))
        {
            mvp->vector |= v;
        }
        else
        {
            if (mvp->level < level2)
            {
                mvp->angleL |= v;
            }
            else if (level2 < mvp->level)
            {
                mvp->angleH |= v;
            }
            mode = rawmode;
        }
        v <<= 1;
    }
    return mvp->level;
}
