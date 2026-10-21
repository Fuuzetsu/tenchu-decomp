#include "common.h"
#include "ending.exe.h"
#include "../main.exe/item.h"
#include "../main.exe/padcmd.h"
#include <psxsdk/libmcrd.h>
#include <psxsdk/libpad.h>

/* Controller reports, command history, and the ending rumble envelope. */

extern u8 Anakon;
extern u8 align[6];

void ComPad(int port, u8 *rxbuf)
{
    EndingPadPort *pad;
    u8 *act;
    int i;
    int initlevel;
    int pressed;

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

    pad = &EndingPads[port >> PAD_PORT_INDEX_SHIFT]
                  [port & PAD_SLOT_INDEX_MASK];

    if (rxbuf[PAD_REPORT_STATUS] != PAD_REPORT_STATUS_OK)
    {
        pad->button = 0;
        pad->x = 0;
        pad->y = 0;
        return;
    }

    PAD_DECODE_DIGITAL_REPORT(pad, rxbuf, pressed);
    {
        int buttons = pressed;

        /* Digital pads synthesize a stick deflection of 45. */
        pressed &= PADLright;
        if (pressed)
            pad->x = PAD_DIGITAL_AXIS_MAGNITUDE;
        else if (buttons & PADLleft)
            pad->x = -PAD_DIGITAL_AXIS_MAGNITUDE;
    }
    {
        int buttons = pad->button;

        if (buttons & PADLdown)
            pad->y = PAD_DIGITAL_AXIS_MAGNITUDE;
        else if (buttons & PADLup)
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

static inline void PadShockApply(s32 port, s32 act1, s32 act2)
{
    EndingPadPort *p = &EndingPads[port >> PAD_PORT_INDEX_SHIFT]
                         [port & PAD_SLOT_INDEX_MASK];
    EndingPadPort *q = p;

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

        for (j = PAD_COMMAND_STREAM_LENGTH - 1; j > 0; j--)
        {
            pad->stream[j] = pad->stream[j - 1];
        }
        pad->stream[0] = 0;
        return Command[i]->command;
    }
    return CMD_NONE;
}

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
                for (j = 1; j < n; j++)
                {
                    pad->stream[j - 1] = inputs[j];
                }
            }
            pad->time = one;
            return (s16)inputs[0];
        }
        i++;
    }
    return 0;
}

short GetPad(short no)
{
    u16 *button;
    s32 port;

    port = no << 4;
    button = &EndingPads[port >> PAD_PORT_INDEX_SHIFT]
                     [port & PAD_SLOT_INDEX_MASK]
                         .button;
    return *button;
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

long GetRealPad(int port)
{
    u16 *button;
    PadProc();
    button = &EndingPads[port >> PAD_PORT_INDEX_SHIFT]
                     [port & PAD_SLOT_INDEX_MASK]
                         .button;
    return *button;
}

void save_pad_analog_(void)
{
    TLinkInfo *ps =
        (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;

    if (EndingPads[0][0].fAnalog != 0)
    {
        ps->analog_pad_present |= 1;
    }
    else
    {
        ps->analog_pad_present &= ~1;
    }
}

void InitPadControl(void)
{
    int i;

    MemCardInit(0);
    MemCardStart();
    memset(ComBuf[0], 0, sizeof(ComBuf));
    memset(EndingPads, 0, sizeof(EndingPads));
    PadInitDirect(ComBuf[0], ComBuf[1]);
    PadStartCom();
    if ((((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->analog_pad_present &
         1) != 0)
    {
        for (i = PAD_ANALOG_MODE_SWITCH_DELAY; i > 0; i--)
        {
            VSync(0);
        }
        PadSetMainMode(PAD_PORT_1, PAD_MAIN_MODE_ANALOG,
                       PAD_MAIN_MODE_KEEP_LOCK);
    }
}

void PadShock(s32 port, s32 p1, s32 p2)
{
    PadShockApply(port, p1, p2);
}

/* Rumble envelope: power, elapsed time, attack, and release. */

void PadShockAR(int port, int pow, int attack, int release)
{
    PadArrange.time = 0;
    PadArrange.pow = pow;
    PadArrange.attack = attack;
    PadArrange.release = release;
}

void GetPadXY(short no, short *x, short *y)
{
    s32 port;
    EndingPadPort *pad;

    port = no << 4;
    pad = &EndingPads[port >> PAD_PORT_INDEX_SHIFT]
                  [port & PAD_SLOT_INDEX_MASK];
    *x = (u16)pad->x;
    *y = (u16)pad->y;
}
