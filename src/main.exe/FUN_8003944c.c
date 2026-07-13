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
 * STATUS: NON_MATCHING — 33 of 248 bytes differ (whole-image count is a
 * fixed offset above that, from the other in-progress NON_MATCHING file in
 * this batch, FUN_8004a6bc — not from this function drifting downstream:
 * the length matches exactly, 248 bytes both).
 *
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
 * Residual: field STORE order matches Ghidra's rendering exactly (pz, vy,
 * vz, scale, [rotate read but not yet stored — same "read early value,
 * store late" idiom as `py`], time, vx, unk22, bright, mode, py, rotate),
 * and the register set/length already match — but cc1's scheduler pushes
 * the scale-store+rotate-load PAIR later (next to the mode/py stores)
 * than the target (which places that pair right after `vz`, before
 * time/vx). Tried: an explicit `rot` temp at several statement positions,
 * reordering vx/time, autorules (no improving type edit) — all
 * byte-identical residual. This is a pure list-scheduling tie, not a
 * source-shape difference tools/autorules.py or manual reordering can
 * reach; `tools/permute.py FUN_8003944c` errored ("does not contain any
 * function") on its pycparser preprocessing pass rather than running —
 * a tooling gap worth fixing centrally, not something to work around by
 * hand here.
 */
extern void DrawImpact(TEffectSlot *ef);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8003944c", FUN_8003944c);
#else
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
    long rot;

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
    rot = param_6;
    pp->vx = param_4;
    pp->time = param_3;
    pp->unk22 = (u8)param_9;
    pp->bright = 0;
    pp->mode = (u8)param_10;
    pp->py = py;
    pp->rotate = rot;
}
#endif
