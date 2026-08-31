#include "common.h"
#include "main.exe.h"

/*
 * get_pad_record_ (0x8001b4e0) — returns &PadPort[arg0 >> 4][arg0 & 3] (the whole
 * TPadPort record, not the .held field like GetRealPad). No callees;
 * pure address arithmetic (row*0x38 + col*0xE), so unlike GetRealPad there's
 * no PadProc() call and no ordering trick needed — plain 2D-array indexing
 * reproduces the row-then-column evaluation order directly.
 */
TPadPort *get_pad_record_(s32 arg0)
{
    return &PadPort[arg0 >> 4][arg0 & 3];
}
