#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetTIMInfo(unsigned long *adr, struct GsIMAGE *image);
 *     3DCTRL.C:749, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *     param $a1       struct GsIMAGE * image
 * END PSX.SYM */

/* GetTIMInfo (0x80018ac8) — skips the leading u_long (TIM ID word) and hands
 * the rest of the buffer to the real PSY-Q GsGetTimInfo(); always "succeeds"
 * (returns 1). */

s16 GetTIMInfo(u_long *adr, GsIMAGE *image)
{
    GsGetTimInfo(adr + 1, image);
    return 1;
}
