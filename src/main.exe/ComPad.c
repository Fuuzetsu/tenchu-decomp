#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ComPad(int port, unsigned char *rxbuf);
 *     PADCMD.C:136, 100 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       int port
 *     param $s2       unsigned char * rxbuf
 *     reg   $s0       struct TPadPort * pad
 *     reg   $s1       int initlevel
 *     reg   $s0       int i
 *     reg   $s3       int port
 *     reg   $v1       int i
 *     reg   $v1       int i
 *     reg   $a1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * Reads one controller report into PadPort. A multitap report recursively
 * supplies four eight-byte subreports; an error report clears the port; a
 * normal report derives digital x/y values, records analog mode, and advances
 * the actuator setup state machine.
 *
 * Retail inserted `active` at offset 6 in the demo's 12-byte TPadPort, moving
 * the later byte fields by one and making this version 14 bytes. The repeated
 * block-scoped `int i` locals follow the original debug symbols.
 *
 * Matching notes:
 *  - The empty one-shot loop is a zero-code scheduling boundary. Together
 *    with the identical full-width assignments it keeps the target's
 *    `sh v0; move v1,v0` sequence instead of narrowing the copy to an `andi`
 *    or moving the store into a branch delay slot.
 *  - The second button test intentionally reloads `button`; the target contains
 *    a fresh `lhu` there.
 *  - Capturing `actbuf` before the Send guard fixes the outer branch delay
 *    slot. The recovered `u8 *` PadSetActAlign argument and six-byte static
 *    align object likewise reproduce the final guard/call schedule.
 */

extern int PadInfoMode(int port, int mode, int unused);
extern int PadGetState(int port);
extern int PadSetAct(int port, u8 *data, int len);
extern int PadSetActAlign(int port, u8 *data);
extern u8 align[6];

void ComPad(int port, u8 *rxbuf)
{
    TPadPort *pad;
    u8 *act;
    int i;
    int raw;
    int initlevel;
    int hi, lo;

    if ((rxbuf[1] >> 4) == 8)
    {
        for (i = 0; i < PAD_SLOTS_PER_PORT; i++)
        {
            ComPad(port + i, rxbuf + 2 + i * 8);
        }
        return;
    }

    pad = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                  [port & PAD_SLOT_INDEX_MASK];

    if (rxbuf[0] != 0)
    {
        pad->button = 0;
        pad->x = 0;
        pad->y = 0;
        pad->active = 0;
        return;
    }

    hi = rxbuf[2];
    lo = rxbuf[3];
    pad->y = 0;
    pad->x = 0;
    raw = ~(lo | (hi << 8));
    {
        int i;

        pad->button = raw;
        /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
        do
        {
        } while (0);
        /* Identical arms on the port test: retail's own dead branch,
         * byte-required (collapsing moves the store to v1; measured). */
        if (port != 0)
            i = raw;
        else
            i = raw;
        /* 0x2D = 45: the synthesized stick deflection for digital pads.
         * The raw recycle is byte-required (a direct store loses the
         * branch shape; measured). */
        if (i & PADLright)
        {
            raw = 0x2D;
            pad->x = raw;
        }
        else if (i & PADLleft)
        {
            raw = -0x2D;
            pad->x = raw;
        }
    }
    {
        int i;

        i = pad->button;
        if (i & PADLdown)
            pad->y = 0x2D;
        else if (i & PADLup)
            pad->y = -0x2D;
    }

    if ((rxbuf[1] >> 4) == 7)
        pad->fAnalog = 1;
    else
        pad->fAnalog = 0;

    if (PadInfoMode(port, 2, 0) != 0)
    {
        pad->actbuf[0] = pad->act1;
        pad->actbuf[1] = pad->act2;
    }
    else
    {
        pad->actbuf[0] = 0x40;
        pad->actbuf[1] = pad->act1;
    }

    initlevel = PadGetState(port);
    pad->active = 1;
    if (initlevel == 1)
        pad->Send = 0;

    act = pad->actbuf;
    if (pad->Send == 0)
    {
        PadSetAct(port, act, 2);
        if (initlevel != 2)
        {
            if (initlevel != 6)
                return;
            PadSetActAlign(port, align);
        }
        pad->Send = 1;
    }
}
