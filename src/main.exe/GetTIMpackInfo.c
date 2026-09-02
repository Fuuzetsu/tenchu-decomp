#include "common.h"
#include "main.exe.h"
#include "timpack.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetTIMpackInfo(unsigned long *adr, struct GsIMAGE *image, int idx);
 *     3DCTRL.C:802, 17 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * adr
 *     param $a1       struct GsIMAGE * image
 *     param $a2       int idx
 *     stack sp+16     struct RECT rect
 * END PSX.SYM */

short GetTIMpackInfo(unsigned long *adr, GsIMAGE *image, int idx)
{
    short i;
    TIMPackIndex *index;
    u_long *cursor;
    u_long *offsets;

    adr++;
    index = (TIMPackIndex *)adr;
    if (idx < 0 ||
        (offsets = index->offsets, (int)index->count <= idx))
    {
        return 0;
    }
    cursor = offsets;
    i = 0;
    if (idx > 0)
    {
        do
        {
            i++;
            cursor++;
        } while (i < idx);
    }
    GsGetTimInfo(TIM_PACK_IMAGE(offsets, cursor), image);
    return 1;
}
