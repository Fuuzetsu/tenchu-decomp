#include "common.h"
#include "main.exe.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetLightningI(struct VECTOR *start, struct VECTOR *end, int gen, short r, int g, int b);
 *     EFFECT.C:1500, 60 src lines, frame 152 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s6       struct VECTOR * start
 *     param $s7       struct VECTOR * end
 *     param stack+4294967216 int gen
 *     param stack+4294967224 short r
 *     param stack+16  int g
 *     param stack+20  int b
 *     stack sp+88     short g
 *     stack sp+96     short b
 *     reg   $s0       long lcount
 *     reg   $s4       int i
 *     stack sp+24     struct SVECTOR scr
 *     stack sp+32     struct SVECTOR oldscr
 *     reg   $s1       int x
 *     reg   $s2       int y
 *     reg   $s3       int z
 *     stack sp+40     struct GsLINE line
 *     reg   $v0       long x
 *     reg   $t1       long y
 *     reg   $t2       long z
 *     reg   $s6       struct VECTOR * v1
 *     reg   $s7       struct VECTOR * v2
 *     reg   $a1       long dz
 *     reg   $a3       long dy
 *     reg   $t0       long dx
 *     stack sp+56     struct VECTOR sv
 *     reg   $s1       long x
 *     reg   $s2       long y
 *     reg   $s3       long z
 *     reg   $fp       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * MATCHED: the source-level structure is the pair of small projection helpers
 * independently named by PSX.SYM in the same EFFECT.C translation unit:
 * PrepareGetScreenPositionS followed by GetScreenPositionS.  Keeping those
 * helpers inline preserves their scalar parameter identities at both call sites.
 * The debug-proven SVECTOR * type for the output is essential: screen->vz
 * retains the memory dependency that schedules the result store before the next
 * projection work.  This natural decomposition removes the former identical-arm
 * fence, pointer carriers, and the purported 15-byte allocation floor.
 */

extern MATRIX GsWSMATRIX;

extern long abs(long value);
extern int rand(void);
extern void *memset(void *dst, int value, u32 size);
extern void GsSortLine(GsLINE *line, GsOT *ot, u16 priority);

static inline void PrepareLightningScreenPosition(void)
{
    MATRIX *matrix = (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS;

    matrix->t[0] = 0;
    matrix->t[1] = 0;
    matrix->t[2] = 0;
    SetTransMatrix(matrix);
    SetRotMatrix(&GsWSMATRIX);
}

static inline void GetLightningScreenPosition(long x, long y, long z,
                                              SVECTOR *screen)
{
    SVECTOR *vector = (SVECTOR *)TENCHU_SCRATCHPAD(0x80);

    vector->vx = x - (short)ViewInfo.vpx;
    vector->vy = y - (short)ViewInfo.vpy;
    vector->vz = z - (short)ViewInfo.vpz;
    screen->vz = (s16)RotTransPers(
        vector, (s32 *)screen, (s32 *)TENCHU_SCRATCHPAD_ADDRESS,
        (s32 *)TENCHU_SCRATCHPAD(0x10));
}

void SetLightningI(VECTOR *start, VECTOR *end, int gen, short r, short g, short b)
{
    enum
    {
        SplitLen = 200
    };
    enum
    {
        Range = 80
    };
    SVECTOR scr;
    SVECTOR oldscr;
    GsLINE line;
    VECTOR sv;
    int next_gen;
    short lr;
    short lg;
    short lb;
    long distance;
    long lcount;
    int i;
    int x;
    int y;
    int z;

    lr = r;
    next_gen = gen - 1;
    lg = g;
    lb = b;
    if (gen != 0)
    {
        PrepareLightningScreenPosition();
        GetLightningScreenPosition(start->vx, start->vy, start->vz, &oldscr);

        line.attribute = GS_ATTR_SEMITRANS_ADD;
        line.r = lr;
        line.g = lg;
        line.b = lb;

        {
            VECTOR *v1, *v2;
            long dx, dy, dz;
            int large;

            v1 = start;
            v2 = end;
            large = 0;
            dx = v1->vx - v2->vx;
            dy = v1->vy - v2->vy;
            dz = v1->vz - v2->vz;
            if (abs(dx) > 0x1000 || abs(dy) > 0x1000 || abs(dz) > 0x1000)
            {
                large = 1;
            }
            if (large)
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

        lcount = distance / SplitLen;
        i = 1;
        if (lcount > 0)
        {
            while (1)
            {
                if (i >= lcount)
                {
                    break;
                }

                x = ((end->vx - start->vx) * i) / lcount + start->vx;
                y = ((end->vy - start->vy) * i) / lcount + start->vy;
                z = ((end->vz - start->vz) * i) / lcount + start->vz;
                x += -Range + rand() % (Range * 2);
                y += -Range + rand() % (Range * 2);
                z += -Range + rand() % (Range * 2);

                if ((rand() & 2) == 0)
                {
                    memset(&sv, 0, sizeof(VECTOR));
                    sv.vx = x;
                    sv.vy = y;
                    sv.vz = z;
                    SetLightningI(&sv, end, next_gen, lr, lg, lb);
                }

                GetLightningScreenPosition(x, y, z, &scr);

                if (scr.vz > 0 && oldscr.vz > 0)
                {
                    int z;
                    int p;

                    line.x0 = oldscr.vx;
                    line.y0 = oldscr.vy;
                    z = scr.vz >> 2;
                    line.x1 = scr.vx;
                    line.y1 = scr.vy;
                    if (z >= 0)
                    {
                        if (z < DEPTH_LIMIT)
                            p = z;
                        else
                            p = DEPTH_LIMIT - 1;
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
        }
    }
}
