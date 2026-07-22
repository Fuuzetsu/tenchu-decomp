#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * FUN_80038c0c (0x80038c0c, 0xd4 bytes) — builds a semi-transparent POLY_F4
 * and its DR_TPAGE command in a recovered POLY_XF4 at the current work base,
 * advances the work base by 0xC0, colors the quad with the caller's RGB, then
 * adds the quad and draw mode to the order table.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - The `-0xA0` constant materialized before the first AddPrim call is a
 *    dead/live-across-call scratch (m2c's 3rd "argument" to the first
 *    AddPrim is that leftover register, not a real argument — cookbook's
 *    m2c-overcounts-args rule): AddPrim takes exactly 2 arguments here too.
 *  - **The `ply->ply.x0 = -0xA0` store's SOURCE POSITION is before the tpage
 *    command store, not after it (out of increasing-offset order, and out of
 *    the order Ghidra/m2c both render it in).** Every other field is in
 *    strict offset order; only this one constant's statement needs to move
 *    one slot earlier for its `li` to schedule where the target has it
 *    (found by tools/permute.py, ~1500 iterations, score 0 — a permuter
 *    candidate also inserted a dead `if (!p) {}` alongside the reorder;
 *    that part was verified NOT load-bearing and dropped per the cookbook's
 *    "bisect a multi-diff score-0 candidate" rule).
 */

void FUN_80038c0c(u8 *ot, s8 r, s8 g, s8 b)
{
    POLY_XF4 *ply;

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase((u8 *)ply + 0xC0);
    setlen(&ply->ply, 5);
    setcode(&ply->ply, 0x2A);
    setlen(&ply->tpage, 1);
    ply->ply.x0 = -0xA0;
    ply->tpage.code[0] = 0xE1000240;
    ply->ply.y0 = -0x78;
    ply->ply.y1 = -0x78;
    ply->ply.x1 = 0xA0;
    ply->ply.x2 = -0xA0;
    ply->ply.y2 = 0x78;
    ply->ply.x3 = 0xA0;
    ply->ply.y3 = 0x78;
    ply->ply.r0 = r;
    ply->ply.g0 = g;
    ply->ply.b0 = b;
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}
