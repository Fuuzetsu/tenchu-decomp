#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetTIMInfo(unsigned long *adr, struct GsIMAGE *image);
 *     3DCTRL.C:749, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       unsigned long * adr
 *     param $a1       struct GsIMAGE * image
 * END PSX.SYM */

/* GetTIMInfo (0x80018ac8) — skips the leading u_long (TIM ID word) and hands
 * the rest of the buffer to the real PSY-Q GsGetTimInfo(); always "succeeds"
 * (returns 1). main.exe.h already declares this TU's prototype as
 * `void *GetTIMInfo(u_long *adr, GsIMAGE *image)` (both matched callers, AddMisc
 * and BriefingAndInventorySelectionScreen, discard the return value) — Ghidra
 * types the return `short`/literal 1, so return an explicit `(void *)1` to
 * stay compatible with the header's declared type. */

void *GetTIMInfo(u_long *adr, GsIMAGE *image)
{
    GsGetTimInfo(adr + 1, image);
    return (void *)1;
}
