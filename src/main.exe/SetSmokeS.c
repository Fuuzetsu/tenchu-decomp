#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSmokeS(struct VECTOR *pos, short vx, short vy, short vz, int time);
 *     EFFECT.C:858, 14 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       short vx
 *     param $a2       short vy
 *     param $a3       short vz
 *     param stack+16  int time
 * END PSX.SYM */

/*
 * MATCHED.
 *  - Retail narrowed the demo build's `int time` parameter to `unsigned short`:
 *    the target loads stack+16 with `lhu`. A separate signed `t = (short)time`
 *    keeps the raw value for the byte store while reproducing the target's
 *    signed guarded remainder and two-instruction sign extension.
 *  - `m = smoke->time - 1` must remain its own statement, as in SetSmoke.
 *    Inlining it lets fold reassociate the subtraction into `sum + 1`, moving
 *    the addiu to the wrong side of the final expression.
 *  - The pool scan uses the SetSmoke/SetExplosion round-robin do-while shape,
 *    with the fallback slot after the loop and the cursor update on success.
 */
extern void DrawSmoke(TEffectSlot *ef);

void SetSmokeS(VECTOR *pos, short vx, short vy, short vz, unsigned short time)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    SmokeType *smoke;
    int r;
    int t;
    int m;

    count = 0;
    base = EffectSlot;
    idx = EFFECT_CURSOR_;
    slot = base + idx;
    do
    {
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
    } while (count < N_EFFECT_SLOTS);
    ef = &dmy;
found:
    smoke = &ef->param.smoke;
    r = rand();
    smoke->scale = r % 0x2000 + 0x1000;
    smoke->rotate = (rand() % 360) * 0x1000;
    smoke->pos.vx = pos->vx;
    smoke->pos.vy = pos->vy;
    smoke->pos.vz = pos->vz;
    smoke->vec.vx = vx;
    smoke->vec.vy = vy;
    smoke->vec.vz = vz;
    smoke->time = time;
    r = rand();
    t = (short)time;
    smoke->sprite = 0;
    m = smoke->time - 1;
    smoke->evtime = m - (t / 2 + r % t);
    ef->proc = (void (*)())DrawSmoke;
}
