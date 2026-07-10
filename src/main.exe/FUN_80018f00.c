#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern int AccessPower;
 *     extern struct POLY_GT4 AccessImage;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * FUN_80018f00 (0x80018f00, 0x110 bytes) — FILEIO.C's "stop the access
 * indicator" routine: disarms the vsync draw callback (`VSyncCallback(0)`,
 * the opposite of PrepareAccess's `VSyncCallback(cbAccess)`), and if the
 * meter was still armed (`AccessPower >= 0`) draws ONE final all-black
 * `AccessImage` quad to erase it from the screen before the callback stops
 * running — same DRAWENV/DISPENV swap-and-restore idiom as cbAccess.c (its
 * header comment names this file as the sibling to read for that shape:
 * `n_draw = o_draw;` is a plain DRAWENV struct assignment, `n_draw.clip =
 * o_disp.disp;` a plain RECT assignment, both matching cc1's emit_block_move
 * shape rather than Ghidra's field-by-field unrolling). Called from
 * FileRead.c only (once, straight-line, after the ReadMode dispatch); not in
 * the demo's PSX.SYM (a retail-only addition, unlike its FILEIO.C siblings
 * InitAccessInfo/cbAccess/PrepareAccess/FileRead which the demo already had).
 * No candidate name in reference/psxsym-candidates.tsv; not proposing one
 * without corroboration.
 *
 * STATUS: NON_MATCHING — 11 of 272 bytes differ (both drafts below are the
 * SAME 68-instruction length as the target — no whole-image shift).
 *
 * `&o_draw` is used 3 times (GetDrawEnv's arg, the `n_draw = o_draw;`
 * block-move's source, and the final `PutDrawEnv(&o_draw)` restore), exactly
 * cbAccess.c's already-documented open case: cc1 merges the FIRST TWO uses
 * (the ones either side of the single intervening GetDispEnv() call) into
 * ONE value kept live in a callee-saved register, where the target instead
 * recomputes `sp+40` fresh at all three sites in caller-saved regs with no
 * $s-register at all. Declaring `o_draw` FIRST among the three locals (this
 * file's current order) avoids that merge entirely and reaches the target's
 * exact 68-instruction count and register content — a NEW lever beyond
 * cbAccess's failed attempts (autorules, a do{}while(0) wrapper, declaration
 * order was tried there but not this specific "address-taken-first" ordering)
 * — but it also feeds cc1's stack-slot allocator, which assigns slots in
 * LOCAL-DECLARATION order (first declared = lowest address): with `o_draw`
 * declared first it lands at sp+16 and `o_disp` at sp+112, whereas the
 * target has `o_disp` at sp+16 and `o_draw` at sp+40 (`n_draw` is correctly
 * at sp+136 either way). Declaring `o_disp` first instead (matching the
 * target's slot order) reintroduces the register merge (69 vs 68
 * instructions, the cbAccess shape) — every declaration order tried
 * (all 6 permutations, plus nesting `o_disp`/`n_draw` inside the `if` body)
 * gave EITHER the correct length XOR the correct slots, never both.
 * `tools/permute.py` reports its base (this file) as score 0 immediately
 * (no candidate written) — its scorer does not penalize this stack-offset
 * residual, so it is USELESS here; `tools/matchdiff.py` (11 bytes) is the
 * only trustworthy signal. `tools/autorules.py` finds no improving edit.
 * Root cause is the same open "repeated frame address recomputed vs.
 * CSE-merged across one intervening call" class as cbAccess — still wants a
 * `-dg`/`-di` RTL dump session; recognize it by this exact 3-uses/1-call
 * shape rather than re-deriving from scratch.
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
    u32 tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    u8 r1, g1, b1, p1;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    u8 r2, g2, b2, p2;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad2;
    u8 r3, g3, b3, p3;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad3;
} POLY_GT4; /* 0x34 (PSYQ libgpu.h) */

extern void VSyncCallback(void (*f)(void));
extern void GetDrawEnv(DRAWENV *env);
extern void GetDispEnv(DISPENV *env);
extern void PutDrawEnv(DRAWENV *env);
extern void DrawPrim(u8 *prim);
extern s32 AccessPower;
extern POLY_GT4 AccessImage;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80018f00", FUN_80018f00);
#else
void FUN_80018f00(void)
{
    DRAWENV o_draw;
    DISPENV o_disp;
    DRAWENV n_draw;

    VSyncCallback(0);
    if (AccessPower >= 0)
    {
        AccessImage.r0 = 0;
        AccessImage.g0 = 0;
        AccessImage.b0 = 0;
        AccessImage.r1 = 0;
        AccessImage.g1 = 0;
        AccessImage.b1 = 0;
        AccessImage.r2 = 0;
        AccessImage.g2 = 0;
        AccessImage.b2 = 0;
        AccessImage.r3 = 0;
        AccessImage.g3 = 0;
        AccessImage.b3 = 0;
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
}
#endif

