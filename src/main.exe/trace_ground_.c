#include "common.h"
#include "main.exe.h"

extern s32 GetVectorLength(s32 dx, s32 dy, s32 dz);

s32 trace_ground_(VECTOR *from, VECTOR *to, VECTOR *out, u32 flag)
{
    AreaNodeType *hint;
    s32 x, y, z;
    s32 dx, dy, dz;
    s32 step;
    s32 t;
    s32 lx, ly, lz;
    s32 tx, ty, tz;
    s32 rawx, rawy, rawz;

    x = from->vx;
    dx = to->vx - x;
    y = from->vy;
    dy = to->vy - y;
    z = from->vz;
    dz = to->vz - z;
    step = (500 << FIXED_SHIFT) / GetVectorLength(dx, dy, dz);
    lx = x;
    ly = y;
    lz = z;
    hint = 0;
    t = step;
    while (1)
    {
        if (t >= FIXED_ONE)
            break;
        /* The bias preserves signed division's truncation toward zero. */
        rawx = dx * t;
        if (rawx < 0)
            rawx += FIXED_TRUNC_BIAS;
        rawy = dy * t;
        tx = x + (rawx >> FIXED_SHIFT);
        if (rawy < 0)
            rawy += FIXED_TRUNC_BIAS;
        rawz = dz * t;
        ty = y + (rawy >> FIXED_SHIFT);
        if (rawz < 0)
            rawz += FIXED_TRUNC_BIAS;
        tz = z + (rawz >> FIXED_SHIFT);
        if (CGetLevel(&hint, tx, ty, tz, flag) < ty)
            break;
        lx = tx;
        ly = ty;
        lz = tz;
        t += step;
    }
    if (out != 0)
    {
        setVector(out, lx, ly, lz);
    }
    return t;
}
