#include "common.h"
#include "main.exe.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetPad(short no);
 *     PADCMD.C:293, 15 src lines, frame 8 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * GetPad (0x8001b144) — held-buttons for controller row `no`, slot zero.
 * The pad API represents a controller as `(row << 4) | slot`; converting `no`
 * to that ordinary encoded-port value before using both halves naturally emits
 * retail's sll16/sra12/sra4 chain.  Keeping the field address in `held` also
 * reproduces the target's address-materialisation order, just as GetRealPad's
 * matched source does.
 *
 * The earlier direct `PadPort[no][0]` draft was a local minimum: it made cc1
 * fold the conversion to sll16/sra16 and led to the incorrect claim that the
 * three-shift form required an inline-asm optimizer barrier.  The demo homolog
 * uses the same three shifts, and its line table separates the pointer setup
 * from the later held-field load, corroborating this human source structure.
 */
s16 GetPad(short no)
{
    u16 *button;
    s32 port;

    port = no << 4;
    button = &PadPort[port >> 4][port & 3].button;
    return *button;
}
