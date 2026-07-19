#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * FUN_80038c0c (0x80038c0c, 0xd4 bytes) — builds two GPU primitives (likely a
 * semi-transparent POLY_F4 + its DR_TPAGE mode-setting header, per the
 * PsyQ-macro hints in the original decompilation) in the current GsGetWorkBase
 * slot, advances the work base by 0xC0, fills in the three caller-supplied
 * RGB-ish bytes at +0xC/+0xD/+0xE, then adds both primitives (+8 first, then
 * +0) to the order table via AddPrim(ot, prim) — same call as AddXF4.c/
 * AddXG4.c/DrawXG4.c/DrawXF4.c just below/above it in this TU. No
 * proven struct name exists for the primitive layout (same "no proven view"
 * shape as SetPolyXG4.c's sibling), so plain offset casts off the
 * GsGetWorkBase pointer, matching Ghidra/m2c literally.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - m2c's grouped s16 stores (unk12/unk16/unk10/.../unk1E) are the real
 *    shape; Ghidra's byte-pair rendering (0x88/0xff etc.) is just its
 *    decompiler not recognizing the paired bytes as one `sh`.
 *  - The `-0xA0` constant materialized before the first AddPrim call is a
 *    dead/live-across-call scratch (m2c's 3rd "argument" to the first
 *    AddPrim is that leftover register, not a real argument — cookbook's
 *    m2c-overcounts-args rule): AddPrim takes exactly 2 arguments here too.
 *  - **The `p+0x10 = -0xA0` store's SOURCE POSITION is before the `p+4`
 *    word store, not after it (out of increasing-offset order, and out of
 *    the order Ghidra/m2c both render it in).** Every other field is in
 *    strict offset order; only this one constant's statement needs to move
 *    one slot earlier for its `li` to schedule where the target has it
 *    (found by tools/permute.py, ~1500 iterations, score 0 — a permuter
 *    candidate also inserted a dead `if (!p) {}` alongside the reorder;
 *    that part was verified NOT load-bearing and dropped per the cookbook's
 *    "bisect a multi-diff score-0 candidate" rule).
 */

void FUN_80038c0c(u8 *arg0, s8 arg1, s8 arg2, s8 arg3)
{
    u8 *p;

    p = (u8 *)GsGetWorkBase();
    GsSetWorkBase(p + 0xC0);
    p[0xB] = 5;
    p[0xF] = 0x2A;
    p[3] = 1;
    *(s16 *)(p + 0x10) = -0xA0;
    *(u32 *)(p + 4) = 0xE1000240;
    *(s16 *)(p + 0x12) = -0x78;
    *(s16 *)(p + 0x16) = -0x78;
    *(s16 *)(p + 0x14) = 0xA0;
    *(s16 *)(p + 0x18) = -0xA0;
    *(s16 *)(p + 0x1A) = 0x78;
    *(s16 *)(p + 0x1C) = 0xA0;
    *(s16 *)(p + 0x1E) = 0x78;
    p[0xC] = arg1;
    p[0xD] = arg2;
    p[0xE] = arg3;
    AddPrim(arg0, p + 8);
    AddPrim(arg0, p);
}
