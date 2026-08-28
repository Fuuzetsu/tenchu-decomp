#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawFlyWire(struct tag_EffectSlot *ef);
 *     EFFECT.C:1346, 36 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       struct tag_EffectSlot * ef
 *     reg   $s0       struct FlyWireType * param
 *     stack sp+24     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

extern void *memset(void *s, int c, u32 n);

void DrawFlyWire(TEffectSlot *ef)
{
    enum
    {
        m = 5
    };
    FlyWireType *param;
    VECTOR pos;

    param = &ef->param.flywire;
    switch (param->mode)
    {
    case 0:
    {
        s16 time;
        s32 sum;

        time = param->time;
        sum = (u16)param->count + 0x1000 / time;
        param->count = sum;
        if ((s16)sum >= 0x1001)
        {
            param->count = 0;
            param->mode++;
            SetBleeds(&param->end, 0, 0x32, 0xA, 0x1E, 0xFFFF00);
            Sound(CamState.Owner, 0x31);
        }
        else
        {
            SetWire(&param->start, &param->end, &param->center, (s16)sum);
        }
        return;
    }
    case 1:
    {
        VECTOR tmp;
        s16 count;

        memset(&tmp, 0, sizeof(VECTOR));
        count = param->count;
        tmp.vx = ((param->center.vx * (m - count)) + (param->NCenter.vx * count)) / m;
        count = param->count;
        tmp.vy = ((param->center.vy * (m - count)) + (param->NCenter.vy * count)) / m;
        count = param->count;
        tmp.vz = ((param->center.vz * (m - count)) + (param->NCenter.vz * count)) / m;
        pos = tmp;
        SetWire(&param->start, &param->end, &pos, 0x1000);
        if (param->count >= m)
        {
            ef->proc = 0;
        }
        param->count++;
        return;
    }
    }
}
