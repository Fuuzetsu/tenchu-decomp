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
 * FUN_80056910 (0x80056910, 0x12c bytes) — tiles ONE shared GsSPRITE
 * (libgs.h's proven struct, embedded at +0x68 of some larger, otherwise
 * unidentified struct — only its `sp` sub-object is ever touched here) over
 * a fixed on-screen rectangle: OR two "orientation" bits into `sp.attribute`
 * depending on the sign of `dir`'s low 16 bits, sets `sp`'s grayscale color
 * (r=g=b=abs(dir) as a byte — a brightness/shade value), then walks a grid
 * from x=-160 stepping by sp.w until x>160 and y=-120 stepping by sp.h
 * until y>sp.h+120, calling GsSortSprite once per cell (priority 1). Only
 * caller is still-asm FUN_80055d64 — no confirmed original name; looks
 * like a full-screen tiled-pattern draw (e.g. a radar/sonar background).
 *
 * STATUS: NON_MATCHING — 8 of 300 bytes differ (2 extra instructions: our
 * build saves/restores one MORE callee-saved register, s0..s5 vs the
 * target's s0..s4). Root cause pinned down but not reproduced from C:
 * `width` (0xa0, used BOTH negated for the per-row `sp->x` reset and
 * un-negated as the inner while's own bound) needs to live in exactly ONE
 * callee-saved register across the whole function, including across the
 * GsSortSprite call inside the inner loop — that's what the target's `s2`
 * does. No respelling tried gets cc1 to do this: a single `s16 width`
 * reused both negated and bare (as above) gets copied into a SECOND
 * register (`move $s1,$s5`) right after the outer guard, so the inner
 * loop's bound test reads the copy, not the original — confirmed
 * regardless of: declaration position (top of function vs directly before
 * first use vs inside the outer `if`), using a same-valued but
 * independently-named second local for the bound-only use (`xbound`, which
 * just gets CSE'd back to one value with the identical duplicate), and an
 * explicit separately-named `s16 negwidth = -width;` for the negated use
 * (made it WORSE, 5 extra instead of 2 — negwidth itself then needed its
 * own sign-extension pair). `height` (0x78), used the same dual way for the
 * OUTER loop's own bound/reset, does NOT show this duplication — only
 * `width` does, and only because its bound use is reached through the
 * INNER loop's call. tools/regalloc.py confirms 6 values live-across-a-call
 * (s0..s5) vs target's 5; tools/autorules.py's only "win" (h: u16->u8)
 * is the cookbook's known false-positive class (narrows the PROVEN u16
 * GsSPRITE.h field to a byte load) — rejected, not applied. Same shape as
 * FUN_800270f8's parked residual (a redundant-register-copy tie that no
 * source respelling moves) but on a value that must survive a NESTED
 * loop's OWN call, not a straight-line branch. Not permuted (residual is
 * 2 instructions short of target length, outside the "same length"
 * permuter recipe); parked rather than opening a surgical session.
 */

typedef struct
{
    u8 pad0[0x68];
    GsSPRITE sp; /* +0x68 */
} SpriteGridType;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80056910", FUN_80056910);
#else

extern GsOT *OTablePt;
extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, int pri);

void FUN_80056910(SpriteGridType *g, u16 dir)
{
    GsSPRITE *sp;
    u32 flags;
    s16 shade;
    u16 h;
    s16 width;
    s16 height;

    sp = &g->sp;
    flags = sp->attribute & 0x8fffffff;
    sp->attribute = flags;
    flags = flags | (0 < (s32)((u32)dir << 16) ? 0x60000000 : 0x50000000);
    sp->attribute = flags;
    shade = (s16)dir;
    if (shade < 0) {
        shade = -shade;
    }
    sp->r = (u8)shade;
    sp->g = (u8)shade;
    sp->b = (u8)shade;
    height = 0x78;
    sp->y = -height;
    h = sp->h;
    if (-height <= (s16)(h + height)) {
        width = 0xa0;
        do {
            sp->x = -width;
            while (sp->x <= width) {
                GsSortSprite(sp, OTablePt, 1);
                sp->x = sp->x + sp->w;
            }
            h = sp->h;
            sp->y = sp->y + h;
        } while (sp->y <= (s16)(h + height));
    }
}
#endif /* NON_MATCHING */
