#include "common.h"
#include "main.exe.h"

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

extern void ComPad(int port, u8 *rxbuf);
extern u8 Anakon;

static inline void PadShock(s32 port, s32 act1, s32 act2)
{
    TPadPort *p = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                         [port & PAD_SLOT_INDEX_MASK];

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
        p->act1 = 0;
        p->act2 = 0;
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
        PadShock(PAD_PORT_1, 1,
                 PadArrange.pow * (PadArrange.attack - ct) /
                     PadArrange.attack);
        PadArrange.time++;
        return;
    }

    ct += PadArrange.release;
    if (ct > 0)
    {
        PadShock(PAD_PORT_1, 0,
                 PadArrange.pow * ct / PadArrange.release);
        PadArrange.time++;
        return;
    }

    PadShock(PAD_PORT_1, 0, 0);
}
