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
 * FUN_8001b174 (0x8001b174) — reads byte offset 6 of the selected controller
 * record (the low byte of controller_input.unk_2[2]).  The pad API encodes a
 * physical port in the high nibble and a multitap slot in the low nibble.
 *
 * This entry point receives a physical port number, so it first converts it
 * to that encoded representation (`arg0 << 4`) before using the ordinary
 * decoder shared by the rest of PADCMD.C.  Keeping the encoded value and the
 * selected pad as separate locals makes cc1 naturally emit the retail
 * sll16/sra12/sra4 sequence; no optimizer barrier is involved.
 */
u8 FUN_8001b174(short arg0)
{
    s32 port = arg0 << 4;
    TPadPort *pad = &PadPort[port >> 4][port & 3];

    return pad->active;
}
