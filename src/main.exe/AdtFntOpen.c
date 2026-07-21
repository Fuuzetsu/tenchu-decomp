#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * AdtFntOpen (0x8005fbdc) — font-adapter open: opens the PSYQ libgpu font
 * stream via FntOpen(x,y,w,h,isbg,n), forwarding all SIX parameters
 * unchanged (x-h stay live in $a0-$a3 from entry straight through to the
 * call; only the two stack-passed trailing params isbg/n are visibly
 * reloaded — m2c only shows those two as the call's args because it can't
 * see untouched incoming registers, see docs/matching-cookbook.md's
 * "leading argument…invisible to m2c" note; Ghidra's full 6-arg call is
 * the real one), then stashes every argument into the font-adapter state
 * D_8008F1B8 (same global as AdtFntLoad.c's tx/ty@0x18/0x1C and
 * AdtQuiet.c's quiet@0x20).
 */
extern AdtFntState D_8008F1B8;

void AdtFntOpen(int x, int y, int w, int h, int isbg, int n)
{
    FntOpen(x, y, w, h, isbg, n);
    D_8008F1B8.x = x;
    D_8008F1B8.y = y;
    D_8008F1B8.w = w;
    D_8008F1B8.h = h;
    D_8008F1B8.isbg = isbg;
    D_8008F1B8.n = n;
}
