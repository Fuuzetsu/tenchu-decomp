#include "common.h"
#include "main.exe.h"
#include "effect.h"

/*
 * MATCH.
 *
 * SetSnow (0x80039160, 0x134 bytes) — EFFECT.C effect-pool allocator:
 * the same EffectSlot[200] round-robin search as SetSplash/SetFrame/
 * SetBleed/FUN_80038fdc/FUN_8003944c (see SetSplash.c for the shared idiom
 * writeup), filling the slot straight from its 4 caller-supplied parameters
 * (a raw "spawn exactly as told" setter, no randomization) and handing it
 * to DrawSnow — a DIFFERENT draw callback from DrawBlood/DrawImpact,
 * still unmatched itself. Called once, from ProcMiscSnowfall.c
 * (`SetSnow(&local_30, &local_40, 0x1000, 0);`) — a falling-snowflake
 * spawner, not blood.
 *
 * SetSnow and DrawSnow jointly prove the shared SnowParticleType fields:
 * position, ground/sample height, draw size, velocity, and sprite selector.
 *
 * The name is high-confidence semantic recovery rather than a transplanted
 * demo symbol: ProcMiscSnowfall is the only caller, this function creates one
 * snow particle, and it installs DrawSnow, whose producer/consumer fields and
 * lifetime agree exactly. Both names were unused.
 *
 * No candidate name in reference/psxsym-candidates.tsv for either this
 * function or DrawSnow; not in the demo's PSX.SYM at all (EFFECT.C
 * functions this deep were apparently a retail-only addition, or the demo
 * spawned snow through a different, simpler path — ProcMiscSnowfall.c
 * itself IS in the demo per its own header, so only this helper is new).
 */
extern void *GlobalAreaMap;
extern void DrawSnow(TEffectSlot *ef);
extern long GetAreaMapLevel(void *area, long x, long y, long z, int mode);

void SetSnow(long *arg0, u16 *arg1, s32 arg2, u8 arg3)
{
    int idx;
    int count;
    TEffectSlot *base;
    TEffectSlot *slot;
    TEffectSlot *ef;
    SnowParticleType *particle;
    u16 vy;

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
    ef->param.snow.x = arg0[0];
    particle = &ef->param.snow;
    particle->y = arg0[1];
    particle->z = arg0[2];
    particle->velocity[0] = arg1[0];
    particle->velocity[1] = arg1[1];
    vy = arg1[2];
    particle->sprite = arg3;
    particle->size = arg2;
    particle->sample_y = particle->y - 8000;
    particle->velocity[2] = vy;
    particle->ground = GetAreaMapLevel(GlobalAreaMap, ef->param.snow.x,
        particle->sample_y, particle->z, 8);
    ef->proc = (void (*)())DrawSnow;
}
