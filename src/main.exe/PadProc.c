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

/*
 * PadProc (0x8001ada4) services the direct pad and multitap header, then
 * advances PadArrange's attack/release rumble envelope.  The expired-release
 * path turns both actuators off without performing the second time increment.
 *
 * PadShock appears earlier in the original PADCMD.C and is inlined at the
 * three writes here.  Its second actuator argument is an int, while act2 is a
 * raw unsigned byte: negative values are converted to the 0..255 byte domain
 * by adding 0x100 before the actuator pair is written.  GCC removes that
 * source branch after QImode truncation, so standalone PadShock's instructions
 * are unchanged.  Before jump folding, however, the two real pair-write paths
 * keep both arguments live; that produces retail's a3 constant, a0 quotient,
 * branch-specific PadPort pointers, and instruction order in both envelope
 * arms.  The demo and trial PadProc bodies have the same register graph, and
 * the demo line table places each inlined expansion on PADCMD.C lines 260 and
 * 266 (with the motor-off expansion on line 270).
 */

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

    ComPad(0, ComBuf[0]);
    ComPad(0x10, ComBuf[1]);

    /* The negate-then-add split is byte-required (the folded subtract
     * recolors the loads; measured). */
    ct = -PadArrange.time++;
    ct += PadArrange.attack;
    if (ct > 0)
    {
        PadShock(0, 1,
                     PadArrange.pow * (PadArrange.attack - ct) /
                         PadArrange.attack);
    }
    else
    {
        ct += PadArrange.release;
        if (ct <= 0)
            goto motor_off;
        PadShock(0, 0,
                     PadArrange.pow * ct / PadArrange.release);
    }
    PadArrange.time++;
    return;

motor_off:
    PadShock(0, 0, 0);
}
