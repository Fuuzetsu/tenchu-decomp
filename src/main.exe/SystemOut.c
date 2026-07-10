#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SystemOut(unsigned char *string);
 *     VALLOC.C:305, 3 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned char * string
 * END PSX.SYM */

/*
 * SystemOut (0x80016e8c, 0x8 bytes) — same TU as vinit.c/vsize.c. The fatal
 * "system halt" hook: it never returns, and never even reads `string` (the
 * message is presumably only printed by a debug build of this function). The
 * whole body is a single self-jump, so the loop's head label coincides with
 * the function entry.
 */

void SystemOut(u8 *string)
{
    for (;;)
    {
    }
}
