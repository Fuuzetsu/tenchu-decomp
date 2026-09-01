#include "common.h"
#include "main.exe.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetPadXY(short no, short *x, short *y);
 *     PADCMD.C:285, 6 src lines, frame 8 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short no
 *     param $a1       short * x
 *     param $a2       short * y
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * GetPadXY (0x8001b480) — writes the x/y analog-stick fields through the
 * out-parameters. `port = no << 4` converts the plain controller number to
 * the encoded row/slot convention used by the PADCMD.C family; the normal
 * `port >> 4` / `port & 3` lookup therefore selects PadPort[no][0]. Keeping
 * that encoded value and the selected record as ordinary locals makes cc1
 * emit the target's sll16/sra12/sra4 chain and compute the shared address
 * once. No optimizer barrier is needed.
 *
 * This exact human-shaped source falsifies the former SIGNEXT-SPLIT park.
 * The demo's same-named function has the same three-shift prefix and the
 * same two field stores, with only its earlier 12-byte record stride and
 * trivial-frame epilogue differing from retail.
 */
void GetPadXY(short no, short *x, short *y)
{
    s32 port;
    TPadPort *pad;

    port = no << 4;
    pad = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                  [port & PAD_SLOT_INDEX_MASK];
    *x = (u16)pad->x;
    *y = (u16)pad->y;
}
