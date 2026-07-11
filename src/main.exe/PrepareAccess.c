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
 * This is the SAME idiom as FileRead's inlined guard (see its header): a
 * shared `void (*f)(void)` local assigned per if/else arm and used once after
 * the join is a pseudo LIVE ACROSS the block boundary, so `local-alloc.c`'s
 * `combine_regs` refuses to tie the `%hi(cbAccess)` temp to it
 * (`reg_qty[sreg] == -1` for any cross-block pseudo — cookbook's RTL-dump
 * section) and the temp free-falls to the first free class register, $v0.
 * `rtldump PrepareAccess --draft`'s `.greg` confirmed it: only pseudo 80
 * (`f`) needed global-alloc coloring (landing in $a0 via its call-argument
 * preference), while the `high(cbAccess)` temp was a separate block-local
 * pseudo local-alloc parked in $v0 with no tie attempt. Calling
 * `VSyncCallback` directly in each arm makes its argument pseudo local to
 * that one block, so `combine_regs` DOES tie the `%hi` temp to it (both
 * land in $a0 together), and `jump.c` cross-jump-merges the two calls'
 * identical tail back into the single `jal VSyncCallback` the target has.
 * MATCH.
 */
extern void VSyncCallback(void (*f)(void));
extern void cbAccess(void);
extern s32 AccessPower;

void PrepareAccess(void)
{
    if (AccessPower >= 0) {
        AccessPower = 0;
        VSyncCallback(cbAccess);
    } else {
        VSyncCallback(0);
    }
}
