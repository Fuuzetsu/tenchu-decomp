#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitSoundEffect(void);
 *     AUDIO.C:28, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * InitSoundEffect (0x80018c80) — SPU/SoundSystem bring-up: init the SPU
 * library, switch it to VSync tick mode, start the sequencer, reset the
 * master volume to max on both channels, then pick mono/stereo output from
 * the persisted D_80010059 byte (PersistentState._89_1_ — offset 0x59 from
 * the 0x80010000 persistent-state blob, inside game_types.h's opaque
 * field_0x49[0x15] catch-all span; kept as a fresh extern byte, same
 * "don't carve the shared struct for one still-unidentified field" pattern
 * as reset_alert_duration's gNannido / FUN_8004f68c's D_8001005A).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Ghidra renders the dispatch `if (byte == 0) Mono(); else Stereo();`,
 *    but SsSetStereo() sits physically FIRST (right after the `beqz`) with
 *    SsSetMono() at the branch target — a branch-if-equal into a
 *    physically-later block, so the real source polarity is inverted:
 *    `if (byte != 0) Stereo(); else Mono();`.
 */
extern void SsInit(void);
extern void SsSetTickMode(int mode);
extern void SsStart(void);
extern void SsSetMVol(int voll, int volr);
extern void SsSetMono(void);
extern void SsSetStereo(void);
extern u8 D_80010059;

void InitSoundEffect(void)
{
    SsInit();
    SsSetTickMode(1);
    SsStart();
    SsSetMVol(0x7F, 0x7F);
    if (D_80010059 != 0)
    {
        SsSetStereo();
    }
    else
    {
        SsSetMono();
    }
}
