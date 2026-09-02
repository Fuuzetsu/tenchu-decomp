#include "common.h"
#include "main.exe.h"

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

extern u8 Anakon;

static inline void PadShockApply(s32 port, s32 p1, s32 p2)
{
    TPadPort *p = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                         [port & PAD_SLOT_INDEX_MASK];
    TPadPort *q = p;

    if (Anakon != 0)
    {
        if (p2 < 0)
        {
            p->act1 = p1;
            p->act2 = p2 + 0x100;
        }
        else
        {
            p->act1 = p1;
            p->act2 = p2;
        }
    }
    else
    {
        q->act1 = 0;
        q->act2 = 0;
    }
}

void PadShock(s32 port, s32 p1, s32 p2)
{
    PadShockApply(port, p1, p2);
}
