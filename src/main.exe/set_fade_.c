#include "common.h"
#include "main.exe.h"
#include "effect.h"

/*
 * set_fade_ (0x80038fdc, 0xc0 bytes) — EFFECT.C effect-pool allocator:
 * same EffectSlot[200] round-robin search as SetSplash/SetFrame/SetBleed
 * (see SetSplash.c for the full writeup of the shared idioms — goto loop
 * instead of while(1)+break so loop.c doesn't hoist &dmy's address,
 * idx-computed-before-slot so idx/slot land in the target's t0/v1 pair).
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
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    FadeType *fade;

    idx = EFFECT_CURSOR_;
    count = 0;
    base = EffectSlot;
    slot = base + idx;
loop:
    idx++;
    slot++;
    if (idx > N_EFFECT_SLOTS - 1)
    {
        slot = base;
        idx = 0;
    }
    if (slot->proc == 0)
    {
        EFFECT_CURSOR_ = idx + 1;
        if (N_EFFECT_SLOTS - 1 < idx + 1)
        {
            EFFECT_CURSOR_ = 0;
        }
        ef = slot;
        goto found;
    }
    count++;
    if (count > N_EFFECT_SLOTS - 1)
    {
        ef = &dmy;
        goto found;
    }
    goto loop;
found:
    ef->param.fade.r = r;
    fade = &ef->param.fade;
    fade->g = g;
    fade->b = b;
    fade->mode = FADE_MODE_IN;
    start_time = GameClock;
    fade->priority = priority;
    fade->start_time = start_time;
    fade->end_time = start_time + 5;
    ef->proc = (void (*)())draw_fade_;
}
