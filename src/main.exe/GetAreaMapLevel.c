#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapLevel(unsigned long *area, long x, long y, long z, int mode);
 *     CONFLICT.C:107, 69 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 * is the table base) is walked down/up to the row matching y10, then each
 * row entry's rect is tested and ComputeAreaLevel gives the height. The
 * result is cached in FieldArea/FieldIndex/FieldAttrib (attribute) and
 * D_80097EC0 (last y10). Returns level*10 (or the delta to y with mode&2),
 * 0x80000000 when nothing is below.
 *
 * Matching notes (docs/matching-cookbook.md; all verified against the bytes):
 *  - Every loop is a hand-rolled goto loop, NOT while/do-while: real loops
 *    here get jump.c-rotated (do-while) or loop.c-strength-reduced (extra
 *    combined address bases like s8=idx+2 / s0=node+6 appear). Goto loops
 *    keep the original's top-test + conditional back-jump and one cursor.
 *  - x and z are the PARAMETERS reused (`x = x / 10;`) — that is what puts
 *    them in callee-saved homes with the `move s5,a1`/`move s6,a3` prologue
 *    copies; y10 is a separate local (y itself is reloaded from its arg home
 *    slot 0x50 at the end). mode is a u16 parameter: promoted to a word by
 *    the caller, narrowed via `sh` into a local frame slot (0x10) and lhu'd
 *    back on each spilled use.
 *  - The 5th-arg tests split: (mode & 1)/(mode & 0x10) read the still-live
 *    word register; (mode & 8)/(& 4)/(& 2) read the spilled u16 slot.
 *  - p is a `long *` cursor at &idx->index; the row fields are reached as
 *    ((short *)p)[-1..5] (the -2($s2) access proves the cast-based shape) and
 *    the row rect tests re-read the same expressions in the division block
 *    so cse reuses the bounds registers.
 *  - qx/qz are `short`: the (q<<16)>>15 / (q<<16)>>13 sequences are the
 *    sign-extend of the short quotient merged with the *2 and *8 array scaling.
 *  - `node = (AreaNodeType *)((j << 4) + (long)list);` — integer + integer
 *    keeps the operand order (addu s0,v0,a2); `list + j` emits addu s0,a2,v0.
 *  - The tail return-0x80000000 body carries the ret_min label INSIDE the
 *    (y10 < -1000 && !(mode & 4)) body, and the level==MIN / attribute&2
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

extern NodeIndexType *FieldIndex;
extern AreaNodeType *FieldArea;
extern long D_80097EC0; /* last queried y10 */
extern u16 FieldAttrib;  /* attribute of the found node (FieldAttrib) */

extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);

long GetAreaMapLevel(unsigned long *area, long x, long y, long z, u16 mode)
{
    long j;
    long *p;
    NodeIndexType *idx;
    AreaNodeType *node;
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
        if (mode & 1)
            y10 -= 0x96;

        if (y10 == D_80097EC0 && (mode & 0x10)
            && FieldArea->x1 <= x && x <= FieldArea->x2
            && FieldArea->z1 <= z && z <= FieldArea->z2)
        {
            level = ComputeAreaLevel(FieldArea, x, z);
        }
        D_80097EC0 = y10;

        if (idx == (NodeIndexType *)area)
            goto walked;
    down:
        if (!(y10 < idx->y))
            goto walked;
        idx--;
        if (idx != (NodeIndexType *)area)
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
                f8 = mode & 8;
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
                        j = ((IndexArrayType *)list)->array[qz][qx];
                        if (j == -1)
                            goto next;
                        list = (AreaNodeType *)((IndexArrayType *)list)->index;
                        n = -n;
                    }
                    if (j < n)
                    {
                        node = (AreaNodeType *)((j << 4) + (long)list);
                    inner:
                        if (z < node->z1)
                            goto next;
                        if (node->x1 <= x && x <= node->x2 && z <= node->z2)
                        {
                            FieldIndex = idx;
                            FieldArea = node;
                            if (f8)
                            {
                                if (node->division == -1)
                                    level = node->y;
                                else
                                    level = ComputeAreaLevel(node, x, z);
                                FieldAttrib = FieldArea->attribute;
                                goto next;
                            }
                            lv = ComputeAreaLevel(node, x, z);
                            if ((level == 0x80000000 || lv < level) && y10 <= lv)
                            {
                                FieldAttrib = FieldArea->attribute;
                                level = lv;
                                if (FieldAttrib & 0x2000)
                                    goto next;
                            }
                        }
                        j++;
                        node++;
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
        if (y10 < -1000 && (mode & 4) == 0)
        {
        ret_min:
            return 0x80000000;
        }
        ret = level;
    } while (0);
    if (mode & 2)
        ret = y10;
    return ret;
}
