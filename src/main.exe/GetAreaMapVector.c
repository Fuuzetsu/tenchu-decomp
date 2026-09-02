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

/*
 * GetAreaMapVector (0x80019ea0, 548 bytes) — probes the height at `pos` plus
 * the height of the 4 neighbouring cells listed in the static `direction`
 * table (dx/dz pairs, scaled by `wide`), and packs the result into *mvp:
 * level, a height delta, the found area's attrib/area-ptr/index-ptr, and 3
 * bitmasks over the 4 directions (`vector` = blocked, `angleL`/`angleH` =
 * level went up/down). Returns mvp->level. Retail's MapVector preserves the
 * PSX.SYM-proven first 0x10 bytes and appends area/index at 0x10/0x14.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `i`/`v` are `short` (PSX.SYM) — a short loop counter suppresses loop.c's
 *    strength reduction, so `&direction[i]` recomputes base+i*4 from scratch
 *    every iteration instead of walking a pointer (matches the asm: no
 *    hoisted `direction` base address, even though the loop otherwise gets
 *    normal invariant hoisting for the `(u16)mode` argument).
 *  - The `level == MIN` branch's `mvp->height = 0` runs UNCONDITIONALLY
 *    (a delay-slot fill in the real asm) even on the `mode & 4` path that
 *    skips the early return — write it before the inner `if`, not inside it.
 *  - The post-first-call height delta re-reads `pos->vy` directly (a fresh
 *    load), not the cached `y` local also used by the loop — matches the
 *    asm doing an actual memory reload there instead of reusing $s3.
 *  - `rawmode` is a second, untouched-by-the-MIN-branch copy of the incoming
 *    `mode`. Its only real consumer is the early `if (!(mode & 4))` test —
 *    `mode` is never read again afterwards, so both `mode = rawmode;` writes
 *    (one gated on `mvp->attrib == 0`, right after the setup; one inside the
 *    loop's non-`vector`-blocked path) are dead stores whose VALUE never
 *    matters. Their POSITION does: cc1 needs a live write to `mode`'s pseudo
 *    at each of those two points to reproduce retail's register pressure and
 *    scheduling there — move either write and the surrounding codegen shifts
 *    (confirmed by permuter search: relocating the first-arm write into the
 *    loop is what collapses the setup's persistent-register/scheduling
 *    residual to zero).
 *  - The duplicated wide/y early-return tails are allocation donors only.
 *    jump2 removes the redundant CFG, while global allocation sees enough
 *    extra references to select the retail homes for wide, y, and the cached
 *    mode. Duplicating the complete tail (rather than only `attrib = 2`) also
 *    recovers retail's `li 2`/attrib-store ordering.
 *  - `initial_level` preserves the first call result for the early return;
 *    spelling the MIN test as an XOR equality lets combine recover retail's
 *    direct comparison while preserving the register-valued return. The
 *    normal height calculation deliberately re-reads `mvp->level`.
 *  - FieldAttrib is the recovered signed-short global. The slope-mask test
 *    converts it to its `u16` bit pattern, preserving retail's `lhu` without
 *    changing the object's declared type.
 */

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
            /* TRIPLE identical arms, byte-required: collapsing to one body
             * swaps the s7/s8 allocation (measured) — the extra flow joins
             * are a CFG fence, same class as the twin arms (cookbook). */
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
