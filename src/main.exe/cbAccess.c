#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void cbAccess(void);
 *     FILEIO.C:115, 27 src lines, frame 240 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_GT4 AccessImage;
 *     extern int AccessPower;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * cbAccess (0x80018dec, 0x114 bytes) — the access-meter's vsync-callback
 * draw routine (armed by PrepareAccess/FileRead): advances AccessPower by 8
 * (wrapping mod 256) and re-tints AccessImage's 4 vertices to a
 * lighten-towards-white gradient from that value, then swaps in a draw
 * environment with the display's own clip rect (so the meter draws
 * unclipped over the current frame), draws it, and restores the original
 * draw environment. Same DRAWENV/DISPENV/RECT layout as the still-unmatched
 * sibling FUN_80018f00.c (identical o_disp/o_draw/n_draw copy shape — read
 * that one too if this one's copy loop needs re-deriving) and the matched
 * AdtReleaseDisp.c (source of these local typedefs).
 *
 * Matching notes:
 *  - `n_draw = o_draw;` is a plain DRAWENV (align-4, 0x5c bytes) struct
 *    assignment — cc1's emit_block_move splits it into a 5x 16-byte-chunk
 *    loop (4 words each) plus a 12-byte (3-word) tail, matching the raw .s
 *    word-for-word (cookbook's "cast type's alignment drives copy code").
 *  - `n_draw.clip = o_disp.disp;` is then written SEPARATELY on top: RECT is
 *    align-2 (four shorts), so this one compiles to lwl/lwr+swl/swr pairs —
 *    NOT part of the word-copy above, even though clip sits at n_draw+0 (this
 *    was the first real bug: writing it as 4 separate scalar field
 *    assignments compiles each as its own scalar lhu/sh instead of the
 *    aligned 4-byte lwl/lwr combine — matched only once respelled as ONE
 *    aggregate RECT assignment).
 *  - The color computation must funnel through ONE variable reused for both
 *    the `AccessPower` store and every AccessImage.rN/gN store the asm
 *    colours through $v0: `v = (AccessPower + 8) & 0xff;` (matches the
 *    `andi $v0,$v0,0xff` before ANY store — this is not a `u_char` truncation
 *    at the store, the mask happens at the assignment). This part is
 *    byte-identical to the target already.
 *
 * STATUS: NON_MATCHING — 4 of 276 bytes differ (69 vs our 70 instructions).
 * `&o_draw` is used three times: GetDrawEnv(&o_draw) [call arg], the
 * `n_draw = o_draw;` block-move's source-pointer preheader setup, and the
 * final PutDrawEnv(&o_draw). All three are the identical frame-relative
 * address `(plus $sp 40)` with no intervening CODE_LABEL between the FIRST
 * two (GetDrawEnv's own call does not end a basic block, and emit_block_move's
 * preheader setup for the struct-copy loop sits BEFORE that loop's own
 * back-edge label) — so cc1's cse merges the block-move's source-pointer
 * pseudo with GetDrawEnv's call-argument pseudo, extending its live range
 * across the intervening GetDispEnv() call and forcing it into a
 * callee-saved register ($s0: an extra prologue/epilogue save pair, +1
 * instruction / +4 bytes). The target does NOT perform this merge — it
 * recomputes `sp+40` fresh at all three sites, entirely in caller-saved
 * registers ($a0 then $v0 then $a0 again), with no $s-register at all
 * (despite the DEMO PSX.SYM header recording saved-reg mask 0x80010000,
 * i.e. one $s-register used in the demo build — retail's actual asm here
 * uses zero, confirming the "demo allocation may differ from retail"
 * caveat is not just theoretical for this function).
 *
 * Ruled out (regalloc.py + autorules.py + manual bisection):
 *  - autorules.py: no local-type or chain-collapse edit improves the diff.
 *  - A `do { ... } while (0);` wrapper around the copy+PutDrawEnv(&n_draw)
 *    block does not break the merge (still colours to $s0) and additionally
 *    perturbs an unrelated delay-slot fill 8 bytes later — net worse (34 vs
 *    28 differing lines).
 *  - Local declaration order (`v` before vs after the DRAWENV/DISPENV
 *    locals) has no effect on the tie.
 *  - This is NOT the cookbook's named "la/address-materialization reload
 *    tie" (that's a same-length single/dual-register color swap for ONE
 *    address use; this is a genuine EXTRA instruction from a THIRD use
 *    being folded into the first two) — it is closer to the "Overlapping
 *    big frame buffers" cse-merge mechanism (DoInfoViewProc), but that
 *    section's own "only a CODE_LABEL breaks the window" rule does not
 *    resolve it here since the merging pair (call-arg pseudo, block-move
 *    preheader pseudo) both sit BEFORE any label — recognize this as a
 *    DISTINCT open case: a repeated-frame-address merge across an
 *    intervening call with NO available label to break it. Since the
 *    residual is a wrong INSTRUCTION COUNT (not same-length), it is not
 *    eligible for tools/permute.py per the cookbook's iteration protocol
 *    (permuter only cracks same-length ties); no permuter run was made.
 */
typedef struct
{
    s16 x, y, w, h;
} RECT; /* 0x8 (PSYQ libgpu.h) */

typedef struct
{
    RECT clip;       /* +0x00 */
    s16 ofs[2];       /* +0x08 */
    RECT tw;         /* +0x0c */
    u16 tpage;        /* +0x14 */
    u8 dtd, dfe, isbg, r0, g0, b0; /* +0x16..+0x1b */
    u32 dr_env[16];   /* +0x1c: tag + code[15] */
} DRAWENV; /* 0x5c (PSYQ libgpu.h) */

typedef struct
{
    RECT disp;   /* +0x00 */
    RECT screen; /* +0x08 */
    u8 isinter, isrgb24, pad0, pad1; /* +0x10..+0x13 */
} DISPENV; /* 0x14 (PSYQ libgpu.h) */

typedef struct
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
    u_char r1, g1, b1, p1;
    short x1, y1;
    u_char u1, v1;
    u_short tpage;
    u_char r2, g2, b2, p2;
    short x2, y2;
    u_char u2, v2;
    u_short pad2;
    u_char r3, g3, b3, p3;
    short x3, y3;
    u_char u3, v3;
    u_short pad3;
} POLY_GT4;

extern void GetDrawEnv(DRAWENV *env);
extern void GetDispEnv(DISPENV *env);
extern void PutDrawEnv(DRAWENV *env);
extern void DrawPrim(u8 *prim);
extern s32 AccessPower;
extern POLY_GT4 AccessImage;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/cbAccess", cbAccess);
#else
void cbAccess(void)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    u32 v;

    v = (AccessPower + 8) & 0xff;
    AccessPower = v;
    AccessImage.r0 = v;
    AccessImage.b0 = 0xff - AccessImage.r0;
    AccessImage.g0 = AccessImage.r0;
    AccessImage.r1 = AccessImage.r0;
    AccessImage.g1 = AccessImage.b0;
    AccessImage.b1 = AccessImage.r0;
    AccessImage.r2 = AccessImage.b0;
    AccessImage.g2 = AccessImage.r0;
    AccessImage.b2 = AccessImage.r0;
    AccessImage.r3 = AccessImage.r0;
    AccessImage.g3 = AccessImage.r0;
    AccessImage.b3 = AccessImage.r0;
    GetDrawEnv(&o_draw);
    GetDispEnv(&o_disp);
    n_draw = o_draw;
    n_draw.clip = o_disp.disp;
    n_draw.ofs[0] = o_disp.disp.x;
    n_draw.ofs[1] = o_disp.disp.y;
    PutDrawEnv(&n_draw);
    DrawPrim((u8 *)&AccessImage);
    PutDrawEnv(&o_draw);
}
#endif
