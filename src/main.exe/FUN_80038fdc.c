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
 * The struct written (effect.h's new XF4Type) is a DIFFERENT union member
 * than BloodType at offset 0 (three separate 1-byte stores, not BloodType's
 * 4-byte `hint` pointer) even though it shares BloodType's px/py/pz layout
 * at +4/+8/+0xc — see effect.h's XF4Type comment for the naming evidence
 * (FUN_80036284, this slot's `proc`, calls AddXF4/SetPolyXF4; pairs with
 * the unplaced demo `DrawXF4`, EFFECT.C:1785).
 */
extern void FUN_80036284(TEffectSlot *ef);

void FUN_80038fdc(u8 arg0, u8 arg1, u8 arg2, long arg3)
{
    long tmp;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    XF4Type *fp;

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
    ef->param.xf4.unk0 = arg0;
    fp = &ef->param.xf4;
    fp->unk1 = arg1;
    fp->unk2 = arg2;
    fp->unk10 = 0;
    tmp = GameClock;
    fp->px = arg3;
    fp->py = tmp;
    fp->pz = tmp + 5;
    ef->proc = (void (*)())FUN_80036284;
}
