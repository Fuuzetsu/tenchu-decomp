#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * FUN_8005679c (0x8005679c, 0x11c bytes) — draws a decimal number as a
 * strip of digit sprites off ONE shared GsSPRITE glyph-atlas sprite
 * (libgs.h's proven GsSPRITE: x/y@0x4/0x6, w@0x8, u@0xE): place the sprite
 * at (x, y), then repeatedly peel off the low decimal digit of `dist`
 * (abs value; negative input draws the sign afterward), pick that digit's
 * glyph by offsetting the atlas U coordinate by digit*sp->w, sort/draw the
 * sprite, restore u, and step x left by 12px for the next (more
 * significant) digit — so the number is drawn least-significant-digit
 * first, growing right-to-left, matching a typical fixed-width digit-strip
 * HUD counter. If the input was negative, one more glyph is drawn at
 * U-offset w*10 (an 11th atlas slot past the 10 digits — presumably a
 * "-" glyph) at the final x. Called only via a proc-pointer/indirect site
 * (no direct jal caller found) — no confirmed original name.
 *
 * STATUS: NON_MATCHING — 12 of 284 bytes differ, SAME length as target (71
 * insns both sides). Two structural fixes already folded in (verified via
 * matchdiff): the `if/else` for the `dist<0` guard must be the OPPOSITE
 * polarity of Ghidra's literal "assign default, override on the compare"
 * rendering (write the compare first: `if ((s16)dist<0) neg=true; else
 * dist=... ;` no — see the source below, both branches return early); and
 * the do-while's bottom test wants `(s16)dist != 0`, not Ghidra's literal
 * `(dist & 0xffff) != 0` (the `& 0xffff` mask spelling forces an extra
 * sll/sra pair the target doesn't have). What's left is a pure
 * register-allocation tie in the loop body: the target holds the loop
 * counter in a scratch register copied via an explicit `move $vN,$a1`
 * before scaling it for the atlas-U index math (and copies the
 * incremented value back with a second `move`); our compiled loop scales
 * the incoming register in place with no such copy. A same-length
 * decomp-permuter run (~94k iterations, `--stop-on-zero -j4`) stayed flat
 * around score ~1620-4000 the whole time — never found a 0 — matching the
 * cookbook's permuter-immune register-copy-tie signature. Parked rather
 * than opening a further surgical session.
 */

extern GsOT *OTablePt;
extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, int pri);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005679c", FUN_8005679c);
#else

void FUN_8005679c(GsSPRITE *sp, u32 dist, s16 x, s16 y)
{
    u8 u;
    bool neg;
    s16 digit;

    sp->x = x;
    sp->y = y;
    if ((s16)dist < 0) {
        dist = -(s32)(s16)dist;
        neg = true;
    } else {
        neg = false;
    }
    do {
        digit = (s16)dist;
        dist = (s32)digit / 10;
        u = sp->u;
        sp->u = u + (digit % 10) * sp->w;
        GsSortSprite(sp, OTablePt, 0);
        sp->u = u;
        sp->x = sp->x - 0xc;
    } while ((s16)dist != 0);
    if (neg) {
        sp->u = u + sp->w * '\n';
        GsSortSprite(sp, OTablePt, 0);
        sp->u = u;
    }
}
#endif /* NON_MATCHING */
