#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_FT4 TelopP;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * FUN_800570b8 (0x800570b8, 0x160 bytes) — the telop (on-screen caption)
 * text-line renderer, called by DrawTelop (matched, same TU) with
 * `org = OTablePt->org`, `x = -(width/2)`, `y = 0x5c`, `str` the caption
 * text. If a telop is already "active" (TelopP.u0 or TelopP.u1 nonzero —
 * FUN_800576e8's same proven TelopType view, this TU's shorthand for
 * "already queued/fading"), instead draws TelopP itself as a textured quad
 * via GsSortPoly (the real PSYQ POLY_FT4, proven in full by
 * SetupImageToPolyFT4.c — x0/x2 = x, y0/y1 = y, x1/x3 = x + (u1-u0)
 * [i.e. the queued caption's own pixel width], y2/y3 = y+15). Otherwise
 * walks `str`, drawing one glyph per byte via FUN_8005778c (still asm;
 * takes the same org/x/y/char signature) at a cursor that resets to the
 * start-of-line X on '\n' and otherwise advances by each glyph's width out
 * of the per-glyph table D_8008EF98[] (the SAME SJIS-code-to-glyph-index
 * remap as FUN_800576e8: 0x92->0x27, -0x20 if >=0x20, an extra -0x40 for
 * the upper half-width-kana block >=0xC0). No confirmed original name.
 *
 * STATUS: NON_MATCHING — 138 of 352 bytes differ, SAME length as target (88
 * insns both sides). Two real structural/dispatch fixes are folded in
 * (verified via matchdiff, each reduced the diff): (1) the `TelopP.u0/u1`
 * dispatch is a "two independent if (cond) goto L;" ladder, not Ghidra's
 * `if (u0==0 && (cursor=x, u1==0)) {chars} else {quaddraw}` nesting — the
 * quad-draw body is the FALLTHROUGH of both tests (`if(u0!=0) goto
 * quaddraw; if(u1==0) goto charloop;` then quaddraw inline), matching the
 * asm's branch targets exactly; and (2) `cursor=x;` happens unconditionally
 * in the prologue (before the `*str==0` check), not inside it. Left:
 * a cluster of pure register/schedule ties — the quad-draw field stores
 * use TARGET's `s1`(cursor)/`s2`(y) where ours computes `s2`(cursor)/
 * `s1`(y) (a role swap through the whole block); the `a2=0` (GsSortPoly's
 * literal 3rd arg) sits in the branch's own delay slot in target vs
 * hoisted earlier in ours; and the 0x92->0x27 remap's `hi=ch>0xbf` test
 * reads `a0` (a register that, in target, happens to still hold the
 * pre-remap byte on the common path) where ours reads the explicit `hi`
 * temp — tried inlining `if (ch>0xbf)` post-decrement (a decomp-permuter
 * candidate, score 790) but verified via matchdiff it's a real regression
 * (205 bytes, wrong length) despite scoring better on the permuter's own
 * metric — REJECTED, not adopted (the cookbook's "own score, not raw
 * bytes" warning, caught in the act). A same-length decomp-permuter run
 * (~33k iterations, `--stop-on-zero -j4`) bottomed out around score 670
 * without reaching 0; the one verified real fix it surfaced (swapping the
 * newline branch's statement order, `cursor=x;` before `y+=0x10;`) is
 * already folded in above. Parked rather than opening a further surgical
 * session.
 */

typedef struct
{
    u_long tag;
    u_char r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad1;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad2;
} TelopPolyType;

struct GsOT_TAG;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_800570b8", FUN_800570b8);
#else

extern TelopPolyType TelopP;
extern GsOT *OTablePt;
extern u8 D_8008EF98[];
extern void FUN_8005778c(struct GsOT_TAG *org, s32 x, s32 y, u32 ch);
extern void GsSortPoly(TelopPolyType *p, GsOT *ot, s32 pri);

void FUN_800570b8(struct GsOT_TAG *org, s32 x, s32 y, u8 *str)
{
    s32 cursor;
    s32 ch;
    bool hi;

    cursor = x;
    if (*str != 0) {
        if (TelopP.u0 != 0) {
            goto quaddraw;
        }
        if (TelopP.u1 == 0) {
            goto charloop;
        }
    quaddraw:
        TelopP.y0 = (s16)y;
        TelopP.y2 = TelopP.y0 + 0xf;
        TelopP.x0 = (s16)x;
        TelopP.x1 = TelopP.x0 + (TelopP.u1 - TelopP.u0);
        TelopP.y1 = TelopP.y0;
        TelopP.x2 = TelopP.x0;
        TelopP.x3 = TelopP.x1;
        TelopP.y3 = TelopP.y2;
        GsSortPoly(&TelopP, OTablePt, 0);
        goto end;
    charloop:
        do {
            if (*str == 10) {
                cursor = x;
                y = y + 0x10;
            } else {
                FUN_8005778c(org, cursor, y, *str);
                ch = *str;
                if (ch == 0x92) {
                    ch = 0x27;
                }
                hi = ch > 0xbf;
                if (ch > 0x1f) {
                    ch = ch - 0x20;
                }
                if (hi) {
                    ch = ch - 0x40;
                }
                cursor = cursor + D_8008EF98[ch];
            }
            str++;
        } while (*str != 0);
    end:;
    }
}
#endif /* NON_MATCHING */
