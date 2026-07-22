#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern long GameClock;
 * END PSX.SYM */

/*
 * FUN_80038fdc (0x80038fdc, 0xc0 bytes) — EFFECT.C effect-pool allocator:
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
extern void FUN_80036284(TEffectSlot *ef);

void FUN_80038fdc(u8 r, u8 g, u8 b, long priority)
{
    long tmp;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    FadeType *fade;

    idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
    count = 0;
    base = EffectSlot;
    slot = base + idx;
loop:
    idx = idx + 1;
    slot = slot + 1;
    if (199 < idx)
    {
        slot = base;
        idx = 0;
    }
    if (slot->proc == 0)
    {
        CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
        if (199 < idx + 1)
        {
            CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
        }
        ef = slot;
        goto found;
    }
    count = count + 1;
    if (199 < count)
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
    fade->mode = 0;
    tmp = GameClock;
    fade->priority = priority;
    fade->start_time = tmp;
    fade->end_time = tmp + 5;
    ef->proc = (void (*)())FUN_80036284;
}
