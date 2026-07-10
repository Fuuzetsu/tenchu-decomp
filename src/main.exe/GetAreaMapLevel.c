#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapLevel(unsigned long *area, long x, long y, long z, int mode);
 *     CONFLICT.C:107, 69 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 *     extern struct GsOT *OTablePt;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/*
 * GetAreaMapLevel (0x80019a10) — area-map floor-height query (this is the
 * map/collision TU, not the item TU). Coordinates are divided by 10 into
 * map units; the NodeIndexType table (FieldIndex caches the last hit, `map`
 * is the table base) is walked down/up to the row matching y10, then each
 * row entry's rect is tested and ComputeAreaLevel gives the height. The
 * result is cached in FieldArea/FieldIndex/FieldAttrib (attribute) and
 * D_80097EC0 (last y10). Returns level*10 (or the delta to y with flag&2),
 * 0x80000000 when nothing is below.
 *
 * Matching notes (docs/matching-cookbook.md; all verified against the bytes):
 *  - Every loop is a hand-rolled goto loop, NOT while/do-while: real loops
 *    here get jump.c-rotated (do-while) or loop.c-strength-reduced (extra
 *    combined address bases like s8=idx+2 / s0=area+6 appear). Goto loops
 *    keep the original's top-test + conditional back-jump and one cursor.
 *  - x and z are the PARAMETERS reused (`x = x / 10;`) — that is what puts
 *    them in callee-saved homes with the `move s5,a1`/`move s6,a3` prologue
 *    copies; y10 is a separate local (y itself is reloaded from its arg home
 *    slot 0x50 at the end). flag is a u16 parameter: promoted to a word by
 *    the caller, narrowed via `sh` into a local frame slot (0x10) and lhu'd
 *    back on each spilled use.
 *  - The 5th-arg tests split: (flag & 1)/(flag & 0x10) read the still-live
 *    word register; (flag & 8)/(& 4)/(& 2) read the spilled u16 slot.
 *  - p is a `long *` cursor at &idx->index; the row fields are reached as
 *    ((short *)p)[-1..5] (the -2($s2) access proves the cast-based shape) and
 *    the row rect tests re-read the same expressions in the division block
 *    so cse reuses the bounds registers.
 *  - qx/qz are `short`: the (q<<16)>>15 / (q<<16)>>13 sequences are the
 *    sign-extend of the short quotient merged with the *2/*8 array scaling.
 *  - `area = (AreaNodeType *)((j << 4) + (long)list);` — integer + integer
 *    keeps the operand order (addu s0,v0,a2); `list + j` emits addu s0,a2,v0.
 *  - The tail return-0x80000000 body carries the ret_min label INSIDE the
 *    (y10 < -1000 && !(flag & 4)) body, and the level==MIN / attribute&2
 *    checks `goto ret_min`: written this way there is exactly ONE
 *    return-MIN body, entered by fallthrough, so it survives inline as
 *    [j epilogue; lui-delay] and the two gotos become branches whose delay
 *    slots reorg fills by stealing that lui (writing separate `return
 *    0x80000000;` statements lets cross-jump merge/invert them differently).
 *  - The (flag & 4) test's bnez lands at E60 past the E54 lhu because reorg's
 *    redundant_insn check sees t1 already holds the flag slot on that path
 *    and steals the following andi into the delay slot instead.
 *  - The do{}while(0) wrapper (found by tools/permute.py) is load-bearing:
 *    see the comment at its site. maspsx needs --expand-div for this file
 *    (true `/` by a variable -> ASPSX's guarded div with break 7/break 6).
 */

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
    long index; /* 0x4 — AreaNodeType* / AreaDivisionType*; 0 terminates */
    s16 x1;     /* 0x8 */
    s16 z1;     /* 0xA */
    s16 x2;     /* 0xC */
    s16 z2;     /* 0xE */
} NodeIndexType;

typedef struct AreaDivisionType
{
    AreaNodeType *area; /* 0x0 */
    s16 index[4][4];    /* 0x4 */
} AreaDivisionType;

extern NodeIndexType *FieldIndex;
extern AreaNodeType *FieldArea;
extern long D_80097EC0; /* last queried y10 */
extern u16 FieldAttrib;  /* attribute of the found area (FieldAttrib) */

extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);

long GetAreaMapLevel(NodeIndexType *map, long x, long y, long z, u16 flag)
{
    long j;
    long *p;
    NodeIndexType *idx;
    AreaNodeType *area;
    AreaNodeType *list;
    long level;
    long n;
    long y10;
    long f8;
    long lv;
    long ret;
    short qx;
    short qz;

    idx = FieldIndex;
    x = x / 10;
    FieldAttrib = 0x81;
    z = z / 10;
    y10 = y / 10;
    level = 0x80000000;
    /* The do{}while(0) wrapper is load-bearing: its loop notes double flow.c's
     * loop_depth ref-weighting for everything inside, which is what pushes the
     * allocation priorities into the original's order (p above idx, n above
     * y10 -> $s2/$s3/$s7/$fp exactly as in the target); the degenerate loop
     * itself generates no code. */
    do
    {
        if (flag & 1)
            y10 -= 0x96;

        if (y10 == D_80097EC0 && (flag & 0x10)
            && FieldArea->x1 <= x && x <= FieldArea->x2
            && FieldArea->z1 <= z && z <= FieldArea->z2)
        {
            level = ComputeAreaLevel(FieldArea, x, z);
        }
        D_80097EC0 = y10;

        if (idx == map)
            goto walked;
    down:
        if (!(y10 < idx->y))
            goto walked;
        idx--;
        if (idx != map)
            goto down;
    walked:
        if (idx->index != 0)
        {
        up:
            if (idx->y < y10)
            {
                idx++;
                if (idx->index != 0)
                    goto up;
            }
            if (idx->index != 0)
            {
                p = &idx->index;
                f8 = flag & 8;
            loop:
                if (level != 0x80000000)
                    goto calc;
                if (((short *)p)[2] <= x && x <= ((short *)p)[4]
                    && ((short *)p)[3] <= z && z <= ((short *)p)[5])
                {
                    n = ((short *)p)[-1];
                    list = (AreaNodeType *)*p;
                    j = 0;
                    if (n < 0)
                    {
                        qx = (x - ((short *)p)[2]) * 4 / (((short *)p)[4] - ((short *)p)[2]);
                        qz = (z - ((short *)p)[3]) * 4 / (((short *)p)[5] - ((short *)p)[3]);
                        j = ((AreaDivisionType *)list)->index[qz][qx];
                        if (j == -1)
                            goto next;
                        list = ((AreaDivisionType *)list)->area;
                        n = -n;
                    }
                    if (j < n)
                    {
                        area = (AreaNodeType *)((j << 4) + (long)list);
                    inner:
                        if (z < area->z1)
                            goto next;
                        if (area->x1 <= x && x <= area->x2 && z <= area->z2)
                        {
                            FieldIndex = idx;
                            FieldArea = area;
                            if (f8)
                            {
                                if (area->division == -1)
                                    level = area->y;
                                else
                                    level = ComputeAreaLevel(area, x, z);
                                FieldAttrib = FieldArea->attribute;
                                goto next;
                            }
                            lv = ComputeAreaLevel(area, x, z);
                            if ((level == 0x80000000 || lv < level) && y10 <= lv)
                            {
                                FieldAttrib = FieldArea->attribute;
                                level = lv;
                                if (FieldAttrib & 0x2000)
                                    goto next;
                            }
                        }
                        j++;
                        area++;
                        if (j < n)
                            goto inner;
                    }
                }
            next:
                p += 4;
                idx++;
                if (*p != 0)
                    goto loop;
            }
        }
        if (level == 0x80000000)
            goto ret_min;
    calc:
        if (FieldAttrib & 2)
            goto ret_min;
        level = level * 10;
        y10 = level - y;
        if (y10 < -1000 && (flag & 4) == 0)
        {
        ret_min:
            return 0x80000000;
        }
        ret = level;
    } while (0);
    if (flag & 2)
        ret = y10;
    return ret;
}
