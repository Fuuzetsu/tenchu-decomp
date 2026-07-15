#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void cbAccess(void);
 *     FILEIO.C:115, 27 src lines, frame 240 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 * MATCH. The final CSE lever is the identical `GetDispEnv(&o_disp)` call in
 * both arms of the `AccessPower` test. Before jump cleanup, its control-flow
 * boundary separates the first `&o_draw` use (`GetDrawEnv`) from the source
 * pointer emitted for `n_draw = o_draw`; after allocation and scheduling,
 * jump cleanup erases the test and merges the calls, so the final binary has
 * no branch or duplicate call. This makes cc1 rematerialize `sp+40` in the
 * target's caller-saved registers ($a0, then $v0, then $a0) instead of CSEing
 * it across GetDispEnv into $s0, which had added a save/restore instruction.
 * The fence must sit on the SECOND call: placing it at function entry pins
 * the return-address save ahead of the target's body setup, while fencing the
 * first call delays its argument and changes the color-register allocation.
 *
 * The target also writes `AccessImage.g0` before computing/storing `b0`.
 * Keeping that source order resolves the remaining independent-store
 * scheduling tie. A one-shot loop and declaration-order changes did not
 * affect the address merge; guided autorules likewise found no improvement.
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

void cbAccess(void)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    u32 v;

    v = (AccessPower + 8) & 0xff;
    AccessPower = v;
    AccessImage.r0 = v;
    AccessImage.g0 = AccessImage.r0;
    AccessImage.b0 = 0xff - AccessImage.r0;
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
    if (AccessPower != 0)
        GetDispEnv(&o_disp);
    else
        GetDispEnv(&o_disp);
    n_draw = o_draw;
    n_draw.clip = o_disp.disp;
    n_draw.ofs[0] = o_disp.disp.x;
    n_draw.ofs[1] = o_disp.disp.y;
    PutDrawEnv(&n_draw);
    DrawPrim((u8 *)&AccessImage);
    PutDrawEnv(&o_draw);
}
