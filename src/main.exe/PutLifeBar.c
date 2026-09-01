#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutLifeBar(int x, int y, int n, int mx, int style);
 *     INFOVIEW.C:292, 32 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int x
 *     param $s4       int y
 *     param $s1       int n
 *     param $a3       int mx
 *     param stack+16  int style
 *     reg   $s5       int style
 *     stack sp+16     struct POLY_F4 poly
 *     reg   $a3       int w
 *     reg   $v0       int x
 *     reg   $a0       int y
 *     reg   $a3       int n
 *     reg   $s2       int ou
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE NumberImage;
 *     extern struct INFOVIEW__196fake LifeBarStyle[2];
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — pure C, all 540 bytes / 135 instructions exact.
 *
 * PutLifeBar (0x8004ab38, 0x21C bytes) — draws one n-bar "style" (style
 * 0 for the player's own bar via DoInfoViewProc, style 1 for an enemy's via
 * PutLifeBarS/ReqLifeBar): a right-to-left digit strip (identical idiom to
 * the MATCHED PutNumber.c, same TU) showing `n`, then two GsSPRITE draws —
 * a static "frame" sprite and a "fill" sprite whose height is scaled by
 * n/mx and whose color flashes red (0xE6) when low and GameClock is odd.
 *
 * The raw .s (not Ghidra's rendering, which invents wrong strides/offsets
 * for this un-named table) shows a single 0x50-byte-stride array,
 * `LifeBarStyle[2]`, NOT the 5-slot `LifeBar[]` pool (that's a different,
 * already-matched struct in ReqLifeBar.c/PutLifeBarS.c). Each entry:
 *   +0x00 u16 base   (lhu)  — height when n==0
 *   +0x02 s16 scale  (lh)   — per-unit height multiplier
 *   +0x04 s16 dx     (lh)   — digit-strip x offset from the bar position
 *   +0x06 s16 dy     (lh)   — digit-strip y offset
 *   +0x08 GsSPRITE frame    — static background/frame sprite
 *   +0x2C GsSPRITE fill     — the scaled, colored fill sprite
 * (confirmed against the sibling init function's own comment: two
 * GsSPRITE-pairs-per-style table immediately follows these same 4 header
 * shorts, `LifeBarFrame`/`LifeBarFill` are this file's `frame`/`fill`).
 *
 * The digit loop is a hand-rolled goto (not do/while): the /10 magic
 * constant re-materializes EVERY iteration in the target instead of being
 * hoisted to a preheader, same as PutNumber's proven rule (a real do-while
 * would get it hoisted by loop.c). The digit block's shadowed `n` divides
 * into `q` exactly like PutNumber's `cols`/`q` — the remainder for
 * NumberImage.u is `n - q*10`,
 * not Ghidra's `(char)` casts (the store to a `u8` field truncates either
 * way, so no cast is needed for the byte-truncating store to match).
 *
 * The fill sprite's `.h` field is transiently overwritten with the scaled
 * height for the GsSortSprite draw call, then restored to its old value
 * right after — the persistent per-frame `.h` is untouched across calls.
 *
 * The division `scale * n / mx` divides by the variable `mx` — needs
 * maspsx --expand-div (Build.hs maspsxGpExterns) for the break-6/break-7
 * guard sequence.
 *
 * Matching levers (none adds a target-absent instruction):
 *  - The identical `ou->r` arms use initialized `u` as their discriminator.
 *    The temporary extra `u` reference makes global allocation choose the
 *    target's $s4=u / $s5=mx ordering; testing `ou` left them swapped.
 *  - Reusing `color` first for `GameClock & 1`, then for 0xE6/0x80, gives
 *    that result a $v0 preference. A separate clock-test pseudo forced the
 *    otherwise exact color tail into $v1.
 *  - The digit quotient is a nested-block `q` whose scope ends before the
 *    backedge test. That scope boundary stops CSE from replacing the test of
 *    copied `t` ($a3) with the equivalent quotient source ($s0), resolving
 *    the final two-byte operand residual. The goto is safe: each entry
 *    assigns this scalar before use, and the block has no initializer/VLA.
 */

void PutLifeBar(s32 x, s32 y, s32 n, s32 mx, life_bar_style style)
{
    GsSPRITE *img;
    GsSPRITE *ou;
    s32 q;
    s16 oldh;
    s32 color;
    s32 dx;
    s32 dy;
    s32 u;

    {
        /* The digit renderer's own copies. PSX.SYM names the PARAMETERS
         * x, y and n, which is certain evidence and is used above; it
         * also lists x/y/n a second time, but that is the demo's
         * different life-bar function (it renders through a POLY_F4
         * retail does not have) and cannot be a shadow here anyway --
         * these three are initialised FROM the parameters, and the
         * frame/fill half below still needs them unmodified. So the
         * inner names are ours. */
        s32 px;
        s32 py;
        s32 count;

        count = n;
        NumberImage.w = (dx = LifeBarStyle[style].dx,
                         dy = LifeBarStyle[style].dy, 4);
        img = &NumberImage;
        u = img->u;
        px = x + dx;
        py = y + dy;
        img->x = px;
        img->y = py;

        {
            s32 q;

        loop:
            q = count / 10;
            img->u = u + (count % 10) * 4;
            GsSortSprite(img, OTablePt, 0);
            img->x -= 6;
            count = q;
        }
        if (count != 0)
            goto loop;
    }
    img->u = u;

    ou = &LifeBarStyle[style].frame;
    ou->x = x;
    ou->y = y;
    GsSortSprite(ou, OTablePt, 1);

    q = LifeBarStyle[style].scale * n / mx;
    ou = &LifeBarStyle[style].fill;
    oldh = ou->h;
    ou->h = LifeBarStyle[style].base + q;

    if (mx / 4 < n)
        color = 0x80;
    else
    {
        color = GameClock & 1;
        if (color != 0)
            color = 0xE6;
        else
            color = 0x80;
    }
    ou->b = color;
    ou->g = color;
    if (u != 0)
    {
        ou->r = color;
    }
    else
    {
        ou->r = color;
    }

    ou->x = x;
    ou->y = y;
    GsSortSprite(ou, OTablePt, 0);
    ou->h = oldh;
}
