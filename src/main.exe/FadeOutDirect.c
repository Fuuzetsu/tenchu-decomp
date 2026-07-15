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
 * STATUS: NON_MATCHING — 12 differing asm lines, function 103 insns vs the
 * target's 104 (default `./Build` keeps the byte-identical INCLUDE_ASM stub;
 * build the draft with `NON_MATCHING=FadeOutDirect ./Build`).
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
 *  - The `n_draw`/`PutDrawEnv(&n_draw)` group NEEDS a `do{}while(0)`
 *    wrapper (a byte-neutral scheduling lever, see cookbook "Register
 *    allocation steering"): without it, cc1 rematerializes `&n_draw`
 *    into `$a0` immediately after the block-copy loop (a0 is idle for the
 *    whole `ply` setup that follows) and holds it there unused for ~20
 *    instructions instead of computing it just-in-time right before the
 *    call, the way the target and DrawPause.c (same source shape, already
 *    matched) both do. The wrapper's loop-depth weighting is what keeps
 *    the address's materialization pinned next to its use. Confirmed by
 *    direct trial: identical source WITHOUT the wrapper reproducibly
 *    inserts the extra `addiu a0,sp,136` early.
 *  - The quad's four corners: `x0=y0=y1=x2=0`, `x1=x3=o_disp.disp.w`,
 *    `y2=y3=o_disp.disp.h` (a full-screen rect using the CURRENT display
 *    width/height, read fresh from `o_disp` a second time here, not
 *    reused from the `n_draw.clip` copy above).
 *  - The `time`-counted draw loop is `while (1) { if (time == 0) break;
 *    ...; time = time - 1; }`, not a plain `for`/`while (time != 0)`: the
 *    target has a TOP test + unconditional back-jump with the decrement
 *    in the back-jump's delay slot (cookbook Loops: this shape keeps
 *    loop.c's hoisting without jump.c rotating it into a guarded
 *    bottom-tested do-while, which a plain `for(;time!=0;time--)` would
 *    get instead). `time` is the reused PARAMETER (prologue `move
 *    $s1,$a0`), not a fresh local.
 *
 * RESIDUAL (12 lines / 1 instruction, everything else byte-identical):
 *  - The `ply.ply.code` byte is set in TWO steps in the target, not one:
 *    `= 0x28;` first, then (after the unrelated tpage-mode-word
 *    computation) re-read and OR'd with `2` and stored back — a genuine
 *    `lbu`/`ori 2`/`sb` at that stack slot, not a single `sb` of the final
 *    0x2A ('*') the way SetPolyXF4's own `ply->ply.code = '*';` compiles.
 *    Ghidra's decompilation collapses this to one `local_31 = 0x2a;`
 *    literal, hiding the two-store shape entirely — the raw `.s` is the
 *    only evidence it exists. Writing the identical two statements here
 *    (`= 0x28;` then `= ply.ply.code | 2;`, with the intervening
 *    `*(u_char*)&ply.tpage.tag+3 = 1;` store in between, in every
 *    statement order tried — adjacent, split, before/after the
 *    tpage.code[0] computation) lets THIS cc1 prove the intermediate
 *    store is dead (no aliasing ambiguity — same base register, disjoint
 *    constant offsets) and fold straight to `li v0,42`, one instruction
 *    SHORTER than the target. Marking the field `volatile` at the access
 *    (`*(volatile u_char*)&ply.ply.code`) DOES force the genuine
 *    lbu/ori/sb sequence and recovers the missing instruction (104 vs
 *    104) — but it also introduces a NEW divergence: with volatile
 *    present, the scheduler spreads the reload and the store-back across
 *    the surrounding `ply.tpage.code[0]` computation (arriving far apart)
 *    instead of adjacent the way the target keeps them, netting the SAME
 *    12-line residual by a different route. `volatile` is also not a
 *    plausible reading of 1998 PSX game source for a plain stack local,
 *    so it isn't used here. Tried: every relative ordering of the two
 *    `code` touches and the `tpage.tag`/`tpage.code[0]` statements
 *    (adjacent, interleaved, before, after); `do{}while(0)` around the
 *    OR alone and around the whole tag/code group (autorules found no
 *    improving edit; a bounded decomp-permuter run, ~230 iterations
 *    across the search, best internal score unrelated to this residual —
 *    every candidate it tried still has the fold). This looks like a
 *    genuine gcc-2.8.1 cse.c/local dead-store-elimination difference
 *    triggered by something about the ORIGINAL statement shape that none
 *    of the tried respellings reproduce — a decomp.me (psyq4.3) session
 *    would be the next lever, not a further permuter run.
 *  - A SEPARATE one-instruction scheduling tie rides along: the target's
 *    `ply.ply.tag`+3=5 store's `li v0,5` sits right after `lui
 *    a1,0xE100`/`addiu v1,sp,0xE8`; this draft's compiles one instruction
 *    earlier (before those two), a tie not resolved by the wrapper that
 *    fixed the `&n_draw` hoist.
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

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FadeOutDirect", FadeOutDirect);
#else /* NON_MATCHING */

void FadeOutDirect(short time, short attrib, u8 r, u8 g, u8 b)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    POLY_XF4 ply;

    GetDrawEnv(&o_draw);
    GetDispEnv(&o_disp);
    do
    {
        n_draw = o_draw;
        n_draw.clip = o_disp.disp;
        n_draw.ofs[0] = o_disp.disp.x;
        n_draw.ofs[1] = o_disp.disp.y;
        PutDrawEnv(&n_draw);
    } while (0);
    *((u_char *)&ply.ply.tag + 3) = 5;
    ply.ply.code = 0x28;
    *((u_char *)&ply.tpage.tag + 3) = 1;
    ply.ply.code = ply.ply.code | 2;
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
    while (1)
    {
        if (time == 0)
        {
            break;
        }
        DrawPrim((u8 *)&ply.ply);
        DrawPrim((u8 *)&ply.tpage);
        DrawSync(0);
        VSync(0);
        time = time - 1;
    }
    PutDrawEnv(&o_draw);
}

#endif /* NON_MATCHING */
