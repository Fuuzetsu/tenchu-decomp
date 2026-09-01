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

/*
 * GetTIMpackInfo (0x80018ae8) — index a TIM-pack's offset table (same
 * "skip the leading u_long ID word" convention as GetTIMInfo.c/LoadTIM.c):
 * TIMPackIndex supplies the element count and a table of per-element byte
 * offsets relative to that table; TIMPackEntry supplies the ID word skipped
 * before the packed TIM data. Fails (returns 0)
 * for an out-of-range idx; otherwise walks the offset table to `idx` and
 * hands the located TIM to GsGetTimInfo.
 *
 * Matching notes (docs/matching-cookbook.md): `i` is a plain `short` loop
 * counter — cc1's combine pass proves `i = i + 1` need not be truncated at
 * every assignment (only the compare needs the 16-bit view, and modular
 * add/truncate commute), so the asm keeps the raw 32-bit accumulation in
 * one register and only sign-extends a throwaway copy for the `while`
 * test — Ghidra renders that literally as `iVar2 * 0x10000 >> 0x10`
 * instead of inferring a `short` type.
 */
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
