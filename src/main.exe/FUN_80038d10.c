#include "common.h"
#include "main.exe.h"

/*
 * FUN_80038d10 (0x80038d10) — draws two GPU primitives back to back out of
 * one buffer: the primitive at +8 first, then the one at +0 (same
 * DrawPrim(u8 *prim) BIOS-linked call AdtSelect.c already declares locally
 * per this project's per-TU extern convention). arg0 is cached in a
 * callee-saved reg across both calls (a plain parameter read twice needs no
 * separate temp — see the cookbook's cached-pointer rule).
 */
extern void DrawPrim(u8 *prim);

void FUN_80038d10(u8 *arg0)
{
    DrawPrim(arg0 + 8);
    DrawPrim(arg0);
}
