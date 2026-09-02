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

long GetRealPad(int port)
{
    u16 *button;
    PadProc();
    button = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                     [port & PAD_SLOT_INDEX_MASK]
                         .button;
    return *button;
}
