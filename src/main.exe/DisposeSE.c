#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeSE(struct SoundEffect *se);
 *     AUDIO.C:61, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct SoundEffect * se
 * END PSX.SYM */

/*
 * DisposeSE (0x80018da0) — silence all voices, close the VAB, then free a
 * sound-effect object's VAB header buffer and the SoundEffect itself. Same
 * null-check-then-free shape as DisposeBG/DisposeAfterimage, plus an extra
 * unconditional SsUtAllKeyOff(0)/SsVabClose(se->VABid) pair before the frees
 * (se survives the first three calls in a callee-saved register; the last
 * call takes it as the argument). Uses game_types.h's shared SoundEffect
 * (VABid@0 s16, program@2 s16 unused here, VABhead@4 pointer).
 */
extern void SsUtAllKeyOff(s32 flag);
extern void SsVabClose(vab_id id);
extern void vfree(void *p);

void DisposeSE(SoundEffect *se)
{
    if (se != 0)
    {
        SsUtAllKeyOff(0);
        SsVabClose(se->VABid);
        vfree(se->VABhead);
        vfree(se);
    }
}
