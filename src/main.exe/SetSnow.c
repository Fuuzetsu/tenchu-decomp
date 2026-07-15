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
 * The fields line up with effect.h's BloodType byte-for-byte (same as
 * FUN_8003944c), but DrawSnow's own Ghidra decompilation proves this
 * pool slot is really a GENERIC POSITION+VELOCITY particle for this caller,
 * not blood: it reads hint/px/py back as plain `long`s (a position, not a
 * pointer), adds time/vx/vy back as `short` DELTAS to them (velocity, not a
 * countdown/random-jitter), and compares the px+vx result against `pz`
 * (a ground-level threshold from GetAreaMapLevel) to kill the effect once
 * the particle sinks below the floor — a falling-particle update, not
 * blood's splatter/fade. None of this is corroborated by a MATCHED
 * function, so effect.h's BloodType names are kept as-is (same convention
 * as FUN_8003944c) rather than inventing a renamed/new union view from an
 * unmatched sibling's Ghidra output.
 *
 * The name is high-confidence semantic recovery rather than a transplanted
 * demo symbol: ProcMiscSnowfall is the only caller, this function creates one
 * snow particle, and it installs DrawSnow, whose producer/consumer fields and
 * lifetime agree exactly. Both names were unused.
 *
 * One field genuinely diverges in WIDTH, independent of the naming
 * question above: `arg3` (a single byte) is written at param+0x1e, which
 * BloodType calls `vz` (a `short`, proven correct elsewhere — FUN_8003944c
 * stores a real `u16` there). DrawSnow's own decompilation reads
 * EXACTLY that byte back (`*(byte *)((int)param_1 + 0x22)` — param_1 is
 * EF-based there, so ef+0x22 == param+0x1e) as a table INDEX (a sprite/kind
 * selector), never as part of a 2-byte value — proving this caller's slot
 * uses only the LOW byte of `vz`'s slot for something else entirely, a
 * genuine divergent-union case (cookbook: "reach a divergent-value field
 * via an offset cast off the same proven pointer"), so it's written via a
 * raw `u8` offset store rather than through `.blood.vz`.
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
    BloodType *pp;
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
    ef->param.blood.hint = (struct AreaNodeType *)arg0[0];
    pp = &ef->param.blood;
    pp->px = arg0[1];
    pp->py = arg0[2];
    pp->time = arg1[0];
    pp->vx = arg1[1];
    vy = arg1[2];
    *((u8 *)pp + 0x1e) = arg3;
    pp->rotate = arg2;
    pp->scale = pp->px - 8000;
    pp->vy = vy;
    pp->pz = GetAreaMapLevel(GlobalAreaMap, (long)ef->param.blood.hint, pp->scale, pp->py, 8);
    ef->proc = (void (*)())DrawSnow;
}
