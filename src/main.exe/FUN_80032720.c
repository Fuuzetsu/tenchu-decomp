#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 *
 * PSX.SYM suggests this may be `SetPadState` (LOW confidence, EFFECT.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

/*
 * FUN_80032720 (0x80032720, 0x230 bytes) — spawns an EffectSlot pool entry
 * that draws a small animated 2x2-cell water/warp tile grid from a texture
 * page (AddMisc.c: `extern void FUN_80032720(GsIMAGE *im, short y, short
 * z);`, called with the just-uploaded TIM's own GsIMAGE). Same round-robin
 * EffectSlot[200] pool search as SetSplash/SetFrame/SetBleed/SetSmoke (see
 * SetSplash.c for the shared idiom writeup — goto loop instead of
 * while(1)+break so loop.c doesn't hoist `&dmy`'s address, idx-computed-
 * before-slot, cursor-update store living inside `if (slot->proc==0)
 * {...break;}` for the right branch polarity).
 *
 * The found slot's `param` union is written through FIVE different
 * overlapping views (proven by `tools/access.py --order`, not Ghidra's own
 * struct-name guesses):
 *  - smoke.vec.vy/vx (offsets 2/0) zeroed;
 *  - splash.sx/sy (offsets 0xc/0xe) get the pool's own scroll cursor
 *    (`D_80097F30`/`D_80097F32`, a shared animated texture-scroll offset
 *    also advanced at the very end);
 *  - offsets 8/0xa get `im->px`/`im->py` (the GsIMAGE's own texture-page
 *    position) through a RAW OFFSET CAST — this does NOT line up with any
 *    proven named field at that width (BleedType.pos.vz is a 4-byte field
 *    there, not two shorts): a genuinely divergent union member, per the
 *    cookbook's "reach it via an explicit offset cast off the SAME proven
 *    pointer, don't invent a new named struct" rule;
 *  - bleed.vec.vx/vy/vz/pad (offsets 0x10/0x12/0x14/0x16) get the SAME
 *    im->px/im->py plus im->pw/im->ph (the tile's pixel size) — this is
 *    the actual source RECT the 2x2 grid's MoveImage calls read from;
 *  - smoke.vec.vz/pad (offsets 4/6) get this call's own `y`/`z` params,
 *    written only at the very end (right before `ef->proc` is set),
 *    matching the target's own late spill-reload from their stack homes.
 *
 * Matching notes:
 *  - The 2x2 grid loop uses `short` counters (`j`,`i`), not `int` — a
 *    `short` loop counter suppresses loop.c's strength reduction and keeps
 *    the target's own `(x<<0x10)>>0x10`-style recompute-from-base shape
 *    (cookbook: "a short loop counter suppresses strength reduction").
 *  - The bit test `(0xF >> (j*2+i)) & 1` is always true for j,i in {0,1}
 *    (indices 0-3, all set in 0xF), but the target still computes it. Ghidra
 *    renders an extra `& 0x1F` because MIPS variable shifts mask their count
 *    in hardware; retaining that decompiler artifact emits a real `andi`
 *    which is absent from the target.
 *  - `D_80097F30`/`D_80097F32` are read ONCE into named locals right
 *    after the slot is found (not hoisted to the top the way Ghidra's own
 *    SSA rendering shows `sVar2 = DAT_80097f32; sVar3 = DAT_80097f30;` as
 *    the first two statements) — the raw .s doesn't read them until deep
 *    into the found-body, immediately before the splash.sx/sy stores
 *    (cookbook: "trust the assembly over Ghidra's statement order").
 *  - The outer indefinite loop keeps a meaningful fixed-point
 *    `scrollYShifted` value live through the inner loop. Clearing that scratch
 *    on the exit edge prevents loop.c from incorrectly lifting its signed
 *    conversion; flow later removes the dead clear. The resulting ordinary
 *    pseudo is the target's natural sp+0x14 reload spill, after the y/z spills.
 *  - `mask` is a `short` work variable, not a folded literal. That source
 *    identity gives the target's v1/v0/a3 shift chain without a donor fence.
 *  - PsyQ declares `MoveImage` as returning `int`. Even though this caller
 *    ignores the value, the return in v0 changes the hard-register conflicts:
 *    the second multiply result naturally lands in t0. Declaring it `void`
 *    leaves only the final two register bytes unmatched.
 *
 * These source identities and the exact SDK prototype match all 560 bytes.
 */

extern void UpdateTexScroll(void);
extern s16 D_80097F30;
extern s16 D_80097F32;

void FUN_80032720(GsIMAGE *im, short y, short z)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    union EffectParam *pp;
    s16 scrollX;
    short scrollY;
    short px;
    short py;
    short j;
    short i;

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
{
    u32 scrollYShifted;
    s32 signedScrollY;
    int sx;
    short mask;

    pp = &ef->param;
    pp->smoke.vec.vy = 0;
    ef->param.smoke.vec.vx = 0;

    scrollX = D_80097F30;
    scrollY = D_80097F32;
    pp->splash.sx = scrollX;
    pp->splash.sy = scrollY;

    px = im->px;
    *(short *)((u8 *)pp + 8) = px;
    pp->bleed.vec.vx = px;
    py = im->py;
    *(short *)((u8 *)pp + 0xA) = py;
    pp->bleed.vec.vy = py;
    pp->bleed.vec.vz = im->pw;
    pp->bleed.vec.pad = im->ph;
    sx = scrollX;
    scrollYShifted = (u32)(u16)scrollY << 16;

    j = 0;
    while (1)
    {
        for (i = 0; i < 2; i++)
        {
            mask = 0xF;
            if ((mask >> (j * 2 + i)) & 1)
            {
                signedScrollY = (s32)scrollYShifted >> 16;
                MoveImage((RECT *)&pp->bleed.vec, sx + im->pw * i,
                          signedScrollY + im->ph * j);
            }
        }
        j++;
        if (j >= 2)
        {
            scrollYShifted = 0;
            break;
        }
    }

    D_80097F32 = D_80097F32 + 0x40;
    pp->smoke.vec.vz = y;
    pp->smoke.vec.pad = z;
    ef->proc = (void (*)())UpdateTexScroll;
    if (0x200 < D_80097F32)
    {
        D_80097F32 = 0x100;
        D_80097F30 = D_80097F30 + 0x40;
    }
}
}
