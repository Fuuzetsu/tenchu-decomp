#include "common.h"
#include "main.exe.h"

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

typedef struct
{
    s16 type;
    s16 ThinkType;
    s16 nPath;
    s32 x;
    s32 y;
    s32 z;
    s16 r;
    s16 pad;
    VECTOR path[7];
} TEnemyLayout; /* 0x88 */

extern TEnemyLayout enemy[0x1E];
extern SVECTOR D_80097ABC[];
extern void SetExplosion(VECTOR *pos, SVECTOR *rot);
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
