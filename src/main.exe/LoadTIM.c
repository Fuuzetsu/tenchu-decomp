#include "common.h"
#include "main.exe.h"
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

/*
 * LoadTIM (0x80018904, 0xb0 bytes) - loads a TIM's pixel data (and, when the
 * TIM carries a CLUT per GsIMAGE's pmode bit 3, its CLUT too) via the PSYQ
 * libgpu LoadImage, having fetched the image geometry through GsGetTimInfo
 * (same "skip the leading u_long ID word" convention as GetTIMInfo.c).
 * SystemOut is annotated noreturn by Ghidra but the compiled code falls
 * straight through after the call (no early return) - same shape as
 * InsertConflict.c/LoadAreaMap.c's identical SystemOut-then-continue idiom.
 * The recovered API returns `short`. Retail has no explicit return after
 * DrawSync(0), leaving that call's residual value in $v0; every known caller
 * ignores it. This is ordinary old-C fallthrough, not evidence for replacing
 * the original return type with `void`.
 */
extern char msg_no_image_data[]; /* NO IMAGE DATA */

short LoadTIM(unsigned long *adr)
{
    RECT rect;
    GsIMAGE tim;

    if (adr == 0)
    {
        SystemOut(msg_no_image_data);
    }
    GsGetTimInfo(adr + 1, &tim);
    rect.x = tim.px;
    rect.y = tim.py;
    rect.w = tim.pw;
    rect.h = tim.ph;
    LoadImage(&rect, tim.pixel);
    if ((tim.pmode >> 3) & 1)
    {
        rect.x = tim.cx;
        rect.y = tim.cy;
        rect.w = tim.cw;
        rect.h = tim.ch;
        LoadImage(&rect, tim.clut);
    }
    DrawSync(0);
}
