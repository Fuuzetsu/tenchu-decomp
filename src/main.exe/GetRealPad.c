#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetRealPad(int port);
 *     PADCMD.C:276, 5 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * Returns the held-buttons word for controller [arg0 >> 4][arg0 & 3].
 *
 * The pointer temporary forces GCC to compute the row index (`arg0 >> 4`) before
 * the column (`arg0 & 3`) — the order the original picks. Writing the natural
 * `return PadPort[arg0 >> 4][arg0 & 3].held;` computes them the other way and
 * doesn't byte-match. (With the old non-canonical cc1 this also needed a
 * `do/while(0)` wrapper; the canonical gcc-2.8.1-psx doesn't.)
 */
buttons_held GetRealPad(s32 port)
{
    buttons_held *held;
    PadProc();
    held = &PadPort[port >> 4][port & 3].held;
    return *held;
}
