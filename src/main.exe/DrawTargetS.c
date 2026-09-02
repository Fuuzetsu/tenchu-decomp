#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTargetS(long x, long y, long z, long color);
 *     EFFECT.C:442, 23 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       long x
 *     param $s1       long y
 *     param $s3       long z
 *     param $a3       long color
 *     stack sp+16     struct GsLINE line
 *     reg   $a2       int z
 * END PSX.SYM */

void DrawTargetS(long x, long y, long z, long color)
{
    GsLINE line;
    long priority;

    z >>= 2;
    priority = z < 0 ? 0 :
        (z >= DEPTH_LIMIT ? DEPTH_LIMIT - 1 : z);

    line.r = (u8)(color >> 16);
    line.attribute = 0;
    line.g = (u8)(color >> 8);
    line.b = (u8)color;
    if (color < 0)
    {
        line.x0 = x - 20;
        line.y0 = y - 20;
        line.x1 = x + 20;
        line.y1 = y + 20;
        GsSortLine(&line, OTablePt, priority);
        line.x0 = x + 20;
        line.y0 = y - 20;
        line.x1 = x - 20;
        line.y1 = y + 20;
        GsSortLine(&line, OTablePt, priority);
    }
    else
    {
        line.x0 = x - 2;
        line.y0 = y - 2;
        line.x1 = x + 2;
        line.y1 = y + 2;
        GsSortLine(&line, OTablePt, priority);
        line.x0 = x + 2;
        line.y0 = y - 2;
        line.x1 = x - 2;
        line.y1 = y + 2;
        GsSortLine(&line, OTablePt, priority);
    }
}
