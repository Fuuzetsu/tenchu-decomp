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
 *  - The bit test `(0xF >> (j*2+i)) & 1` is always true for j,i in {0,1}
 *    (indices 0-3, all set in 0xF), but the target still computes it. Ghidra
 *    renders an extra `& 0x1F` because MIPS variable shifts mask their count
 *    in hardware; retaining that decompiler artifact emits a real `andi`
 *    which is absent from the target.
 *  - `D_80097F30`/`D_80097F32` are read ONCE into named locals right
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

extern s16 D_80097F30;
extern s16 D_80097F32;

void SetupTexScroll(GsIMAGE *im, short vx, short vy)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    TexScroll *tscr;
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

    tscr = &ef->param.texscroll;
    ef->param.texscroll.px = tscr->py = 0;

    scrollX = D_80097F30;
    scrollY = D_80097F32;
    tscr->sx = scrollX;
    tscr->sy = scrollY;

    px = im->px;
    tscr->x = px;
    tscr->image.x = px;
    py = im->py;
    tscr->y = py;
    tscr->image.y = py;
    tscr->image.w = im->pw;
    tscr->image.h = im->ph;
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
                MoveImage(&tscr->image, sx + im->pw * i,
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
    tscr->vx = vx;
    tscr->vy = vy;
    ef->proc = (void (*)())UpdateTexScroll;
    if (0x200 < D_80097F32)
    {
        D_80097F32 = 0x100;
        D_80097F30 = D_80097F30 + 0x40;
    }
}
}
