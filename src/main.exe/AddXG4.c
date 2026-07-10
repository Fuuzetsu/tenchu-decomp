#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddXG4(void *ot, struct POLY_XG4 *ply);
 *     EFFECT.C:1803, 3 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       void * ot
 *     param $a1       struct POLY_XG4 * ply
 * END PSX.SYM */

/*
 * AddXG4 (0x80038d40) — adds two GPU primitives out of one buffer to an order
 * table: the primitive at +8 first, then the one at +0 (same shape as
 * FUN_80038d10/FUN_80038db4's DrawPrim wrappers just above in this TU, but
 * through AddPrim(ot, prim) instead of a direct DrawPrim(prim)). Both `ot`
 * and `ply` are cached in callee-saved regs across the two calls (plain
 * parameters read twice need no separate temps — cookbook's cached-pointer
 * rule).
 */
extern void AddPrim(u8 *ot, u8 *prim);

void AddXG4(u8 *ot, u8 *ply)
{
    AddPrim(ot, ply + 8);
    AddPrim(ot, ply);
}
