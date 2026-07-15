#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutLifeBar(int x, int y, int n, int mx, int style);
 *     INFOVIEW.C:292, 32 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 * STATUS: NON_MATCHING — 137 of 540 bytes differ (whole-image count equals
 * the window count: the function IS the right length, so this is a pure
 * register-allocation/scheduling residual, not a structural one).
 *
 * PutLifeBar (0x8004ab38, 0x21C bytes) — draws one life-bar "style" (style
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
 * shorts, `D_8008E41C`/`D_8008E440` are this file's `frame`/`fill`).
 *
 * The digit loop is a hand-rolled goto (not do/while): the /10 magic
 * constant re-materializes EVERY iteration in the target instead of being
 * hoisted to a preheader, same as PutNumber's proven rule (a real do-while
 * would get it hoisted by loop.c). `n` divides into `t`/`q` exactly like
 * PutNumber's `cols`/`q` — the remainder for NumberImage.u is `t - q*10`,
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
 * Residual (root-caused, not just "close"):
 *  - A `u`/`mx` register-priority tie: `u` (NumberImage.u, saved across the
 *    digit loop's calls) and the parameter `mx` (live to the division near
 *    the end) land in $s4/$s5 swapped from the target ($s4=u/$s5=mx in the
 *    target; this draft gets $s4=mx/$s5=u) — neither reordering the two
 *    statements nor reordering their declarations changed the outcome
 *    (global-alloc's priority formula ties them the other way here;
 *    tools/regalloc.py confirms both cross the same 3 calls). This single
 *    swap cascades through ~20 register-only diffs downstream (every later
 *    use of either value, plus the prologue save/restore slots).
 *  - A second, independent residual: the `color = 0x80; if (n <= mx/4 &&
 *    GameClock odd) color = 0xE6;` default-then-override sequence picks a
 *    different hard register ($a1 vs the target's $v0) and a differently
 *    shaped branch (`beqz`+nop vs the target's `bnez`+delay-slot li),
 *    cascading from the same upstream tie.
 *  - tools/permute.py ran 101k+ iterations (`--stop-on-zero -j4`, well past
 *    the cookbook's bounded-run budget): best score 755 (nonzero), flat —
 *    not a permuter-crackable tie in reasonable time. Parked per the
 *    Iteration protocol's attempt cap.
 */
typedef struct
{
    u16 base;       /* +0x00 */
    s16 scale;      /* +0x02 */
    s16 dx;         /* +0x04 */
    s16 dy;         /* +0x06 */
    GsSPRITE frame; /* +0x08 */
    GsSPRITE fill;  /* +0x2C */
} TLifeBarStyle; /* 0x50 */

extern TLifeBarStyle LifeBarStyle[2];
extern GsSPRITE NumberImage;
extern GsOT *OTablePt;
extern s32 GameClock;

extern void GsSortSprite(GsSPRITE *sp, GsOT *ot, s32 pri);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PutLifeBar", PutLifeBar);
#else
void PutLifeBar(s32 x, s32 y, s32 n, s32 mx, s32 style)
{
    GsSPRITE *img;
    GsSPRITE *ou;
    s32 t;
    s32 q;
    s16 oldh;
    s32 color;
    s32 dx;
    s32 dy;
    s32 u;

    t = n;
    NumberImage.w = (dx = LifeBarStyle[style].dx,
                     dy = LifeBarStyle[style].dy, 4);
    img = &NumberImage;
    u = img->u;
    img->x = x + dx;
    img->y = y + dy;

loop:
    q = t / 10;
    img->u = u + (t - q * 10) * 4;
    GsSortSprite(img, OTablePt, 0);
    img->x = img->x - 6;
    t = q;
    if (t != 0)
        goto loop;
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
    else if ((GameClock & 1) == 0)
        color = 0x80;
    else
        color = 0xE6;
    ou->b = color;
    ou->g = color;
    if (ou != 0)
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
#endif
