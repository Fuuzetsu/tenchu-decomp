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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * The distance block uses PSX.SYM's `v1`/`v2` pointer identities, which
 * share $s3/$s2 with the `start`/`end` parameters exactly as aliases
 * would. In the interpolation block PSX.SYM records `t`, `Q` and `R` and
 * NOT the two Bezier coefficients, so those are spelled at their uses --
 * three times each, which is the cost of following the symbol list here
 * rather than a claim that they were single-use.
 */

extern MATRIX GsWSMATRIX;

extern long abs(long value);
extern void GsSortLine(GsLINE *line, GsOT *ot, u16 priority);
extern short DrawModel(ModelType *objp);

static inline void GetWireScreenPosition(long x, long y, long z,
                                         SVECTOR *screen)
{
    MATRIX *matrix = SCREEN_PROJECTION_MATRIX;
    SVECTOR *vector = SCREEN_PROJECTION_POINT;

    matrix->t[0] = 0;
    matrix->t[1] = 0;
    matrix->t[2] = 0;
    setVector(vector, x - ViewInfo.vpx, y - ViewInfo.vpy, z - ViewInfo.vpz);
    SetTransMatrix(matrix);
    SetRotMatrix(&GsWSMATRIX);
    screen->vz = (s16)RotTransPers(vector, (s32 *)screen,
                                   SCREEN_PROJECTION_PERSPECTIVE,
                                   SCREEN_PROJECTION_FLAG);
}

static inline void GetWireRotation(VECTOR *start, VECTOR *end, int *rx,
                                   int *ry)
{
    int dx, dy, dz;

    dx = end->vx - start->vx;
    dz = end->vz - start->vz;
    dy = end->vy - start->vy;
    *ry = ratan2(-dx, -dz);
    *rx = ratan2(dy, SquareRoot0(dx * dx + dz * dz));
}

void SetWire(VECTOR *start, VECTOR *end, VECTOR *center, long len)
{
    enum
    {
        ONE = 4096
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
        VECTOR *v1;
        VECTOR *v2;
        long dx, dy, dz;
        int big;

        v1 = start;
        v2 = end;
        dx = v1->vx - v2->vx;
        dy = v1->vy - v2->vy;
        dz = v1->vz - v2->vz;
        big = 0;
        if (abs(dx) > ONE || abs(dy) > ONE || abs(dz) > ONE)
        {
            big = 1;
        }
        /* The staged flag is byte-required (testing the || directly
         * recolors the delta registers; measured). */
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

    lcount = distance / WIRE_SEG_LEN;
    if (center == 0)
    {
        /* vy/vz go through the freshly assigned alias: byte-required
         * (filling StockCenter first recolors the pointer; measured). */
        StockCenter.vx = (end->vx + start->vx) / 2;
        center = &StockCenter;
        center->vy = (end->vy + start->vy) / 2 + distance / WIRE_SAG_DIV;
        center->vz = (end->vz + start->vz) / 2;
    }

    ecount = lcount * len / ONE;
    i = 0;
    while (1)
    {
        long t, Q, R;
        long one_value;

        if (i >= ecount)
        {
            break;
        }

        /* one_value re-registers ONE for this block: byte-required
         * (using `ONE` directly recolors the sum/negate pair; measured). */
        one_value = ONE;
        t = one_value - i * ONE / lcount;
        Q = t * 2;
        R = t * t / ONE;
        x = ((one_value - Q + R) * end->vx +
             (Q - R * 2) * center->vx + R * start->vx) / ONE;
        y = ((one_value - Q + R) * end->vy +
             (Q - R * 2) * center->vy + R * start->vy) / ONE;
        z = ((one_value - Q + R) * end->vz +
             (Q - R * 2) * center->vz + R * start->vz) / ONE;

        GetWireScreenPosition(x, y, z, &scr);

        if (scr.vz > 0 && oldscr.vz > 0)
        {
            int z;
            int p;

            line.x0 = oldscr.vx;
            line.y0 = oldscr.vy;
            z = scr.vz >> 2;
            line.x1 = scr.vx;
            line.y1 = scr.vy;
            CLAMP_SORT_DEPTH(p, z);
            GsSortLine(&line, OTablePt, (u16)p);
        }
        oldscr = scr;
        i++;
    }

    {
        int rx, ry;

        GetWireRotation(start, end, &rx, &ry);
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
