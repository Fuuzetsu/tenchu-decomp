#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void leAddPath(int id, long x, long y, long z);
 *     WORLD.C:1303, 17 src lines, frame 56 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *  - `pow = svec_y_n100[0];` (whole SVECTOR struct assignment through an
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
 *  - `pos`'s field stores must come BEFORE `pow`'s struct copy in
 *    source (opposite of Ghidra's rendering, which puts the SVECTOR copy
 *    first): the asm interleaves the VECTOR field stores INSIDE the
 *    SVECTOR copy's lui/addiu address materialization.
 */

extern SVECTOR svec_y_n100[]; /* {0,-100,0} */
extern void *memset(void *s, s32 c, u32 n);

void leAddPath(s32 id, s32 x, s32 y, s32 z)
{
    TEnemyLayout *e;
    VECTOR pos;
    SVECTOR pow;

    if ((u32)id < MAX_ENEMIES)
    {
        e = &enemy[id];
        if (e->nPath < MAX_ENEMY_PATH_POINTS)
        {
            (&e->path[0])[e->nPath].vx = x;
            (&e->path[0])[e->nPath].vy = y;
            (&e->path[0])[e->nPath].vz = z;
            e->nPath++;
            memset(&pos, 0, sizeof(pos));
            pos.vx = x;
            pos.vy = y;
            pos.vz = z;
            pow = svec_y_n100[0];
            SetExplosion(&pos, &pow);
        }
    }
}
