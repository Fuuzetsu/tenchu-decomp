#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeBG(struct BackGround *bg);
 *     3DCTRL.C:697, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct BackGround * bg
 * END PSX.SYM */

/*
 * DisposeBG (0x8001885c) — free a tile-BG object's three dynamic buffers
 * (`cell`@0x34, `work`@0x38, `index`@0x3C) then the BackGround itself. Same
 * null-check-then-free shape as DisposeAfterimage/DisposeMotionManager, one
 * more field. Ghidra renders the first free as `vfree(&bg->cell->u)` (a
 * union-address artifact of its GsBG/GsMAP struct nesting), but the raw .s
 * is a plain `lw $a0, 0x34($s0)` — a load of the named `cell` pointer, not
 * an address computation. The complete BackGround layout is shared in
 * game_types.h.
 */
extern void vfree(void *p);

void DisposeBG(BackGround *bg)
{
    if (bg != 0)
    {
        vfree(bg->cell);
        vfree(bg->work);
        vfree(bg->index);
        vfree(bg);
    }
}
