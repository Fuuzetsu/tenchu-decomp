#include "common.h"
#include "main.exe.h"
#include "effect.h"

extern void draw_fade_(TEffectSlot *ef);

void set_fade_(u8 r, u8 g, u8 b, long priority)
{
    long start_time;
    int idx;
    TEffectSlot *slot;
    int count;
    FadeType *fade;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->param.fade.r = r;
    fade = &slot->param.fade;
    fade->g = g;
    fade->b = b;
    fade->mode = FADE_MODE_IN;
    start_time = GameClock;
    fade->priority = priority;
    fade->start_time = start_time;
    fade->end_time = start_time + 5;
    slot->proc = draw_fade_;
}
