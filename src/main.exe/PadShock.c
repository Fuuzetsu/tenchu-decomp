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
 * PadShock (0x8001b404) sets (or clears) the rumble-motor bytes at offsets
 * 8/9 of PadPort[port>>4][port&3]. Anakon gates the pair: nonzero writes
 * p1 and p2, zero clears both. The large-motor input is an int but the port
 * field is a raw byte, so negative values are first moved into the 0..255
 * representation by adding 0x100. GCC folds that normalization after byte
 * truncation; its otherwise-hidden source paths are exposed by the exact
 * inlined PadProc register allocation.
 */
extern u8 Anakon;

static inline void PadShockApply(s32 port, s32 p1, s32 p2)
{
    TPadPort *p = &PadPort[port >> 4][port & 3];
    TPadPort *q = p;

    if (Anakon != 0) {
        if (p2 < 0) {
            p->act1 = p1;
            p->act2 = p2 + 0x100;
        } else {
            p->act1 = p1;
            p->act2 = p2;
        }
    } else {
        q->act1 = 0;
        q->act2 = 0;
    }
}

void PadShock(s32 port, s32 p1, s32 p2)
{
    PadShockApply(port, p1, p2);
}
