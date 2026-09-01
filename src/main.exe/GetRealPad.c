#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetRealPad(int port);
 *     PADCMD.C:276, 5 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int port
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * Returns the held-buttons word for controller [port >> 4][port & 3].
 *
 * The pointer temporary forces GCC to compute the row index (`port >> 4`) before
 * the column (`port & 3`) — the order the original picks. Writing the natural
 * `return PadPort[port >> 4][port & 3].button;` computes them the other way and
 * doesn't byte-match. (With the old non-canonical cc1 this also needed a
 * `do/while(0)` wrapper; the canonical gcc-2.8.1-psx doesn't.)
 */
long GetRealPad(int port)
{
    u16 *button;
    PadProc();
    button = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                     [port & PAD_SLOT_INDEX_MASK]
                         .button;
    return *button;
}
