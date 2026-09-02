#include "common.h"
#include "main.exe.h"
#include "timpack.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadTIMpack(unsigned long *adr);
 *     3DCTRL.C:759, 39 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *     stack sp+16     struct RECT rect
 *     stack sp+24     struct GsIMAGE tim
 * END PSX.SYM */

extern char msg_no_image_pack_data[]; /* NO IMAGE PACK DATA */

short LoadTIMpack(unsigned long *adr)
{
    RECT rect;
    GsIMAGE tim;
    TIMPackIndex *index;
    u_long *p;
    u16 hw;
    short n;
    short i;

    if (adr == 0)
    {
        SystemOut(msg_no_image_pack_data);
    }
    adr++;
    index = (TIMPackIndex *)adr;
    hw = (u16)index->count;
    adr = index->offsets;
    i = 0;
    n = (short)hw;
    p = adr;
    if (n > 0)
    {
        do
        {
            GsGetTimInfo(TIM_PACK_IMAGE(p, adr), &tim);
            setRECT(&rect, tim.px, tim.py, tim.pw, tim.ph);
            LoadImage(&rect, tim.pixel);
            if (TIM_HAS_CLUT(tim.pmode) != 0)
            {
                setRECT(&rect, tim.cx, tim.cy, tim.cw, tim.ch);
                LoadImage(&rect, tim.clut);
                /* Empty loop retained for code layout; its original source construct is unknown. */
                do
                {
                } while (0);
            }
            i++;
            adr++;
        } while (i < n);
    }
    DrawSync(0);
}
