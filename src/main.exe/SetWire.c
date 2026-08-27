#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetWire(struct VECTOR *start, struct VECTOR *end, struct VECTOR *center, long len);
 *     EFFECT.C:1428, 68 src lines, frame 120 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s3       struct VECTOR * start
 *     param $s2       struct VECTOR * end
 *     param $s1       struct VECTOR * center
 *     param $s0       long len
 *     stack sp+16     struct VECTOR StockCenter
 *     reg   $s4       long lcount
 *     reg   $s0       int i
 *     reg   $s5       int ecount
 *     stack sp+32     struct SVECTOR scr
 *     stack sp+40     struct SVECTOR oldscr
 *     stack sp+72     int x
 *     stack sp+76     int y
 *     reg   $s6       int z
 *     stack sp+48     struct GsLINE line
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s3       struct VECTOR * v1
 *     reg   $s2       struct VECTOR * v2
 *     reg   $a1       long dz
 *     reg   $a3       long dy
 *     reg   $t0       long dx
 *     reg   $v0       long t
 *     reg   $a0       long Q
 *     reg   $a2       long R
 *     stack sp+72     long x
 *     stack sp+76     long y
 *     reg   $s6       long z
 *     reg   $v1       int z
 *     stack sp+64     int rx
 *     stack sp+68     int ry
 *     reg   $s3       struct VECTOR * start
 *     reg   $s2       struct VECTOR * end
 *     reg   $s0       int dz
 *     reg   $s2       int dy
 *     reg   $s1       int dx
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT *OTablePt;
 *     extern struct ModelType *ModelHook;
 * END PSX.SYM */

/*
 * MATCHED: both projection sites use the inline GetScreenPosition structure
 * independently named by PSX.SYM earlier in EFFECT.C.  Keeping the scalar
 * x/y/z parameters and the debug-proven SVECTOR * output together in that
 * helper preserves the target's scheduling and register lifetimes.  It also
 * removes the former loop-store scramble and the claimed 80-byte floor.
 */

extern MATRIX GsWSMATRIX;

extern long abs(long value);
extern void GsSortLine(GsLINE *line, GsOT *ot, u16 priority);
extern short DrawModel(ModelType *objp);

static inline void GetWireScreenPosition(long x, long y, long z,
                                         SVECTOR *screen)
{
    MATRIX *matrix = (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS;
    SVECTOR *vector = (SVECTOR *)TENCHU_SCRATCHPAD(0x20);

    matrix->t[0] = 0;
    matrix->t[1] = 0;
    matrix->t[2] = 0;
    vector->vx = x - (short)ViewInfo.vpx;
    vector->vy = y - (short)ViewInfo.vpy;
    vector->vz = z - (short)ViewInfo.vpz;
    SetTransMatrix(matrix);
    SetRotMatrix(&GsWSMATRIX);
    screen->vz = (s16)RotTransPers(
        vector, (s32 *)screen, (void *)TENCHU_SCRATCHPAD(0x28),
        (void *)TENCHU_SCRATCHPAD(0x2c));
}

void SetWire(VECTOR *start, VECTOR *end, VECTOR *center, long len)
{
    enum
    {
        one = 4096
    };
    VECTOR StockCenter;
    long lcount;
    int i;
    int ecount;
    SVECTOR scr;
    SVECTOR oldscr;
    int x, y, z;
    GsLINE line;
    long distance;

    GetWireScreenPosition(start->vx, start->vy, start->vz, &oldscr);

    line.attribute = 0;
    line.r = 0x50;
    line.g = 0x48;
    line.b = 0x38;

    {
        long dx, dy, dz;
        int big;

        dx = start->vx - end->vx;
        dy = start->vy - end->vy;
        dz = start->vz - end->vz;
        big = 0;
        if (abs(dx) > one || abs(dy) > one || abs(dz) > one)
        {
            big = 1;
        }
        if (big)
        {
            dx /= 0x100;
            dy /= 0x100;
            dz /= 0x100;
            distance = SquareRoot0(dx * dx + dy * dy + dz * dz) << 8;
        }
        else
        {
            distance = SquareRoot0(dx * dx + dy * dy + dz * dz);
        }
    }

    lcount = distance / 300;
    if (center == 0)
    {
        StockCenter.vx = (end->vx + start->vx) / 2;
        center = &StockCenter;
        center->vy = (end->vy + start->vy) / 2 + distance / 32;
        center->vz = (end->vz + start->vz) / 2;
    }

    ecount = lcount * len / one;
    i = 0;
    while (1)
    {
        long t, Q, R;
        long one_value, A, B;

        if (i >= ecount)
        {
            break;
        }

        one_value = one;
        t = one_value - i * one / lcount;
        Q = t * 2;
        R = t * t / one;
        A = one_value - Q + R;
        B = Q - R * 2;
        x = (A * end->vx + B * center->vx + R * start->vx) / one;
        y = (A * end->vy + B * center->vy + R * start->vy) / one;
        z = (A * end->vz + B * center->vz + R * start->vz) / one;

        GetWireScreenPosition(x, y, z, &scr);

        if (((s32)scr.vz << 16) > 0 && oldscr.vz > 0)
        {
            int z;
            int p;

            line.x0 = oldscr.vx;
            line.y0 = oldscr.vy;
            z = ((s32)scr.vz << 16) >> 18;
            line.x1 = scr.vx;
            line.y1 = scr.vy;
            if (z >= 0)
            {
                if (z < 0x4e2)
                    p = z;
                else
                    p = 0x4e1;
            }
            else
            {
                p = 0;
            }
            GsSortLine(&line, OTablePt, (u16)p);
        }
        oldscr = scr;
        i++;
    }

    {
        int rx, ry;

        {
            int *rxp, *ryp;
            int dx, dy, dz;

            dx = end->vx - start->vx;
            dz = end->vz - start->vz;
            dy = end->vy - start->vy;
            rxp = &rx;
            ryp = &ry;
            *ryp = ratan2(-dx, -dz);
            *rxp = ratan2(dy, SquareRoot0(dx * dx + dz * dz));
        }
        ModelHook->locate.coord.t[0] = x;
        ModelHook->locate.coord.t[1] = y;
        ModelHook->locate.coord.t[2] = z;
        ModelHook->rotate.vx = rx;
        ModelHook->rotate.vy = ry;
        ModelHook->rotate.vz = 0;
        UpdateCoordinate(ModelHook);
        DrawModel(ModelHook);
    }
}
