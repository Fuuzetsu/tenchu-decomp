#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadProc(void);
 *     PADCMD.C:249, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $a0       int ct
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char ComBuf[2][34];
 *     extern struct tag_TItem items[30];
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
extern u8 ComBuf[2][34];

extern PadArrangeType PadArrange;

static inline void PadProcShock(s32 port, s32 value1, s32 value2)
{
    TPadPort *p = &PadPort[port >> 4][port & 3];
    TPadPort *q = p;

    if (Anakon != 0)
    {
        if (value2 < 0)
        {
            p->act1 = value1;
            p->act2 = value2 + 0x100;
        }
        else
        {
            p->act1 = value1;
            p->act2 = value2;
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

    ComPad(0, ComBuf[0]);
    ComPad(0x10, ComBuf[1]);

    ct = -PadArrange.time++;
    ct += PadArrange.attack;
    if (ct > 0)
    {
        PadProcShock(0, 1,
                     PadArrange.pow * (PadArrange.attack - ct) /
                         PadArrange.attack);
    }
    else
    {
        ct += PadArrange.release;
        if (ct <= 0)
            goto motor_off;
        PadProcShock(0, 0,
                     PadArrange.pow * ct / PadArrange.release);
    }
    PadArrange.time++;
    return;

motor_off:
    PadProcShock(0, 0, 0);
}
