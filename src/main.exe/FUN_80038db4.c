#include "common.h"
#include "main.exe.h"

/*
 * FUN_80038db4 (0x80038db4) — identical shape to FUN_80038d10 just above it
 * (same 0x80038dxx TU): draws two GPU primitives out of one buffer, +8 then
 * +0, via the BIOS-linked DrawPrim(u8 *prim) (declared per-TU, as
 * AdtSelect.c already does). arg0 cached across both calls needs no
 * separate temp (cookbook's cached-pointer rule).
 */
extern void DrawPrim(u8 *prim);

void FUN_80038db4(u8 *arg0)
{
    DrawPrim(arg0 + 8);
    DrawPrim(arg0);
}
