#include "common.h"
#include "main.exe.h"

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
