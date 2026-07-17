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
 * Unlike FUN_80038fdc, this one fills EVERY BloodType field straight from
 * its own 10 parameters (a raw "spawn a blood-family effect exactly as
 * told" setter, not a computed one like SetBlood.c) and hands the slot to
 * DrawImpact (calls RotTransPers/GsSortSprite/GetScreenPosition — the same
 * projection/draw family as DrawBlood.c). Called by DamageControl,
 * ProcItemGosin and
 * ProcItemShinsoku (all still asm) — likely something like SetBloodDirect/
 * AddBloodEffect; no candidate in reference/psxsym-candidates.tsv to
 * corroborate.
 *
 * param_9/param_10 are stack args read with `lhu` (16-bit) even though
 * Ghidra types them `undefined1`/`uchar` and only their low byte is ever
 * stored (`sb`) — trusting the raw access width over Ghidra's guess
 * (cookbook: "Ghidra can mistype a stack-passed parameter's width").
 * effect.h's BloodType gained a real `unk22` field here: offset 0x22 was
 * previously unnamed alignment padding, but this function's `sb` store
 * there is a genuine write, not an artifact.
 *
 * The two `lw $v1,N($sp)` in the tail are param_5/param_6 rematerialized from
 * their REG_EQUIV incoming slots: register pressure across the search loop is
 * high enough that neither pseudo gets a hard register, so reload re-reads
 * each one at its single use. Both nops there are maspsx load-delay artifacts
 * of a load landing adjacent to its use, NOT scheduler gaps.
 *
 * The store order below is plain source order, which is the whole trick. A
 * long-standing 16-byte checkpoint tried to explain retail's early
 * `lw $v1,0x14($sp)` as a *hoisted* rotate load and reached for a
 * `do{}while(0)` scheduler fence to move it. That is impossible: a load
 * carries a true dependence on EVERY preceding struct store (sched.c's
 * fixed-scalar/varying-struct dismissal does not fire for a plain sp-relative
 * `mem` against `mem/s` stores), so it can never hoist. Retail's load is early
 * because `pp->rotate = param_6;` sits right here in source order, and sched2
 * SANK the store — stores at distinct constant offsets disambiguate freely —
 * until reorg took it for the return delay slot. Assigning through a `rot`
 * local instead pushed the load to its use and cost the fence 16 bytes.
 */
extern void DrawImpact(TEffectSlot *ef);

void FUN_8003944c(long *param_1, long param_2, short param_3, short param_4,
                   long param_5, long param_6, u16 param_7, u16 param_8,
                   u16 param_9, u16 param_10)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    BloodType *pp;
    long py;

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
    ef->param.blood.hint = (struct AreaNodeType *)param_1[0];
    pp = &ef->param.blood;
    pp->px = param_1[1];
    py = param_1[2];
    pp->pz = param_2;
    pp->vy = param_7;
    pp->vz = param_8;
    pp->scale = param_5;
    pp->rotate = param_6;
    pp->time = param_3;
    pp->vx = param_4;
    pp->unk22 = (u8)param_9;
    pp->bright = 0;
    pp->mode = (u8)param_10;
    pp->py = py;
}
