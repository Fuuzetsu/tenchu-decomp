#include "common.h"
#include "main.exe.h"

/*
 * FUN_8001b4e0 (0x8001b4e0) — returns &PadPort[arg0 >> 4][arg0 & 3] (the whole
 * controller_input record, not the .held field like GetRealPad). No callees;
 * pure address arithmetic (row*0x38 + col*0xE), so unlike GetRealPad there's
 * no PadProc() call and no ordering trick needed — plain 2D-array indexing
 * reproduces the row-then-column evaluation order directly.
 */
void *FUN_8001b4e0(s32 arg0)
{
    return &PadPort[arg0 >> 4][arg0 & 3];
}
