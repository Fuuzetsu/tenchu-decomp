#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * MATCH.
 *
 * FUN_800190f4 (0x800190f4, 0x144 bytes) — the demo/loading-screen splash:
 * loads two TIMs ("loading.tim", "load_ten.tim") straight off the CD into
 * two POLY_FT4 quads (SetupImageToPolyFT4, already matched at this address
 * under its real name — Ghidra's own decompile still calls it
 * `FUN_8004eaf0`, an unrelated stale label), snapshots the CURRENT draw/
 * display environment, builds a NEW draw environment by copying the current
 * one wholesale and then overwriting its clip rect + offset from the
 * display environment's own `disp` rect, and finally draws both TIM quads
 * through it before restoring the original draw environment.
 *
 * Matching notes:
 *  - `draw2 = draw;` (a whole DRAWENV struct assignment, 0x5c/92 bytes,
 *    align 4) is the proven `emit_block_move` shape: a 5x 16-byte-chunk
 *    loop (80 bytes) + a 12-byte (3-word) tail, exactly like DeleteConflict's
 *    0x78-byte pool-swap assignment — no hand-written loop needed.
 *  - `draw2.clip = disp.disp;` (RECT, align 2, 8 bytes) is the align-2
 *    struct-copy idiom (`lwl/lwr`+`swl/swr` word-pair copy, same family as
 *    an SVECTOR assignment) — Ghidra's own decompile renders this same copy
 *    as an opaque masked bit-op it couldn't identify as a struct field.
 *  - The two TIM path strings are bound `D_`-symbols the pre-conversion
 *    `.s` still referenced; keep them as `extern char D_XXXXXXXX[];` (never
 *    write fresh string literals here) — see config/symbols.main.exe.txt.
 */
extern char D_8001118C[];
extern char D_800111B0[];
extern void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply, short x, short y);

void FUN_800190f4(void)
{
    u_long *tim;
    GsIMAGE img;
    POLY_FT4 poly1;
    POLY_FT4 poly2;
    DISPENV disp;
    DRAWENV draw;
    DRAWENV draw2;

    tim = FileRead(D_8001118C);
    GetTIMInfo(tim, &img);
    LoadTIMAndFree(tim);
    SetupImageToPolyFT4(&img, &poly1, 0xD4, 0xDE);
    tim = FileRead(D_800111B0);
    GetTIMInfo(tim, &img);
    LoadTIMAndFree(tim);
    SetupImageToPolyFT4(&img, &poly2, 0xD4, 0xC0);
    GetDrawEnv(&draw);
    GetDispEnv(&disp);
    draw2 = draw;
    draw2.clip = disp.disp;
    draw2.ofs[0] = disp.disp.x;
    draw2.ofs[1] = disp.disp.y;
    PutDrawEnv(&draw2);
    DrawPrim((u8 *)&poly1);
    DrawPrim((u8 *)&poly2);
    DrawSync(0);
    PutDrawEnv(&draw);
}
