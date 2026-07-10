#include "common.h"
#include "main.exe.h"

/*
 * GetTIMpackInfo (0x80018ae8) — index a TIM-pack's offset table (same
 * "skip the leading u_long ID word" convention as GetTIMInfo.c/LoadTIM.c):
 * `adr[1]` is the pack's element count, `adr+2` is the base of a table of
 * per-element byte offsets (relative to that same base, +4 more to skip
 * each element's own ID word) into the packed TIM data. Fails (returns 0)
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
extern void GsGetTimInfo(u_long *tim, GsIMAGE *im);

short GetTIMpackInfo(u_long *adr, GsIMAGE *image, int idx)
{
    short i;
    u_long *puVar3;
    u_long *puVar4;

    adr = adr + 1;
    if (idx < 0 || (puVar4 = adr + 1, (int)adr[0] <= idx)) {
        return 0;
    }
    puVar3 = puVar4;
    i = 0;
    if (idx > 0) {
        do {
            i = i + 1;
            puVar3 = puVar3 + 1;
        } while (i < idx);
    }
    GsGetTimInfo((u_long *)((int)puVar4 + puVar3[0] + 4), image);
    return 1;
}
