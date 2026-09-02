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

extern long AreaMapLastY; /* last queried y/10 (`y2`) */

extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);

/* mode bits: 1 = also accept floors up to 1500 units below y
 * (step-down tolerance); 2 = return the height difference (level - y)
 * instead of the level; 4 = keep floors deeper than 1000 below (else
 * "no floor"); 8 = first-hit sampling — take the first containing
 * node without the highest-below pick (DrawSnow); 0x10 = same-height
 * fast path reusing the cached FieldArea via AreaMapLastY. Returns
 * 0x80000000 for no floor; a base-material-2 node (the buoyant
 * surface) reports no floor, and MAP_RESULT_FINAL accepts the current
 * result without examining the rest of that leaf list. */
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
    FieldAttrib = MAP_ATTRIBUTE_UNRESOLVED;
    z = z / 10;
    y2 = y / 10;
    yy = LEVEL_NONE;
    /* Wrapper retained for code layout; its original source construct is unknown. */
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
                    if (NODE_INDEX_ROW_FIELD(row, x1) <= x &&
                        x <= NODE_INDEX_ROW_FIELD(row, x2) &&
                        NODE_INDEX_ROW_FIELD(row, z1) <= z &&
                        z <= NODE_INDEX_ROW_FIELD(row, z2))
                    {
                        nn = NODE_INDEX_ROW_FIELD(row, n);
                        list = (AreaNodeType *)*row;
                        n = 0;
                        if (nn < 0)
                        {
                            qx = (x - NODE_INDEX_ROW_FIELD(row, x1)) *
                                     AREA_INDEX_AXIS_SIZE /
                                 (NODE_INDEX_ROW_FIELD(row, x2) -
                                  NODE_INDEX_ROW_FIELD(row, x1));
                            qz = (z - NODE_INDEX_ROW_FIELD(row, z1)) *
                                     AREA_INDEX_AXIS_SIZE /
                                 (NODE_INDEX_ROW_FIELD(row, z2) -
                                  NODE_INDEX_ROW_FIELD(row, z1));
                            n = ((IndexArrayType *)*row)->array[qz][qx];
                            if (n == AREA_NODE_INDEX_NONE)
                                goto next;
                            list = (AreaNodeType *)((IndexArrayType *)*row)->index;
                            nn = -nn;
                        }
                        if (n < nn)
                        {
                            node = (AreaNodeType *)(n * (long)sizeof(AreaNodeType) +
                                                    (long)list);
                        inner:
                            if (z < node->z1)
                                goto next;
                            if (node->x1 <= x && x <= node->x2 && z <= node->z2)
                            {
                                FieldIndex = index;
                                FieldArea = node;
                                if (first_hit)
                                {
                                    if (node->division == AREA_DIVISION_ALL)
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
                                    if (FieldAttrib & MAP_RESULT_FINAL)
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
                    row += NODE_INDEX_ROW_WORDS;
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
