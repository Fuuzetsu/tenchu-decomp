#include "common.h"
#include "main.exe.h"
#include "tim.h"

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

s16 GetTIMInfo(u_long *adr, GsIMAGE *image)
{
    GsGetTimInfo(TIM_FILE_IMAGE(adr), image);
    return 1;
}
