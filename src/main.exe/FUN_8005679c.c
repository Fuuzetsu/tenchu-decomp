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
 * STATUS: MATCHING — pure C, all 284 bytes exact (71 instructions).
 *
 * The distinct `value` and `quotient` halfword locals preserve the target's
 * caller-saved value copy and callee-saved quotient across GsSortSprite.
 * The one-shot sign-preparation block is also codegen-significant: its loop
 * note gives the saved atlas-U byte enough allocation weight to claim $s1
 * ahead of the sprite pointer, while enclosing the width load produces the
 * target's load-latency sequence `lhu width; li 10; mult` without emitting a
 * runtime branch.
 */

extern GsOT *OTablePt;

void FUN_8005679c(GsSPRITE *sp, u32 dist, s16 x, s16 y)
{
    u8 u;
    bool neg;
    s16 value;
    s32 signed_value;
    s16 quotient;
    s16 multiplier;
    s32 width;
    s32 sign_base;

    value = dist;
    signed_value = (s16)dist;
    sp->x = x;
    sp->y = y;
    if (signed_value < 0) {
        value = -signed_value;
        neg = true;
    } else {
        neg = false;
    }
    do {
        quotient = (s16)value / 10;
        u = sp->u;
        sp->u = u + ((s16)value % 10) * sp->w;
        GsSortSprite(sp, OTablePt, 0);
        value = quotient;
        sp->u = u;
        sp->x = sp->x - 0xc;
    } while ((quotient << 16) != 0);
    if (neg) {
        do {
            sign_base = u & 0xff;
            width = sp->w;
        } while (0);
        multiplier = 10;
        sp->u = sign_base + multiplier * width;
        GsSortSprite(sp, OTablePt, 0);
        sp->u = sign_base;
    }
}
