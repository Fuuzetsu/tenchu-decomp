#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * PSX.SYM suggests this may be `ViewAdjustBG` (LOW confidence, 3DCTRL.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

extern short LoadTIMpack(unsigned long *adr);
extern void vfree(void *p);

void LoadTIMpackAndFree(u_long *tim)
{
    LoadTIMpack(tim);
    vfree(tim);
}
