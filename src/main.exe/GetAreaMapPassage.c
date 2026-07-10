#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct VECTOR * GetAreaMapPassage(unsigned long *area, struct VECTOR *pos, struct SVECTOR *vect, short n);
 *     CONFLICT.C:223, 31 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s4       unsigned long * area
 *     param $a1       struct VECTOR * pos
 *     param $s1       struct SVECTOR * vect
 *     param $s3       short n
 *     reg   $a0       struct AreaNodeType * node
 *     stack sp+24     long [2] x
 *     stack sp+32     long [2] z
 *     stack sp+40     long [2] y
 *
 * Globals it touches, as the original declared them:
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — links 568 bytes vs the 580-byte target (3
 * instructions / 12 bytes SHORT; 80 differing lines). Default build keeps
 * the byte-identical INCLUDE_ASM stub.
 *
 * Matching notes (verified against the raw .s):
 *  - `cv` is the file-scope VECTOR at 0x800bc0f8 (splat-carved data symbol,
 *    already referenced by name in the disassembly's `%hi(cv)`/`%lo(cv)`) —
 *    declared `extern VECTOR cv;` here, not defined (config/symbols.main.exe.txt
 *    already places it).
 *  - `AreaNodeType`/`NodeIndexType` are the SAME layouts GetAreaMapLevel.c
 *    already proved (x1@4,z1@6,x2@8,z2@0xA in AreaNodeType; y@0 in
 *    NodeIndexType, size 0x10 — `FieldIndex[-1].y` is the raw `lh
 *    -0x10($v1)` off FieldIndex itself). Each TU here privately redeclares
 *    them (no shared header), matching the existing convention
 *    (ComputeAreaLevel.c/GetAreaMapVector.c both do the same).
 *  - The OUTER "step one level, requery" loop is a hand-rolled
 *    `label: ...; goto label;`, NOT a real for/while: its give-up path
 *    (`return &cv;`) takes the address of a global INSIDE the loop body —
 *    the exact SetBleed/SetSmoke hazard (loop.c would otherwise hoist the
 *    `&cv` materialization into a preheader). The INNER step-and-test loop
 *    has no such hazard (its give-up path returns a plain `0`) and is a
 *    genuine `do { step } while (6-way bounds test);` — the bottom test is
 *    the natural compiled shape already.
 *  - `x[2]`/`z[2]`/`y[2]` (2-long stack arrays, PSX.SYM's own names) hold
 *    the three axis bounds pairs: x[0]/x[1] = node->x1/x2 * 10 (always
 *    computed), z[0]/z[1] = node->z1/z2 * 10 (z[1] unconditionally, even
 *    though it sits right next to the FieldIndex-vs-area branch), y[0] =
 *    the just-obtained GetAreaMapLevel result, y[1] = -1000000 or
 *    `FieldIndex[-1].y * 10` depending on whether FieldIndex still points
 *    at this same `area` table.
 *  - `GetAreaMapLevel`'s 5th argument (`flag`) is stack-passed 0 (`sw
 *    zero,0x10(sp)` right before the call) — matches its own prototype's
 *    `u16 flag`.
 *
 * RESIDUAL (12 bytes / 3 instructions short): after the `y[1]` if/else,
 * target reuses the SAME two base registers it has held `&cv`/`%hi(cv)` in
 * since function entry (never reloaded, even across the GetAreaMapLevel
 * call — `&cv` is a compile-time constant, so this is a plain CSE of an
 * address materialization, not a stored-pointer persistence question).
 * Our draft instead recomputes `&cv` fresh (an extra `lui`/`addiu` pair)
 * right after the `y[1]` if/else, which also perturbs which of x[0]/x[1]/
 * y[0]/y[1] get pre-loaded into registers before the inner loop (target
 * pre-loads exactly 3 — x[0],x[1],y[0] — leaving z[0]/z[1]/y[1] to reload
 * lazily inside the loop's own bottom test; ours pre-loads a 4th). Tried:
 * an explicit `VECTOR *pcv = &cv;` local threaded through every access
 * (worse, 552 bytes — an even bigger gap); dropping the `node` local for
 * direct `FieldArea->` access (no change). `tools/autorules.py
 * GetAreaMapPassage` found no improving edit. This is the cookbook's
 * allocator-priority/register-pressure tie class (see SetBleeds/SetSmoke's
 * `$fp`-vs-`base` writeups) — parked per the attempt-cap guidance; the
 * struct layouts, dispatch polarities (both the y[0]==MIN give-up arm and
 * the FieldIndex==area arm needed inverting from Ghidra's literal
 * rendering — "nest the long arm"/"opposite polarity" cookbook rules) and
 * store order above are all independently confirmed correct.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetAreaMapPassage", GetAreaMapPassage);
#else

typedef struct AreaNodeType
{
    s16 y;         /* 0x0 */
    s16 dy;        /* 0x2 */
    s16 x1;        /* 0x4 */
    s16 z1;        /* 0x6 */
    s16 x2;        /* 0x8 */
    s16 z2;        /* 0xA */
    u16 attribute; /* 0xC */
    s16 division;  /* 0xE */
} AreaNodeType;

typedef struct NodeIndexType
{
    s16 y;      /* 0x0 */
    s16 n;      /* 0x2 */
    long index; /* 0x4 */
    s16 x1;     /* 0x8 */
    s16 z1;     /* 0xA */
    s16 x2;     /* 0xC */
    s16 z2;     /* 0xE */
} NodeIndexType;

extern AreaNodeType *FieldArea;
extern NodeIndexType *FieldIndex;
extern VECTOR cv;

extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z, u16 flag);

VECTOR *GetAreaMapPassage(unsigned long *area, VECTOR *pos, SVECTOR *vect, short n)
{
    long x[2], z[2], y[2];
    AreaNodeType *node;

    cv = *pos;
    if (n <= 0)
    {
        n = 100;
    }
outer:
    y[0] = GetAreaMapLevel(area, cv.vx, cv.vy, cv.vz, 0);
    if (y[0] != 0x80000000)
    {
        node = FieldArea;
        x[0] = node->x1 * 10;
        x[1] = node->x2 * 10;
        z[0] = node->z1 * 10;
        z[1] = node->z2 * 10;
        if (FieldIndex != (NodeIndexType *)area)
        {
            y[1] = FieldIndex[-1].y * 10;
        }
        else
        {
            y[1] = -1000000;
        }
        do
        {
            cv.vx = cv.vx + vect->vx;
            cv.vy = cv.vy + vect->vy;
            n = n - 1;
            cv.vz = cv.vz + vect->vz;
            if (n == 0)
            {
                return 0;
            }
        } while (x[0] <= cv.vx && cv.vx <= x[1] && y[0] <= cv.vy
                 && cv.vy <= y[1] && z[0] <= cv.vz && cv.vz <= z[1]);
        goto outer;
    }
    cv.vz = cv.vz - vect->vz;
    cv.vy = cv.vy - vect->vy;
    cv.vx = cv.vx - vect->vx;
    return &cv;
}
#endif
