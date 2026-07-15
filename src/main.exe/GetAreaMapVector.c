#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapVector(unsigned long *area, struct MapVector *mvp, struct VECTOR *pos, long wide, int mode);
 *     CONFLICT.C:180, 39 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 * STATUS: NON_MATCHING — exact 548-byte extent, with 20 bytes still
 * different.  The persistent register assignment now matches retail
 * (mvp=$s0, pos=$s2, wide=$s7, x=$s6, y=$s3, z=$s5, raw mode=$s1,
 * cached mode=$s4, truncated mode=$fp), and the loop plus normal return path
 * are byte-exact.  Keep the INCLUDE_ASM guard: the remaining differences are
 * two localized scheduling/value-provenance equivalences in the setup after
 * the prologue and first GetAreaMapLevel call.
 *
 * GetAreaMapVector (0x80019ea0) — probes the height at `pos` plus the height
 * of the 4 neighbouring cells listed in the static `direction` table
 * (dx/dz pairs, scaled by `wide`), and packs the result into *mvp: level,
 * a height delta, the found area's attrib/area-ptr/index-ptr, and 3 bitmasks
 * over the 4 directions (`vector` = blocked, `angleL`/`angleH` = level went
 * up/down). Returns mvp->level. `mvp` is NOT a bare MapVector (game_types.h's
 * shared typedef is only the 8-byte level/height pair used elsewhere) — this
 * function writes a bigger 0x18-byte record (StickonCheck.c's header notes
 * the true record is 0x20 bytes; this function only touches the first 0x18),
 * matching the canonical MapVector layout (psxsym-types.h) through 0xF plus
 * two pointer fields (area/index) tacked on at 0x10/0x14 that PSX.SYM's
 * MapVector struct doesn't list — a per-file extended local view, same
 * convention as AreaMapVectorResult in StickonCheck.c.
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
 *  - A `do { } while (0);` right after the setup remains load-bearing: its
 *    loop notes give the MIN comparison the target allocation/scheduling
 *    shape without emitting a runtime loop.
 *  - Keeping both the raw full-width mode and a cached full-width mode needs
 *    two source identities. `rawmode = (mode2 = mode)` followed by identical
 *    `mode = rawmode` arms defeats copy propagation without leaving a branch;
 *    using `mvp->attrib` as the erased discriminator also schedules the cached
 *    mode copy after the three Field* loads. The loop then retains its in-loop
 *    `andi` on the cached value.
 *  - The duplicated wide/y early-return tails are allocation donors only.
 *    jump2 removes the redundant CFG, while global allocation sees enough
 *    extra references to select the retail homes for wide, y, and the cached
 *    mode. Duplicating the complete tail (rather than only `attrib = 2`) also
 *    recovers retail's `li 2`/attrib-store ordering.
 *  - `initial_level` preserves the first call result for the early return;
 *    spelling the MIN test as an XOR equality lets combine recover retail's
 *    direct comparison while preserving the register-valued return. The
 *    normal height calculation deliberately re-reads `mvp->level`.
 *
 * THE RESIDUAL is limited to: the setup stack-mode load occurs before y/z
 * instead of after them; and the first result is reloaded from `mvp->level`
 * instead of copied from $v0, which exchanges the adjacent FieldArea store and
 * MIN `lui`. One bounded 300-second permuter run (~26k candidates), guided
 * autorules sweeps of 80 and 160, and focused lifetime/CFG probes found no
 * zero. A later identical-arm-condition probe plus the XOR spelling above
 * reduced the guarded checkpoint from 33 to 20 differing image bytes.
 */

struct AreaNodeType;
struct NodeIndexType;

typedef struct
{
    long level;    /* 0x0 */
    long height;   /* 0x4 */
    u16 attrib;    /* 0x8 */
    s16 degree;    /* 0xA — untouched here */
    u8 vector;     /* 0xC */
    u8 direct;     /* 0xD — untouched here */
    u8 angleL;     /* 0xE */
    u8 angleH;     /* 0xF */
    struct AreaNodeType *area;    /* 0x10 */
    struct NodeIndexType *index;  /* 0x14 */
} MapVectorEx;

extern u16 FieldAttrib;
extern struct AreaNodeType *FieldArea;
extern struct NodeIndexType *FieldIndex;
extern s16 direction[][2];
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z, int mode);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetAreaMapVector", GetAreaMapVector);
#else
long GetAreaMapVector(unsigned long *area, MapVectorEx *mvp, VECTOR *pos, long wide, int mode)
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

    mvp->level = GetAreaMapLevel(area, x, y, z, (short)(mode & ~0x10));
    mvp->attrib = FieldAttrib;
    mvp->area = FieldArea;
    mvp->index = FieldIndex;
    rawmode = (mode2 = mode);
    initial_level = mvp->level;
    if (mvp->attrib != 0)
    {
        mode = rawmode;
    }
    else
    {
        mode = rawmode;
    }
    do { } while (0);
    if ((initial_level ^ 0x80000000) == 0)
    {
        mvp->height = 0;
        if (!(mode & 4))
        {
            if (wide != 0)
            {
                if (y != 0)
                {
                    mvp->attrib = 2;
                    mvp->angleH = 0xF;
                    mvp->angleL = 0xF;
                    mvp->vector = 0xF;
                    return initial_level;
                }
                else
                {
                    mvp->attrib = 2;
                    mvp->angleH = 0xF;
                    mvp->angleL = 0xF;
                    mvp->vector = 0xF;
                    return initial_level;
                }
            }
            else
            {
                mvp->attrib = 2;
                mvp->angleH = 0xF;
                mvp->angleL = 0xF;
                mvp->vector = 0xF;
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
    for (; i < 4; i++)
    {
        level2 = GetAreaMapLevel(area, x + direction[i][0] * wide, y, z + direction[i][1] * wide, m);
        if (level2 == 0x80000000 ||
            ((level2 - y < -500) && !(mode2 & 4) && !((mvp->attrib | FieldAttrib) & 0xC000)))
        {
            mvp->vector |= v;
        }
        else if (mvp->level < level2)
        {
            mvp->angleL |= v;
        }
        else if (level2 < mvp->level)
        {
            mvp->angleH |= v;
        }
        v <<= 1;
    }
    return mvp->level;
}
#endif
