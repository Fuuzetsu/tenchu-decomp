#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SystemOut(unsigned char *string);
 *     VALLOC.C:305, 3 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * string
 * END PSX.SYM */

/*
 * SystemOut (0x80016e8c, 0x8 bytes) — same TU as vinit.c/vsize.c. The fatal
 * "system halt" hook: it never returns, and never even reads `string` (the
 * message is presumably only printed by a debug build of this function). The
 * whole body is a single self-jump, so the loop's head label coincides with
 * the function entry.
 */

void SystemOut(unsigned char *string)
{
    for (;;)
    {
    }
}
