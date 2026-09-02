#include "common.h"
#include "main.exe.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetPad(short no);
 *     PADCMD.C:293, 15 src lines, frame 8 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short no
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

short GetPad(short no)
{
    u16 *button;
    s32 port;

    port = no << 4;
    button = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                     [port & PAD_SLOT_INDEX_MASK]
                         .button;
    return *button;
}
