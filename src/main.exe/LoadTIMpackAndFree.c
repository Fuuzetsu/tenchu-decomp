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

/*
 * LoadTIMpackAndFree (0x800188d8) — identical shape to LoadTIMAndFree just
 * above it: loads a TIM-pack via LoadTIMpack, then frees the buffer via
 * vfree. `tim` is cached in a callee-saved reg across both calls (a plain
 * parameter read twice needs no separate temp — cookbook's cached-pointer
 * rule).
 */
extern short LoadTIMpack(unsigned long *adr);
extern void vfree(void *p);

void LoadTIMpackAndFree(u_long *tim)
{
    LoadTIMpack(tim);
    vfree(tim);
}
