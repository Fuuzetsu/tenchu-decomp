#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PrepareAccess(void);
 *     FILEIO.C:144, 8 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern int AccessPower;
 * END PSX.SYM */

/*
 * PrepareAccess (0x80019768) — arm or disarm the memory-card access vsync
 * callback: if AccessPower has gone negative, install NULL; otherwise clear
 * AccessPower and install cbAccess. Ghidra resolves `cbAccess` as a real
 * FUNCTION symbol (0x80018dec) so `f = cbAccess;` reads as the function's own
 * address — the raw .s confirms a `lui %hi(cbAccess)/addiu %lo(cbAccess)`
 * address computation, not a memory load (m2c's flatter `&D_80018DEC` is the
 * same thing, just unaware it's code). `AccessPower` is `%gp_rel` (a small
 * gp-relative DATA global DEFINED in this TU) — needs the gp-extern lists.
 *
 * Opposite-polarity lever: the asm's `bltz` branches to the `f = NULL` body
 * with the `AccessPower = 0; f = cbAccess;` body as the FALLTHROUGH — the
 * source condition is the negation of Ghidra's `AccessPower < 0` rendering
 * (cookbook: branch-to-later-block + fallthrough-is-other-body means
 * opposite polarity).
 *
 * STATUS: NON_MATCHING — 2 bytes (a pure register-allocation tie). The draft
 * is arithmetically correct, the right length, and every instruction matches
 * except the `cbAccess` address split: the target computes BOTH the `lui
 * %hi` and the following `addiu %lo` directly into $a0 (the call-argument
 * register `f` itself lives in), while cc1 here puts the `%hi` temp in $v0
 * and combines into $a0 with `addiu a0,v0,...`. regalloc.py shows only ONE
 * pseudo (`f`) needed real global-alloc coloring; the HI temp is a separate,
 * single-use pseudo with no preference edge toward $a0 in this compile.
 * Reordering the two fallthrough-branch statements (`f = cbAccess;` before
 * `AccessPower = 0;`) and a `do{}while(0)` wrapper around the branch body
 * were both tried — the former regressed to 9 bytes (shifts the whole
 * fallthrough block), the latter had no effect. autorules.py found no
 * mechanical win. A decomp-permuter run of ~87k iterations (--stop-on-zero,
 * -j8) never beat the score-10 base, so this is below the C level — a
 * local-alloc temp-coloring tie, not a source-shape choice.
 */
extern void VSyncCallback(void (*f)(void));
extern void cbAccess(void);
extern s32 AccessPower;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PrepareAccess", PrepareAccess);
#else
void PrepareAccess(void)
{
    void (*f)(void);

    if (AccessPower >= 0) {
        AccessPower = 0;
        f = cbAccess;
    } else {
        f = 0;
    }
    VSyncCallback(f);
}
#endif
