#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct TexScroll * SetupTexScroll(struct GsIMAGE *img, short x, short y, short mode);
 *     EFFECT.C:1853, 27 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsIMAGE * img
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short mode
 * END PSX.SYM */

/*
 * SetupTexScroll (0x80032720, 0x230 bytes) — spawns an EffectSlot pool entry
 * that draws a small animated 2x2-cell water/warp tile grid from a texture
 * page (AddMisc.c passes the just-uploaded TIM's own GsIMAGE plus the two
 * retail scroll velocities). It uses the same round-robin EffectSlot[200]
 * pool search as SetSplash/SetFrame/SetBleed/SetSmoke (see
 * SetSplash.c for the shared idiom writeup — goto loop instead of
 * while(1)+break so loop.c doesn't hoist `&dmy`'s address, idx-computed-
 * before-slot, cursor-update store living inside `if (slot->proc==0)
 * {...break;}` for the right branch polarity).
 *
 * The found slot's `texscroll` payload is retail's shortened form of the
 * PSX.SYM TexScroll record: it keeps px/py, vx/vy, x/y, sx/sy, and image,
 * while omitting the demo's time/count pair. The image RECT receives the
 * TIM's source rectangle; sx/sy hold the shared animated scroll cursor;
 * x/y retain the texture-page destination; and vx/vy receive this call's
 * velocity parameters only at the end, immediately before `proc` is set.
 * This is the retail evolution of the demo SetupTexScroll: both initialise
 * the same texture-scroll fields and run the same 2x2 MoveImage loop, while
 * retail obtains sx/sy globally, removes mode/time/count, and returns through
 * the effect pool instead of returning a standalone TexScroll pointer.
 *
 * Matching notes:
 *  - The 2x2 grid loop uses `short` counters (`j`,`i`), not `int` — a
 *    `short` loop counter suppresses loop.c's strength reduction and keeps
 *    the target's own `(x<<0x10)>>0x10`-style recompute-from-base shape
 *    (cookbook: "a short loop counter suppresses strength reduction").
 *  - The cell-mask test is always true for j,i in the 2x2 grid because
 *    retail selects TEXSCROLL_COPY_ALL, but the target still computes it. Ghidra
 *    renders an extra `& 0x1F` because MIPS variable shifts mask their count
 *    in hardware; retaining that decompiler artifact emits a real `andi`
 *    which is absent from the target.
 *  - `TexScrollX`/`TexScrollY` are read ONCE into named locals right
 *    after the slot is found (not hoisted to the top the way Ghidra's own
 *    SSA rendering shows `sVar2 = DAT_80097f32; sVar3 = DAT_80097f30;` as
 *    the first two statements) — the raw .s doesn't read them until deep
 *    into the found-body, immediately before the texscroll sx/sy stores
 *    (cookbook: "trust the assembly over Ghidra's statement order").
 *  - The outer indefinite loop keeps a meaningful fixed-point
 *    `scrollYShifted` value live through the inner loop. Clearing that scratch
 *    on the exit edge prevents loop.c from incorrectly lifting its signed
 *    conversion; flow later removes the dead clear. The resulting ordinary
 *    pseudo is the target's natural sp+0x14 reload spill, after the vx/vy spills.
 *  - `mask` is a `short` work variable, not a folded literal. That source
 *    identity gives the target's v1/v0/a3 shift chain without a donor fence.
 *  - PsyQ declares `MoveImage` as returning `int`. Even though this caller
 *    ignores the value, the return in v0 changes the hard-register conflicts:
 *    the second multiply result naturally lands in t0. Declaring it `void`
 *    leaves only the final two register bytes unmatched.
 *
 * These source identities and the exact SDK prototype match all 560 bytes.
 */

extern s16 TexScrollX;
extern s16 TexScrollY;

void SetupTexScroll(GsIMAGE *img, short vx, short vy)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    TexScroll *tscr;
    s16 scrollX;
    short scrollY;
    short j;
    short i;

    idx = EFFECT_CURSOR_;
    count = 0;
    base = EffectSlot;
    slot = base + idx;
loop:
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
    if (count > N_EFFECT_SLOTS - 1)
    {
        ef = &dmy;
        goto found;
    }
    goto loop;
found:
{
    u32 scrollYShifted;
    int sx;
    short mask;

    tscr = &ef->param.texscroll;
    ef->param.texscroll.px = tscr->py = 0;

    scrollX = TexScrollX;
    scrollY = TexScrollY;
    tscr->sx = scrollX;
    tscr->sy = scrollY;

    tscr->image.x = tscr->x = img->px;
    tscr->image.y = tscr->y = img->py;
    tscr->image.w = img->pw;
    tscr->image.h = img->ph;
    sx = scrollX;
    scrollYShifted = (u32)(u16)scrollY << 16;

    j = 0;
    while (1)
    {
        for (i = 0; i < TEXSCROLL_GRID_COLUMNS; i++)
        {
            mask = TEXSCROLL_COPY_ALL;
            if ((mask >> (j * TEXSCROLL_GRID_COLUMNS + i)) & 1)
            {
                MoveImage(&tscr->image, sx + img->pw * i,
                          ((s32)scrollYShifted >> 16) + img->ph * j);
            }
        }
        j++;
        if (j >= TEXSCROLL_GRID_ROWS)
        {
            scrollYShifted = 0;
            break;
        }
    }

    TexScrollY += TEXSCROLL_VRAM_SLOT_STRIDE;
    tscr->vx = vx;
    tscr->vy = vy;
    ef->proc = UpdateTexScroll;
    if (TexScrollY > TEXSCROLL_VRAM_Y_LIMIT)
    {
        TexScrollY = TEXSCROLL_VRAM_ORIGIN_Y;
        TexScrollX += TEXSCROLL_VRAM_SLOT_STRIDE;
    }
}
}
