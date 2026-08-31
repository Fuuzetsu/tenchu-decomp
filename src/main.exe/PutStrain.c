#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutStrain(void);
 *     INFOVIEW.C:218, 70 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       int newpow
 *     reg   $a2       int r
 *     reg   $s1       struct GsSPRITE * spr
 *
 * Globals it touches, as the original declared them:
 *     extern long StrainRatio;
 *     extern long GameClock;
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern s32 StrainRatio;
extern u16 StrainPhase;

/*
 * PutStrain (0x8004a8f0) — draws the "strain" HUD icon at (x,y): a
 * flashing/pulsing warning glyph whose sprite and pulse phase depend on
 * StrainRatio. 0x7fffffff (a sentinel) draws nothing at all. Otherwise:
 * ratio==0 -> KehaiRedImage (no pulse-drift clamp); ratio <
 * -20000 -> the retail-only KehaiCriticalImage (clamps ratio to 0 for the
 * pulse calc); -20000<=ratio<0 -> KehaiYellowImage (also plays
 * SoundEx(0,0xe) every 30 ticks — a warning beep — and clamps ratio to 0);
 * 0<ratio<=20000 draws a
 * PutNumber-style right-to-left digit strip of `(20000-ratio)/200` using
 * NumberImage (the numeric strain percentage) THEN uses KehaiGreenImage;
 * ratio>20000 returns without drawing anything.
 * The chosen icon sprite is then positioned at (x,y), tinted a shade of
 * gray that oscillates via rsin() driven by a persistent phase counter
 * (StrainPhase, advanced by the (adjusted) strain delta), and scaled by a
 * strain-proportional factor, then GsSortSprite'd.
 *
 * Matching notes:
 *  - `GameClock == (GameClock / speed) * speed` is EndDrawing.c's proven
 *    div-by-30 modulo-test spelling (the magic-multiply reproduces from the
 *    div/mul, not `% speed == 0`).
 *  - The digit loop is PutNumber.c's own do-while shape (goto-free real
 *    do-while; `r` is the quotient, reused as the next iteration's dividend
 *    exactly like PutNumber's `q`).
 *  - `ratio` is ONE variable doing double duty: the dispatch value AND (in
 *    the two negative branches, reset to 0) the value the tail geometry
 *    reads — Ghidra's decompilation renders these as two different
 *    variables (`lVar6`/`StrainRatio`) but m2c's raw register trace shows
 *    a single reused pseudo across both roles.
 *  - The phase-advance uses `delta` ADJUSTED (+0x1f when negative, an
 *    arithmetic-shift-rounds-toward-negative-infinity correction before the
 *    `>>5`), but the scale factor uses the RAW (unadjusted) `delta` — two
 *    separate reads of the same `powrange - ratio` expression, not one shared
 *    temp.
 *  - CRITICAL: `NumberImage.u` is NOT read/restored globally — Ghidra's
 *    `uVar1 = NumberImage.u;` at the very top (before the StrainRatio
 *    dispatch) is a decompiler artifact; the target never touches
 *    NumberImage at all in the three simple (non-digit-loop) branches.
 *    `base`/`img` are LOCAL to the digit-loop (`else`) branch only — the
 *    earlier draft that hoisted them to function scope cost an extra
 *    saved register (frame 56 vs target's 48) and never converged; scoping
 *    them into the branch fixed the length exactly.
 *  - The first digit-branch access is the direct `NumberImage.w = 4`, then
 *    `img = &NumberImage`. That creates the target's address pseudo followed
 *    by the separate `$s1` copy; writing `img->w = 4` after the assignment
 *    lets cc1 form the address directly in `$s1` and loses one instruction.
 *    Reading `base` later through `img` also leaves its `lbu` in the target
 *    slot between the y producer and store.
 *  - A folded `u8` consumer identity at the final `img->u = base` write raises
 *    `base` above `spr` in global allocation, placing them in the target's
 *    `$s3`/`$s4` respectively without a zero-trip loop.
 *  - `phase` is genuinely unsigned: the target passes it to `rsin` with one
 *    `andi`, not a signed `sll`/`sra` pair.
 *
 * MATCH — exact 584-byte / 146-instruction pure-C match.
 */
void PutStrain(s32 x, s32 y)
{
    enum
    {
        speed = 30
    };
    enum
    {
        range = 255,
        powrange = 20000
    };
    s32 ratio;
    GsSPRITE *spr;
    s32 delta;
    s32 s;
    u16 phase;
    u8 shade;
    s16 scale;

    ratio = StrainRatio;
    if (ratio != 0x7fffffff)
    {
        if (ratio == 0)
        {
            spr = &KehaiRedImage;
        }
        else if (ratio < -powrange)
        {
            spr = &KehaiCriticalImage;
            ratio = 0;
        }
        else if (ratio < 0)
        {
            spr = &KehaiYellowImage;
            ratio = 0;
            if (GameClock == (GameClock / speed) * speed)
            {
                SoundEx(0, SE_WARNING_BEEP);
            }
        }
        else
        {
            u8 base;
            GsSPRITE *img;
            s32 newpow;
            s32 r;

            if (ratio > powrange)
                return;
            spr = &KehaiGreenImage;
            NumberImage.w = 4;
            img = &NumberImage;
            img->x = (s16)(x + 0x22);
            base = img->u;
            img->y = (s16)(y + 8);
            newpow = (powrange - ratio) / 200;
        strainloop:
            r = newpow / 10;
            img->u = base + (newpow % 10) * 4;
            GsSortSprite(img, OTablePt, 0);
            img->x -= 6;
            newpow = r;
            if (newpow != 0)
                goto strainloop;
            /* allocation staging: folded after flow -- not recovered arithmetic */
            img->u = (base + base) - base;
        }

        /* This is cc1's own signed-divide-by-32 expansion, and unlike
         * the one below it does NOT fold back: `delta / 32` costs 6
         * lines, because `delta` is still live for the scale below
         * and the schedule differs. */
        delta = powrange - ratio;
        s = delta;
        if (delta < 0)
            s = delta + 0x1f;

        spr->x = (s16)x;
        spr->y = (s16)y;
        phase = StrainPhase + (s >> 5);
        StrainPhase = phase;
        shade = rsin(phase) * 0x60 / FIXED_ONE + range / 2;
        spr->b = shade;
        spr->g = shade;
        spr->r = shade;
        scale = (s16)((delta << 0xb) / powrange) + FIXED_HALF;
        spr->scalex = scale;
        spr->scaley = scale;
        GsSortSprite(spr, OTablePt, 0);
    }
}
