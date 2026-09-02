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

    if (PAD_REPORT_TYPE(rxbuf) == PAD_REPORT_TYPE_MULTITAP)
    {
        for (i = 0; i < PAD_SLOTS_PER_PORT; i++)
        {
            ComPad(port + i,
                   rxbuf + PAD_REPORT_HEADER_SIZE +
                       i * PAD_SLOT_REPORT_SIZE);
        }
        return;
    }

    pad = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                  [port & PAD_SLOT_INDEX_MASK];

    if (rxbuf[PAD_REPORT_STATUS] != PAD_REPORT_STATUS_OK)
    {
        pad->button = 0;
        pad->x = 0;
        pad->y = 0;
        pad->active = 0;
        return;
    }

    hi = rxbuf[PAD_REPORT_BUTTON_HIGH];
    lo = rxbuf[PAD_REPORT_BUTTON_LOW];
    pad->y = 0;
    pad->x = 0;
    raw = ~(lo | (hi << 8));
    {
        int i;

        pad->button = raw;
        /* Empty loop retained for code layout; its original source construct is unknown. */
        do
        {
        } while (0);
        /* Retail keeps identical branches here; their original distinction is unknown. */
        if (port != 0)
            i = raw;
        else
            i = raw;
        /* Digital pads synthesize a stick deflection of 45. */
        if (i & PADLright)
        {
            raw = PAD_DIGITAL_AXIS_MAGNITUDE;
            pad->x = raw;
        }
        else if (i & PADLleft)
        {
            raw = -PAD_DIGITAL_AXIS_MAGNITUDE;
            pad->x = raw;
        }
    }
    {
        int i;

        i = pad->button;
        if (i & PADLdown)
            pad->y = PAD_DIGITAL_AXIS_MAGNITUDE;
        else if (i & PADLup)
            pad->y = -PAD_DIGITAL_AXIS_MAGNITUDE;
    }

    if (PAD_REPORT_TYPE(rxbuf) == PAD_REPORT_TYPE_ANALOG)
        pad->fAnalog = 1;
    else
        pad->fAnalog = 0;

    if (PadInfoMode(port, PAD_INFO_CURRENT_EXTENDED_ID, 0) != 0)
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
    if (initlevel == PAD_STATE_FIND_PAD)
        pad->Send = 0;

    act = pad->actbuf;
    if (pad->Send == 0)
    {
        PadSetAct(port, act, PAD_ACTUATOR_COUNT);
        if (initlevel != PAD_STATE_FIND_CTP1)
        {
            if (initlevel != PAD_STATE_STABLE)
                return;
            PadSetActAlign(port, align);
        }
        pad->Send = 1;
    }
}
