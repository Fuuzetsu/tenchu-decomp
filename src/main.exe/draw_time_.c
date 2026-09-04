#include "common.h"
#include "main.exe.h"

#define DRAW_DECIMAL_STRIP(sprite, value, quotient, base_u)                  \
    do                                                                       \
    {                                                                        \
        (quotient) = (value) / 10;                                           \
        (base_u) = (sprite)->u;                                              \
        (sprite)->u = (base_u) + ((value) % 10) * (sprite)->w;               \
        GsSortSprite((sprite), OTablePt, 0);                                 \
        (value) = (quotient);                                                \
        (sprite)->u = (base_u);                                              \
        (sprite)->x -= 12;                                                   \
    } while ((quotient) != 0)

/* Draw a minutes/seconds time value and optional separator from a digit sprite. */
void draw_time_(GsSPRITE *sprite, s32 time, s32 x, s32 y, s32 drawColon)
{
    s16 value;
    s32 signedValue;
    s16 quotient;
    s32 negative;
    s32 lastNegative;
    s16 multiplier;
    s32 signBase;
    u8 baseU;

    time /= 30;
    value = time / 60;
    sprite->y = y;
    sprite->x = x - 0x20;
    signedValue = value;
    if (signedValue < 0)
    {
        value = -signedValue;
        negative = 1;
    }
    else
    {
        negative = 0;
    }

    DRAW_DECIMAL_STRIP(sprite, value, quotient, baseU);

    if (negative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        sprite->u = signBase;
    }

    time %= 60;
    sprite->y = y;
    sprite->x = x - 12;
    value = time / 10;
    signedValue = value;
    negative = 0;
    if (signedValue < 0)
    {
        value = -signedValue;
        negative = 1;
    }

    DRAW_DECIMAL_STRIP(sprite, value, quotient, baseU);

    if (negative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        sprite->u = signBase;
    }

    sprite->x = x;
    value = time % 10;
    signedValue = value;
    sprite->y = y;
    if (signedValue < 0)
    {
        value = -signedValue;
        lastNegative = 1;
    }
    else
    {
        lastNegative = 0;
    }

    DRAW_DECIMAL_STRIP(sprite, value, quotient, baseU);

    if (lastNegative != 0)
    {
        multiplier = 10;
        signBase = baseU & 0xff;
        sprite->u = signBase + multiplier * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        sprite->u = signBase;
    }

    if (drawColon != 0)
    {
        multiplier = 11;
        signBase = sprite->u;
        sprite->u = signBase + multiplier * sprite->w;
        sprite->x = x - 0x16;
        GsSortSprite(sprite, OTablePt, 0);
        sprite->u = signBase;
    }
}
