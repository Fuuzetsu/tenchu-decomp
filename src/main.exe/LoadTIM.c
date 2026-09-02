#include "common.h"
#include "main.exe.h"
#include "tim.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadTIM(unsigned long *adr);
 *     3DCTRL.C:718, 27 src lines, frame 64 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *     stack sp+16     struct RECT rect
 *     stack sp+24     struct GsIMAGE tim
 * END PSX.SYM */

extern char msg_no_image_data[]; /* NO IMAGE DATA */

short LoadTIM(unsigned long *adr)
{
    RECT rect;
    GsIMAGE tim;

    if (adr == 0)
    {
        SystemOut(msg_no_image_data);
    }
    GsGetTimInfo(TIM_FILE_IMAGE(adr), &tim);
    setRECT(&rect, tim.px, tim.py, tim.pw, tim.ph);
    LoadImage(&rect, tim.pixel);
    if (TIM_HAS_CLUT(tim.pmode))
    {
        setRECT(&rect, tim.cx, tim.cy, tim.cw, tim.ch);
        LoadImage(&rect, tim.clut);
    }
    DrawSync(0);
}
