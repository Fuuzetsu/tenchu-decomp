#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetPadXY(short no, short *x, short *y);
 *     PADCMD.C:285, 6 src lines, frame 8 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a0       short no
 *     param $a1       short * x
 *     param $a2       short * y
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * GetPadXY (0x8001b480) — writes the x/y analog-stick fields through the
 * out-parameters. `port = no << 4` converts the plain controller number to
 * the encoded row/slot convention used by the PADCMD.C family; the normal
 * `port >> 4` / `port & 3` lookup therefore selects PadPort[no][0]. Keeping
 * that encoded value and the selected record as ordinary locals makes cc1
 * emit the target's sll16/sra12/sra4 chain and compute the shared address
 * once. No optimizer barrier is needed.
 *
 * This exact human-shaped source falsifies the former SIGNEXT-SPLIT park.
 * The demo's same-named function has the same three-shift prefix and the
 * same two field stores, with only its earlier 12-byte record stride and
 * trivial-frame epilogue differing from retail.
 */
void GetPadXY(short no, short *x, short *y)
{
    s32 port;
    TPadPort *pad;

    port = no << 4;
    pad = &PadPort[port >> 4][port & 3];
    *x = (short)pad->x;
    *y = (short)pad->y;
}
