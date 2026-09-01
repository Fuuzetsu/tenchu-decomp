#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapLevel(unsigned long *area, long x, long y, long z, int mode);
 *     CONFLICT.C:107, 69 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * area
 *     param $s5       long x
 *     param stack+8   long y
 *     param $s6       long z
 *     param stack+16  int mode
 *     stack sp+16     short mode
 *     reg   $s3       struct NodeIndexType * index
 *     reg   $s0       struct AreaNodeType * node
 *     reg   $fp       long y2
 *     reg   $s4       long yy
 *     reg   $v1       long sy
 *     reg   $s1       long n
 *     reg   $s7       long nn
 *
 * Globals it touches, as the original declared them:
 *     extern struct NodeIndexType *FieldIndex;
 *     extern short FieldAttrib;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/*
 * GetAreaMapLevel (0x80019a10) — node-map floor-height query (this is the
 * map/collision TU, not the item TU). Coordinates are divided by 10 into
 * map units; the NodeIndexType table (FieldIndex caches the last hit, `area`
 * is the table base) is walked down/up to the row matching `y2` (y/10),
 * then each
 * row entry's rect is tested and ComputeAreaLevel gives the height. The
 * result is cached in FieldArea/FieldIndex/FieldAttrib (attribute) and
 * AreaMapLastY (last `y2`). Returns the height*10 (or its delta to y with
 * mode&2), 0x80000000 when nothing is below.
 *
 * Matching notes (docs/matching-cookbook.md; all verified against the bytes):
 *  - Every loop is a hand-rolled goto loop, NOT while/do-while: real loops
 *    here get jump.c-rotated (do-while) or loop.c-strength-reduced (extra
 *    combined address bases like s8=index+2 / s0=node+6 appear). Goto loops
 *    keep the original's top-test + conditional back-jump and one cursor.
 *  - x and z are the PARAMETERS reused (`x = x / 10;`) — that is what puts
 *    them in callee-saved homes with the `move s5,a1`/`move s6,a3` prologue
 *    copies; `y2` is a separate local (y itself is reloaded from its arg home
 *    slot 0x50 at the end). PSX.SYM records both the promoted `int mode`
 *    parameter and the original `short mode` local, the characteristic K&R
 *    boundary shape represented here by `mode` and `mode16`.
 *  - The 5th-arg tests split: (mode & 1)/(mode & 0x10) read the still-live
 *    word register; (mode16 & 8)/(& 4)/(& 2) read the spilled short slot.
 *  - `row` is a `long *` cursor at &index->index; the row fields are reached
 *    as ((short *)row)[-1..5] (the -2($s2) access proves the cast-based
 *    shape) and the row rect tests re-read the same expressions in the
 *    division block so cse reuses the bounds registers.
 *  - qx/qz are `short`: the (q<<16)>>15 / (q<<16)>>13 sequences are the
 *    sign-extend of the short quotient merged with the *2 and *8 array scaling.
 *  - `node = (AreaNodeType *)((n << 4) + (long)list);` — integer + integer
 *    keeps the operand order (addu s0,v0,a2); `list + n` emits addu s0,a2,v0.
 *  - The tail return-0x80000000 body carries the ret_min label INSIDE the
 *    (`y2` < -1000 && !(mode & 4)) body, and the `yy`==MIN / attribute&2
 *    checks `goto ret_min`: written this way there is exactly ONE
 *    return-MIN body, entered by fallthrough, so it survives inline as
 *    [j epilogue; lui-delay] and the two gotos become branches whose delay
 *    slots reorg fills by stealing that lui (writing separate `return
 *    0x80000000;` statements lets cross-jump merge/invert them differently).
 *  - The (mode & 4) test's bnez lands at E60 past the E54 lhu because reorg's
 *    redundant_insn check sees t1 already holds the mode slot on that path
 *    and steals the following andi into the delay slot instead.
 *  - The do{}while(0) wrapper (found by tools/permute.py) is load-bearing:
 *    see the comment at its site. maspsx needs --expand-div for this file
 *    (true `/` by a variable -> ASPSX's guarded div with break 7/break 6).
 */

extern long AreaMapLastY; /* last queried y/10 (`y2`) */

extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);

/* mode bits: 1 = also accept floors up to 1500 units below y
 * (step-down tolerance); 2 = return the height difference (level - y)
 * instead of the level; 4 = keep floors deeper than 1000 below (else
 * "no floor"); 8 = first-hit sampling — take the first containing
 * node without the highest-below pick (DrawSnow); 0x10 = same-height
 * fast path reusing the cached FieldArea via AreaMapLastY. Returns
 * 0x80000000 for no floor; a base-material-2 node (the buoyant
 * surface) reports no floor, and a 0x2000 node is recorded but does
 * not stop the scan. */
long GetAreaMapLevel(AreaMapType *area, long x, long y, long z, int mode)
{
    long n;
    long *row;
    NodeIndexType *index;
    AreaNodeType *node;
    AreaNodeType *list;
    long yy;
    long nn;
    long y2;
    short mode16 = mode;
    long first_hit;
    long sy;
    long ret;
    short qx;
    short qz;

    index = FieldIndex;
    x = x / 10;
    FieldAttrib = 0x81;
    z = z / 10;
    y2 = y / 10;
    yy = LEVEL_NONE;
    /* The do{}while(0) wrapper is load-bearing: its loop notes double flow.c's
     * loop_depth ref-weighting for everything inside, which is what pushes the
     * allocation priorities into the original's order (row above index, nn
     * above y2 -> $s2/$s3/$s7/$fp exactly as in the target); the degenerate
     * loop itself generates no code. */
    do
    {
        if (mode & AREA_LEVEL_STEP_DOWN)
            y2 -= 150;

        if (y2 == AreaMapLastY && (mode & AREA_LEVEL_REUSE_CACHED) &&
            FieldArea->x1 <= x && x <= FieldArea->x2 &&
            FieldArea->z1 <= z && z <= FieldArea->z2)
        {
            yy = ComputeAreaLevel(FieldArea, x, z);
        }
        AreaMapLastY = y2;

        if (index != (NodeIndexType *)area)
        {
        down:
            if (y2 < index->y)
            {
                index--;
                if (index != (NodeIndexType *)area)
                    goto down;
            }
        }
        if (index->index != 0)
        {
        up:
            if (index->y < y2)
            {
                index++;
                if (index->index != 0)
                    goto up;
            }
            if (index->index != 0)
            {
                row = &index->index;
                first_hit = mode16 & AREA_LEVEL_FIRST_HIT;
            loop:
                if (yy == (u32)LEVEL_NONE)
                {
                    if (((short *)row)[2] <= x && x <= ((short *)row)[4] &&
                        ((short *)row)[3] <= z && z <= ((short *)row)[5])
                    {
                        nn = ((short *)row)[-1];
                        list = (AreaNodeType *)*row;
                        n = 0;
                        if (nn < 0)
                        {
                            qx = (x - ((short *)row)[2]) * 4 /
                                 (((short *)row)[4] - ((short *)row)[2]);
                            qz = (z - ((short *)row)[3]) * 4 /
                                 (((short *)row)[5] - ((short *)row)[3]);
                            n = ((IndexArrayType *)list)->array[qz][qx];
                            if (n == -1)
                                goto next;
                            list = (AreaNodeType *)((IndexArrayType *)list)->index;
                            nn = -nn;
                        }
                        if (n < nn)
                        {
                            node = (AreaNodeType *)((n << 4) + (long)list);
                        inner:
                            if (z < node->z1)
                                goto next;
                            if (node->x1 <= x && x <= node->x2 && z <= node->z2)
                            {
                                FieldIndex = index;
                                FieldArea = node;
                                if (first_hit)
                                {
                                    if (node->division == -1)
                                        yy = node->y;
                                    else
                                        yy = ComputeAreaLevel(node, x, z);
                                    FieldAttrib = FieldArea->attribute;
                                    goto next;
                                }
                                sy = ComputeAreaLevel(node, x, z);
                                if ((yy == (u32)LEVEL_NONE || sy < yy) && y2 <= sy)
                                {
                                    FieldAttrib = FieldArea->attribute;
                                    yy = sy;
                                    if (FieldAttrib & 0x2000)
                                        goto next;
                                }
                            }
                            n++;
                            node++;
                            if (n < nn)
                                goto inner;
                        }
                    }
                next:
                    row += 4;
                    index++;
                    if (*row != 0)
                        goto loop;
                }
            }
        }
        if (yy == (u32)LEVEL_NONE)
            goto ret_min;
        if (FieldAttrib & MAP_BUOYANT)
            goto ret_min;
        yy = yy * 10;
        y2 = yy - y;
        if (y2 < -1000 && (mode16 & AREA_LEVEL_ALLOW_DEEP) == 0)
        {
        ret_min:
            return LEVEL_NONE;
        }
        ret = yy;
    } while (0);
    if (mode16 & AREA_LEVEL_RETURN_DELTA)
        ret = y2;
    return ret;
}
