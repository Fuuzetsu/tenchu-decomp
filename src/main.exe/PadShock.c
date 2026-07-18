#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadShock(int port, int p1, int p2);
 *     PADCMD.C:109, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       int port
 *     param $a1       int p1
 *     param $a2       int p2
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * PadShock (0x8001b404) — set (or clear) the rumble-motor bytes at offset
 * 8/9 of PadPort[port>>4][port&3] (same row/col address arithmetic as
 * FUN_8001b4e0). D_8001005D (0x8001005d, one byte before the already-named
 * CHOSEN_LANGUAGE @0x8001005e in config/symbols) gates it: nonzero writes
 * the two motor bytes from p1/p2, zero clears both.
 */
extern u8 D_8001005D;

typedef struct
{
    u16 held;
    s16 x;
    s16 y;
    u8 active;
    u8 analog;
    u8 act1;
    u8 act2;
    u8 actbuf[2];
    u8 send;
    u8 pad;
} PadShockPort;

static inline void PadShockApply(s32 port, s32 p1, s32 p2)
{
    PadShockPort *p = (PadShockPort *)&PadPort[port >> 4][port & 3];
    PadShockPort *q = p;

    if (D_8001005D != 0) {
        p->act1 = p1;
        p->act2 = p2;
    } else {
        q->act1 = 0;
        q->act2 = 0;
    }
}

void PadShock(s32 port, s32 p1, s32 p2)
{
    PadShockApply(port, p1, p2);
}
