#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "padcmd.h"
#include <psxsdk/libmcrd.h>
#include <psxsdk/libpad.h>

/*
 * Retail PADCMD.C adds six helpers, drops the demo SetPad routine, and
 * rearranges the surviving definitions. Both orders remain in the manifest.
 */

extern u8 Anakon;
extern u8 align[6];

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
        do
        {
        } while (0);
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadProc(void);
 *     PADCMD.C:249, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       int ct
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char ComBuf[2][34];
 *     extern struct PADCMD__141fake PadArrange;
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

static inline void PadShockApply(s32 port, s32 act1, s32 act2)
{
    TPadPort *p = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                         [port & PAD_SLOT_INDEX_MASK];
    TPadPort *q = p;

    if (Anakon != 0)
    {
        if (act2 < 0)
        {
            p->act1 = act1;
            p->act2 = act2 + 0x100;
        }
        else
        {
            p->act1 = act1;
            p->act2 = act2;
        }
    }
    else
    {
        q->act1 = 0;
        q->act2 = 0;
    }
}

void PadProc(void)
{
    int ct;

    ComPad(PAD_PORT_1, ComBuf[0]);
    ComPad(PAD_PORT_2, ComBuf[1]);

    ct = -PadArrange.time++;
    ct += PadArrange.attack;
    if (ct > 0)
    {
        PadShockApply(PAD_PORT_1, 1,
                      PadArrange.pow * (PadArrange.attack - ct) /
                          PadArrange.attack);
        PadArrange.time++;
        return;
    }

    ct += PadArrange.release;
    if (ct > 0)
    {
        PadShockApply(PAD_PORT_1, 0,
                      PadArrange.pow * ct / PadArrange.release);
        PadArrange.time++;
        return;
    }

    PadShockApply(PAD_PORT_1, 0, 0);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetCommand(struct PADtype *pad);
 *     PADCMD.C:333, 22 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct PADtype * pad
 *     reg   $a0       unsigned short * cmd
 *     reg   $a2       short i
 *     reg   $a1       short j
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned short *Command[12];
 * END PSX.SYM */

pad_command GetCommand(PADtype *pad)
{
    u16 *pattern;
    short i;
    short j;

    for (i = 0; Command[i] != 0; i++)
    {
        pattern = Command[i]->inputs;
        for (j = 0; pattern[j] != PAD_COMMAND_END; j++)
        {
            if (pattern[j] != pad->stream[j])
                break;
        }
        if (pattern[j] != PAD_COMMAND_END)
            continue;

        j = PAD_COMMAND_STREAM_LENGTH - 1;
        do
        {
            pad->stream[j] = pad->stream[j - 1];
            j--;
        } while (j > 0);
        pad->stream[0] = 0;
        return Command[i]->command;
    }
    return CMD_NONE;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SetCommand(struct PADtype *pad, short cmd);
 *     PADCMD.C:359, 17 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct PADtype * pad
 *     param $a1       short cmd
 *     reg   $a2       unsigned short * com
 *     reg   $a0       short i
 *     reg   $a0       short j
 *     reg   $a1       short k
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned short *Command[12];
 * END PSX.SYM */

short SetCommand(PADtype *pad, pad_command cmd)
{
    s16 i;
    PadCommandSequence *entry;
    u16 *inputs;
    s16 n;
    s16 j;
    s32 one;
    s16 found;

    i = 0;
    while (Command[i] != 0)
    {
        entry = Command[i];
        found = ((u16)entry->command == cmd);
        one = 1;
        if (found)
        {
            inputs = entry->inputs;
            n = 0;
            while (inputs[n] != PAD_COMMAND_END)
            {
                n++;
            }
            if (one < n)
            {
                j = 1;
                do
                {
                    pad->stream[j - 1] = inputs[j];
                    j++;
                } while (j < n);
            }
            pad->time = one;
            return (s16)inputs[0];
        }
        i++;
    }
    return 0;
}

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

short get_pad_active_(short arg0)
{
    s32 port = arg0 << 4;
    TPadPort *pad = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                           [port & PAD_SLOT_INDEX_MASK];

    return pad->active;
}

/* A negative frames_since_new_input value disables input until it wraps. */
s16 update_pressed_buttons(PADtype *buttons, u16 pressed)
{
    s16 i;
    u16 previously_pressed;

    if (buttons->time < 6 && ++buttons->time < 0)
    {
        pressed = 0;
    }

    previously_pressed = buttons->data;
    buttons->data = pressed;
    buttons->sdata = previously_pressed;
    buttons->trig = pressed & ~previously_pressed;

    if (buttons->sdata != buttons->data &&
        buttons->data != 0)
    {
        if (buttons->time > 5)
        {
            buttons->stream[0] = 0;
        }

        for (i = 3; i > 0; i--)
        {
            buttons->stream[i] =
                buttons->stream[i - 1];
        }

        buttons->time = 0;
        buttons->stream[0] = buttons->data;
    }

    return pressed;
}

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

void save_pad_analog_(void)
{
    TLinkInfo *ps =
        (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;

    if (PadPort[0][0].fAnalog != 0)
    {
        ps->analog_pad_present |= 1;
    }
    else
    {
        ps->analog_pad_present &= ~1;
    }
}

s32 remap_buttons_(s16 pad)
{
    s32 i;
    s32 selected_index;
    u16 acc;
    s32 test;
    u8 *mapped_button;

    mapped_button = ButtonAssign;
    acc = pad;
    selected_index = (s32)ControlScheme * BUTTONS_PER_CONTROL_SCHEME;
    i = 0;
    do
    {
        test = pad & ButtonAssign[i];
        if (test != 0)
        {
            mapped_button = &ButtonAssign[selected_index];
            acc = acc | *mapped_button;
        }
        else
        {
            mapped_button = &ButtonAssign[selected_index];
            acc = acc & ~*mapped_button;
        }
        i++;
        selected_index++;
    } while (i < BUTTONS_PER_CONTROL_SCHEME);
    return (s16)acc;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitPadControl(void);
 *     PADCMD.C:82, 13 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char ComBuf[2][34];
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

void InitPadControl(void)
{
    int i;

    MemCardInit(0);
    MemCardStart();
    memset(ComBuf[0], 0, sizeof(ComBuf));
    memset(PadPort, 0, sizeof(PadPort));
    PadInitDirect(ComBuf[0], ComBuf[1]);
    PadStartCom();
    if ((((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->analog_pad_present &
         1) != 0)
    {
        i = PAD_ANALOG_MODE_SWITCH_DELAY;
        do
        {
            VSync(0);
            i--;
        } while (i > 0);
        PadSetMainMode(PAD_PORT_1, PAD_MAIN_MODE_ANALOG,
                       PAD_MAIN_MODE_KEEP_LOCK);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadShock(int port, int p1, int p2);
 *     PADCMD.C:109, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int port
 *     param $a1       int p1
 *     param $a2       int p2
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

void PadShock(s32 port, s32 p1, s32 p2)
{
    PadShockApply(port, p1, p2);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadShockAR(int port, int pow, int attack, int release);
 *     PADCMD.C:241, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int port
 *     param $a1       int pow
 *     param $a2       int attack
 *     param $a3       int release
 *
 * Globals it touches, as the original declared them:
 *     extern struct PADCMD__141fake PadArrange;
 * END PSX.SYM */

/* Rumble envelope: power, elapsed time, attack, and release. */

void PadShockAR(int port, int pow, int attack, int release)
{
    PadArrange.time = 0;
    PadArrange.pow = pow;
    PadArrange.attack = attack;
    PadArrange.release = release;
}

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

void clear_pad_send_(void)
{
    get_pad_record_(PAD_PORT_1)->Send = 0;
}

TPadPort *get_pad_record_(s32 arg0)
{
    return &PadPort[arg0 >> PAD_PORT_INDEX_SHIFT]
                   [arg0 & PAD_SLOT_INDEX_MASK];
}
