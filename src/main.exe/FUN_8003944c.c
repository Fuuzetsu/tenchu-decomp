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
 * END PSX.SYM */

/*
 * FUN_8003944c (0x8003944c, 0xf8 bytes) — EFFECT.C effect-pool allocator:
 * same EffectSlot[200] round-robin search as SetSplash/SetFrame/SetBleed/
 * FUN_80038fdc (see SetSplash.c for the full writeup of the shared idioms).
 * It fills the retail ImpactType directly from its ten arguments and hands
 * the slot to DrawImpact: position and optional parent coordinate, starting
 * and ending size/colour, angular state, duration, and sprite type. This is
 * the configurable counterpart to SetImpact's computed defaults. The name
 * is not recovered, so the address name remains rather than inventing one.
 *
 * `time` and `type` are stack args read with `lhu` (16-bit) even though
 * Ghidra types them `undefined1`/`uchar` and only their low byte is ever
 * stored (`sb`) — trusting the raw access width over Ghidra's guess
 * (cookbook: "Ghidra can mistype a stack-passed parameter's width").
 *
 * The two `lw $v1,N($sp)` in the tail are `start_color`/`end_color`
 * rematerialized from their REG_EQUIV incoming slots: register pressure
 * across the search loop is high enough that neither pseudo gets a hard
 * register, so each is re-read at its single use. Both nops there are maspsx
 * load-delay artifacts of a load landing adjacent to its use, not scheduler
 * gaps.
 *
 * The store order below is plain source order, which is the whole trick. A
 * long-standing 16-byte checkpoint tried to explain retail's early
 * `lw $v1,0x14($sp)` as a *hoisted* rotate load and reached for a
 * `do{}while(0)` scheduler fence to move it. That is impossible: a load
 * carries a true dependence on EVERY preceding struct store (sched.c's
 * fixed-scalar/varying-struct dismissal does not fire for a plain sp-relative
 * `mem` against `mem/s` stores), so it can never hoist. Retail's load is early
 * because `param->end_color.word = end_color;` sits right here in source
 * order, and sched2 SANK the store — stores at distinct constant offsets
 * disambiguate freely — until reorg took it for the return delay slot.
 */
extern void DrawImpact(TEffectSlot *ef);

void FUN_8003944c(VECTOR *pos, GsCOORDINATE2 *super,
                  short start_size, short end_size,
                  long start_color, long end_color,
                  u16 rotate, u16 rotate_speed, u16 time, u16 type)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    ImpactType *param;
    long pz;

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
    ef->proc = (void (*)())DrawImpact;
    ef->param.impact.px = pos->vx;
    param = &ef->param.impact;
    param->py = pos->vy;
    pz = pos->vz;
    param->super = super;
    param->rotate = rotate;
    param->rotate_speed = rotate_speed;
    param->start_color.word = start_color;
    param->end_color.word = end_color;
    param->start_size = start_size;
    param->end_size = end_size;
    param->time = (u8)time;
    param->count = 0;
    param->type = (u8)type;
    param->pz = pz;
}
