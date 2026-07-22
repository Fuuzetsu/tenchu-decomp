#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leAddPath(int id, long x, long y, long z);
 *     WORLD.C:1303, 17 src lines, frame 56 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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

/*
 * leAddPath (0x8003c95c, 0xf0 bytes) — `le`=layout-enemy family (see
 * leResetPath.c for TEnemyLayout, recovered from the Ghidra type export):
 * appends one path waypoint (x,y,z) to enemy[id]'s path[] array (max 7
 * points) and, on success, spawns a marker explosion effect at that point
 * (debug menu "path layout > add path").
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `local_18 = D_80097ABC[0];` (whole SVECTOR struct assignment through an
 *    unknown-size array, not field-by-field or a plain scalar extern) —
 *    align-2 struct copies compile to lwl/lwr+swl/swr block moves (Stack
 *    objects section), and the 8-byte SVECTOR still wants the two-register
 *    hi/lo split for a whole-struct assignment (gp-vs-absolute-globals
 *    counterexample), which the unknown-size respelling forces.
 *  - `e = &enemy[id];` cached once, then `(&e->path[0])[e->nPath]` (not
 *    `e->path[e->nPath]`) for the three vx/vy/vz stores: the struct-member
 *    spelling emits `addu base,index`, but the target wants `addu
 *    index,base` — the `(&e->path[0])[i]` respelling picks that operand
 *    order (Expressions section's array-spelling addu-order rule).
 *  - `local_28`'s field stores must come BEFORE `local_18`'s struct copy in
 *    source (opposite of Ghidra's rendering, which puts the SVECTOR copy
 *    first): the asm interleaves the VECTOR field stores INSIDE the
 *    SVECTOR copy's lui/addiu address materialization.
 */

extern SVECTOR D_80097ABC[];
extern void *memset(void *s, s32 c, u32 n);

void leAddPath(s32 id, s32 x, s32 y, s32 z)
{
    TEnemyLayout *e;
    VECTOR local_28;
    SVECTOR local_18;

    if ((u32)id < 0x1E)
    {
        e = &enemy[id];
        if (e->nPath < 7)
        {
            (&e->path[0])[e->nPath].vx = x;
            (&e->path[0])[e->nPath].vy = y;
            (&e->path[0])[e->nPath].vz = z;
            e->nPath = e->nPath + 1;
            memset((void *)&local_28, 0, 0x10);
            local_28.vx = x;
            local_28.vy = y;
            local_28.vz = z;
            local_18 = D_80097ABC[0];
            SetExplosion(&local_28, &local_18);
        }
    }
}
