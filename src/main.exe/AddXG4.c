#include "common.h"
#include "main.exe.h"

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
