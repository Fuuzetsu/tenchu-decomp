#include "common.h"
#include "main.exe.h"

/*
 * get_pad_active_ (0x8001b174) — reads the selected controller record's
 * `active` byte (TPadPort offset 6).  The pad API encodes a
 * physical port in the high nibble and a multitap slot in the low nibble.
 *
 * This entry point receives a physical port number, so it first converts it
 * to that encoded representation (`arg0 << 4`) before using the ordinary
 * decoder shared by the rest of PADCMD.C.  Keeping the encoded value and the
 * selected pad as separate locals makes cc1 naturally emit the retail
 * sll16/sra12/sra4 sequence; no optimizer barrier is involved.
 */
u8 get_pad_active_(short arg0)
{
    s32 port = arg0 << 4;
    TPadPort *pad = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                           [port & PAD_SLOT_INDEX_MASK];

    return pad->active;
}
