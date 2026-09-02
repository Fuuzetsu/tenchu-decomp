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
