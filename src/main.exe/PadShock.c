#include "common.h"
#include "main.exe.h"

/*
 * PadShock (0x8001b404) — set (or clear) the rumble-motor bytes at offset
 * 8/9 of PadPort[port>>4][port&3] (same row/col address arithmetic as
 * FUN_8001b4e0). D_8001005D (0x8001005d, one byte before the already-named
 * CHOSEN_LANGUAGE @0x8001005e in config/symbols) gates it: nonzero writes
 * the two motor bytes from p1/p2, zero clears both.
 */
extern u8 D_8001005D;

void PadShock(s32 port, s8 p1, s8 p2)
{
    u8 *p = (u8 *)&PadPort[port >> 4][port & 3];
    u8 *q = p;

    if (D_8001005D != 0) {
        p[8] = p1;
        p[9] = p2;
        return;
    }
    q[8] = 0;
    q[9] = 0;
}
