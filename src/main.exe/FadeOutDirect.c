#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void FadeOutDirect(short time, short attrib, unsigned char r, unsigned char g, int b);
 *     EFFECT.C:1816, 32 src lines, frame 296 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $a0       short time
 *     param $a1       short attrib
 *     param $a2       unsigned char r
 *     param $a3       unsigned char g
 *     param stack+16  int b
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *     stack sp+232    struct POLY_XF4 ply
 * END PSX.SYM */

/*
 * STATUS: MATCH (exact).
 *
 * Matching notes (all verified against the original bytes):
 *  - PSX.SYM types the 5th param `int b`, but the raw asm loads it with a
 *    plain `lbu` (single unsigned byte) at entry and only ever stores it
 *    as a byte (`ply.ply.b0 = b`) — declared `unsigned char` here instead,
 *    matching r/g (cookbook: "the asm wins" over a stale/imprecise width).
 *  - `n_draw = o_draw;` is a PLAIN WHOLE-STRUCT ASSIGNMENT (DRAWENV is
 *    0x5c bytes; cc1's emit_block_move compiles it to a 5-iteration
 *    4-word loop over the aligned 0x50-byte bulk + a 3-word tail) — NOT
 *    Ghidra's hand-unrolled walking-pointer rendering (DrawPause.c, same
 *    mechanism, already matched, is the proof).
 *  - `n_draw.clip = o_disp.disp;` is ALSO a plain struct-to-struct copy
 *    (RECT is align-2, so cc1 emits the lwl/lwr+swl/swr block-move idiom
 *    for it) rather than four separate scalar field assignments.
 *  - RECT/DRAWENV/DISPENV are the proven layouts from AdtReleaseDisp.c/
 *    DrawPause.c (same original TU family); DR_TPAGE/POLY_F4/POLY_XF4 are
 *    SetPolyXF4.c's proven layout (same EFFECT.C TU) — reused verbatim.
 *  - The `n_draw`/`PutDrawEnv(&n_draw)` group needs NO `do{}while(0)`
 *    wrapper. An earlier checkpoint carried one and asserted it was
 *    load-bearing ("without it cc1 hoists `addiu a0,sp,136` early"); that
 *    was true only of THAT draft, where the `while(1)+break` loop below
 *    made loop.c hoist `&ply` and inflated register pressure. With the
 *    goto loop and `pp` in place the fence is not merely unnecessary but
 *    HARMFUL: it is worth the last 8 bytes (autorules' `fence-unwrap`
 *    found this). Fences are reconstruction scaffolding, not idiom.
 *  - The quad's four corners: `x0=y0=y1=x2=0`, `x1=x3=o_disp.disp.w`,
 *    `y2=y3=o_disp.disp.h` (a full-screen rect using the CURRENT display
 *    width/height, read fresh from `o_disp` a second time here, not
 *    reused from the `n_draw.clip` copy above).
 *  - The `time`-counted draw loop is a HAND-ROLLED `goto` loop, not
 *    `while (1) { if (time == 0) break; … }`. Both spellings give the same
 *    top test + unconditional back-jump with the decrement in the
 *    back-jump's delay slot, so the control flow alone does not choose
 *    between them — but `while(1)+break` still emits loop notes, so loop.c
 *    hoists the loop-invariant `&ply` into the preheader and pins it in a
 *    callee-saved register (`addiu s0,sp,232` + `move a0,s0`). The target
 *    RECOMPUTES that invariant inside the loop (`addiu a0,sp,232` at
 *    0x80038bc4), which is the cookbook's tell for the goto form ("A
 *    top-test loop that never hoists its invariants is a hand-rolled goto
 *    loop"). `time` is the reused PARAMETER (prologue `move $s1,$a0`).
 *  - `pp` (the `&ply` base register, `$v1`) is what makes the `ply.ply.code`
 *    byte take TWO steps in the target — `= 0x28` through the pointer
 *    (`sb v0,0xF(v1)`), then a genuine `lbu`/`ori 2`/`sb` back at the SAME
 *    field spelled sp-relative (`0xF7(sp)`). The two accesses use different
 *    ADDRESS RTX, and gcc-2.8.1's cse hashes memory by address, so it cannot
 *    forward the stored 0x28 to the reload. Spelling both accesses directly
 *    on the local (`ply.ply.code = 0x28; ply.ply.code |= 2;`) makes both
 *    `0xF7(sp)`, cse forwards, `fold` collapses `0x28|2`, and you get a
 *    single `li v0,0x2A` — one instruction SHORT, which is exactly how the
 *    earlier checkpoint stalled. Note this is cse VALUE FORWARDING, not
 *    dead-store elimination: that draft still emitted the `= 0x28` store.
 *    The pointer offsets decode the target exactly: `&pp->ply.tag+3` -> 11,
 *    `pp->ply.code` -> 15, `&pp->tpage.tag+3` -> 3, all off `$v1 = &ply`.
 *  - `pp` does not appear in the demo's PSX.SYM local list, which records
 *    register locals for this function (all five params are listed). So the
 *    ORIGINAL almost certainly reached these three bytes through PsyQ's
 *    pointer-taking packet macros (`setlen(&p->ply,5)` / `setcode(&p->ply,
 *    0x28)` / `setlen(&p->tpage,1)`, libgpu.h) rather than a named local;
 *    `pp` is the reconstruction that reproduces their addressing. That the
 *    demo build (a separate compile of the same source, 0x80031fbc) emits
 *    this ply-setup block instruction-for-instruction identically to retail
 *    is what proves the shape is source, not a scheduling accident.
 *  - Sibling SetPolyXF4 (same TU, EFFECT.C:1770) writes these same fields
 *    but takes `POLY_XF4 *ply` as a PARAMETER, so there the direct and the
 *    through-pointer spellings are indistinguishable and it matched with
 *    `ply->ply.code = '*';`. Its source is therefore NOT evidence for how
 *    to spell the accesses here, where `ply` is a local aggregate.
 */
typedef struct
{
    s16 x, y, w, h;
} RECT; /* 0x8 (PSYQ libgpu.h) */

typedef struct
{
    RECT clip;                     /* +0x00 */
    s16 ofs[2];                    /* +0x08 */
    RECT tw;                       /* +0x0c */
    u16 tpage;                     /* +0x14 */
    u8 dtd, dfe, isbg, r0, g0, b0; /* +0x16..+0x1b */
    u32 dr_env[16];                /* +0x1c: tag + code[15] */
} DRAWENV;                         /* 0x5c (PSYQ libgpu.h) */

typedef struct
{
    RECT disp;                       /* +0x00 */
    RECT screen;                     /* +0x08 */
    u8 isinter, isrgb24, pad0, pad1; /* +0x10..+0x13 */
} DISPENV;                           /* 0x14 (PSYQ libgpu.h) */

typedef struct
{
    u_long tag;
    u_long code[1];
} DR_TPAGE;

typedef struct
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    short x1, y1;
    short x2, y2;
    short x3, y3;
} POLY_F4;

typedef struct
{
    DR_TPAGE tpage;
    POLY_F4 ply;
} POLY_XF4;

extern void GetDrawEnv(DRAWENV *env);
extern void GetDispEnv(DISPENV *env);
extern void PutDrawEnv(DRAWENV *env);
extern void DrawPrim(u8 *prim);
extern int DrawSync(int mode);
extern int VSync(int mode);

void FadeOutDirect(short time, short attrib, u8 r, u8 g, u8 b)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    POLY_XF4 ply;
    POLY_XF4 *pp;

    GetDrawEnv(&o_draw);
    GetDispEnv(&o_disp);
    n_draw = o_draw;
    n_draw.clip = o_disp.disp;
    n_draw.ofs[0] = o_disp.disp.x;
    n_draw.ofs[1] = o_disp.disp.y;
    PutDrawEnv(&n_draw);
    pp = &ply;
    *((u_char *)&pp->ply.tag + 3) = 5;
    pp->ply.code = 0x28;
    ply.ply.code = ply.ply.code | 2;
    *((u_char *)&pp->tpage.tag + 3) = 1;
    ply.tpage.code[0] = ((attrib & 3) << 5) | 0xE1000200;
    ply.ply.x0 = 0;
    ply.ply.y0 = 0;
    ply.ply.y1 = 0;
    ply.ply.x2 = 0;
    ply.ply.r0 = r;
    ply.ply.g0 = g;
    ply.ply.b0 = b;
    ply.ply.x1 = o_disp.disp.w;
    ply.ply.y2 = o_disp.disp.h;
    ply.ply.x3 = o_disp.disp.w;
    ply.ply.y3 = o_disp.disp.h;
loop:
    if (time == 0)
    {
        goto end;
    }
    DrawPrim((u8 *)&ply.ply);
    DrawPrim((u8 *)&ply.tpage);
    DrawSync(0);
    VSync(0);
    time = time - 1;
    goto loop;
end:
    PutDrawEnv(&o_draw);
}
