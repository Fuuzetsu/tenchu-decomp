#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

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
    /* One-shot fence: byte-required (collapse measured; see cookbook). */
    do
    {
        signedValue = value;
    } while (0);
    negative = 0;
    if (signedValue < 0)
    {
        value = -signedValue;
        negative = 1;
    }

    do
    {
        quotient = value / 10;
        baseU = sprite->u;
        sprite->u = baseU + (value % 10) * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        value = quotient;
        sprite->u = baseU;
        sprite->x -= 12;
    } while (quotient != 0);

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

    do
    {
        quotient = value / 10;
        baseU = sprite->u;
        sprite->u = baseU + (value % 10) * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        value = quotient;
        sprite->u = baseU;
        sprite->x -= 12;
    } while (quotient != 0);

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

    do
    {
        quotient = value / 10;
        baseU = sprite->u;
        sprite->u = baseU + (value % 10) * sprite->w;
        GsSortSprite(sprite, OTablePt, 0);
        value = quotient;
        sprite->u = baseU;
        sprite->x -= 12;
    } while (quotient != 0);

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
