#ifndef TRIAL_DRAWING_H
#define TRIAL_DRAWING_H

#include "trial.exe.h"
#include "../main.exe/graphics.h"

/* Frames occupy consecutive rows of the same texture page. */
static inline void SortTrialSpriteFrame(GsSPRITE *sprite, int frame)
{
    u32 first_v;

    first_v = sprite->v;
    sprite->v += frame * sprite->h;
    GsSortSprite(sprite, OTablePt, 2);
    sprite->v = first_v;
}

/* Digits are stacked vertically in the texture; draw right to left. */
static inline void DrawTrialNumberInline(GsSPRITE *sprite, s16 value, s16 x, s16 y)
{
    bool negative;
    s16 quotient;

    sprite->x = x;
    sprite->y = y;
    if (value < 0)
    {
        value = -value;
        negative = true;
    }
    else
        negative = false;
    do
    {
        quotient = value / 10;
        SortTrialSpriteFrame(sprite, (s16)(value % 10));
        value = quotient;
        sprite->x -= sprite->w;
    } while (quotient != 0);
    if (negative)
        SortTrialSpriteFrame(sprite, 10);
}

#endif
