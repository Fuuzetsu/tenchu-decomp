#include "common.h"
#include "main.exe.h"
#include "effect.h"

/*
 * set_fade_ (0x80038fdc, 0xc0 bytes) — EFFECT.C effect-pool allocator:
 * same EffectSlot[200] round-robin search as SetSplash/SetFrame/SetBleed
 * (see SetSplash.c for the indexed do-while reconstruction).
 * Called only by (still-asm) CVAupdate, which also drives SetBlood/
 * SetNowMotion/SoundEx/SetupTelop for the same cutscene-ish sequence.
 *
 * The FadeType written here is a DIFFERENT union member than BloodType at
 * offset 0: three separate color-byte stores, not BloodType's pointer. Its
 * renderer uses +4 as an ordering-table priority and +8/+0xc as the fade's
 * clock interval, so those fields must not inherit BloodType's position
 * names.
 */
extern void draw_fade_(TEffectSlot *ef);

void set_fade_(u8 r, u8 g, u8 b, long priority)
{
    long start_time;
    int idx;
    TEffectSlot *slot;
    int count;
    FadeType *fade;

    idx = EFFECT_CURSOR_;
    count = 0;
    do
    {
        idx++;
        if (idx >= N_EFFECT_SLOTS)
        {
            idx = 0;
        }
        if (EffectSlot[idx].proc == 0)
        {
            EFFECT_CURSOR_ = idx + 1;
            if (EFFECT_CURSOR_ >= N_EFFECT_SLOTS)
            {
                EFFECT_CURSOR_ = 0;
            }
            slot = &EffectSlot[idx];
            goto found;
        }
        count++;
    } while (count < N_EFFECT_SLOTS);
    slot = &dmy;
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
