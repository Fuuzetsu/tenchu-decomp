#include "common.h"
#include "main.exe.h"

void draw_digits_(GsSPRITE *sp, u32 dist, s16 x, s16 y)
{
    u8 u;
    bool neg;
    s16 value;
    s32 signed_value;
    s16 quotient;
    s16 multiplier;
    s32 width;
    s32 sign_base;

    value = dist;
    signed_value = (s16)dist;
    sp->x = x;
    sp->y = y;
    if (signed_value < 0)
    {
        value = -signed_value;
        neg = true;
    }
    else
    {
        neg = false;
    }
    do
    {
        quotient = value / 10;
        u = sp->u;
        sp->u += (value % 10) * sp->w;
        GsSortSprite(sp, OTablePt, 0);
        value = quotient;
        sp->u = u;
        sp->x -= 0xc;
    } while ((quotient << 16) != 0);
    sign_base = u & 0xff;
    if (neg)
    {
        width = sp->w;
        multiplier = 10;
        sp->u = sign_base + multiplier * width;
        GsSortSprite(sp, OTablePt, 0);
        sp->u = sign_base;
    }
}
