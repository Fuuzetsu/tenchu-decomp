#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * FUN_8001b4e0 (0x8001b4e0) — returns &PadPort[arg0 >> 4][arg0 & 3] (the whole
 * TPadPort record, not the .held field like GetRealPad). No callees;
 * pure address arithmetic (row*0x38 + col*0xE), so unlike GetRealPad there's
 * no PadProc() call and no ordering trick needed — plain 2D-array indexing
 * reproduces the row-then-column evaluation order directly.
 */
void *FUN_8001b4e0(s32 arg0)
{
    return &PadPort[arg0 >> 4][arg0 & 3];
}
