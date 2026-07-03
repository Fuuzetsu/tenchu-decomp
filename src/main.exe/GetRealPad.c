#include "common.h"
#include "main.exe.h"

/*
 * Returns the held-buttons word for controller [arg0 >> 4][arg0 & 3].
 *
 * The pointer temporary forces GCC to compute the row index (`arg0 >> 4`) before
 * the column (`arg0 & 3`) — the order the original picks. Writing the natural
 * `return PadPort[arg0 >> 4][arg0 & 3].held;` computes them the other way and
 * doesn't byte-match. (With the old non-canonical cc1 this also needed a
 * `do/while(0)` wrapper; the canonical gcc-2.8.1-psx doesn't.)
 */
buttons_held GetRealPad(s32 arg0)
{
    buttons_held *held;
    PadProc();
    held = &PadPort[arg0 >> 4][arg0 & 3].held;
    return *held;
}
