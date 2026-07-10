#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapVector(unsigned long *area, struct MapVector *mvp, struct VECTOR *pos, long wide, int mode);
 *     CONFLICT.C:180, 39 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 * STATUS: NON_MATCHING — 45 of 548 bytes differ, a pure whole-function
 * register-RENUMBERING tie (same instructions, same structure, different
 * specific $s-register assigned to each of pos/wide/x/y/z/truncated-mode —
 * see below); every field offset/type/branch this function establishes is
 * otherwise correct (proven independently of the residual).
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
 *  - A `do { } while (0);` right after the FieldArea/FieldIndex stores (real
 *    lever, keep it) recovered 10 of the original 55 residual bytes — it
 *    boosts flow.c's loop_depth ref-weighting for the `if (mvp->level ==
 *    MIN)` comparison enough to match the target's `move v1,v0` / separate
 *    MIN-constant materialization shape (GetAreaMapLevel's own wrapper is
 *    the same mechanism).
 *
 * THE RESIDUAL: with that fence in place, every remaining diff is a
 * same-instruction, different-hard-register rename across the WHOLE
 * function — target assigns pos=$s2, wide=$s7, x=$s6, y=$s3, z=$s5,
 * mode(raw)=$s1, mode-copy=$s4, truncated-mode=$fp; our compile assigns
 * pos=$s1, wide=$s8(=$fp), x=$s5, z=$s4, mode(raw)=$s6, truncated-mode=$s7
 * (y=$s3 matches already). All 9 callee-saved slots (s0-s7+fp) are in use
 * both sides — global-alloc's priority/tie-break just lands the SAME set of
 * values in a different permutation. Tried: reordering the `m = (short)
 * mode;` statement relative to the `mvp->angle*=0` stores (zero effect —
 * the swap is decided by something other than this statement's position);
 * `tools/permute.py GetAreaMapVector -- --stop-on-zero -j4` ran two bounded
 * passes (~900s combined, 6200+ iterations) and plateaued at internal score
 * 210-215 (the winning candidates were dead-code noise — a byte-identical
 * `if (y||x) {A} else {A}` split, repeated `& 0xFFFFFFFF` masks — not a
 * real fix per the cookbook's bisection warning). Parked per the "do NOT
 * open more surgical sessions on it" guidance.
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
    long level;
    long level2;
    short i;
    short v;
    short m;

    x = pos->vx;
    y = pos->vy;
    z = pos->vz;

    level = GetAreaMapLevel(area, x, y, z, (short)(mode & ~0x10));
    mvp->level = level;
    mvp->attrib = FieldAttrib;
    mvp->area = FieldArea;
    mvp->index = FieldIndex;

    do { } while (0);
    if (mvp->level == 0x80000000)
    {
        mvp->height = 0;
        if (!(mode & 4))
        {
            mvp->attrib = 2;
            mvp->angleH = 0xF;
            mvp->angleL = 0xF;
            mvp->vector = 0xF;
            return level;
        }
    }
    else
    {
        mvp->height = level - pos->vy;
    }

    i = 0;
    v = 1;
    mvp->angleH = 0;
    mvp->angleL = 0;
    mvp->vector = 0;
    m = (short)mode;

    for (; i < 4; i++)
    {
        level2 = GetAreaMapLevel(area, x + direction[i][0] * wide, y, z + direction[i][1] * wide, m);
        if (level2 == 0x80000000 ||
            ((level2 - y < -500) && !(mode & 4) && !((mvp->attrib | FieldAttrib) & 0xC000)))
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
