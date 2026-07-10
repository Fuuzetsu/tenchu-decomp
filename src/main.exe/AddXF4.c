#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddXF4(void *ot, struct POLY_XF4 *ply);
 *     EFFECT.C:1780, 3 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       void * ot
 *     param $a1       struct POLY_XF4 * ply
 * END PSX.SYM */

/*
 * AddXF4 (0x80038de4) — identical shape to AddXG4 just above it (same
 * 0x80038dxx TU): adds two GPU primitives out of one buffer to an order
 * table, +8 then +0, via AddPrim(ot, prim). `ot` and `ply` are cached in
 * callee-saved regs across both calls (cookbook's cached-pointer rule).
 */
extern void AddPrim(u8 *ot, u8 *prim);

void AddXF4(u8 *ot, u8 *ply)
{
    AddPrim(ot, ply + 8);
    AddPrim(ot, ply);
}
