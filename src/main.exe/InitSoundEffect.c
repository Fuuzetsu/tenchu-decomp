#include "common.h"
#include "tuning.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitSoundEffect(void);
 *     AUDIO.C:28, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gSound;
 * END PSX.SYM */

extern void SsInit(void);
extern void SsSetTickMode(int mode);
extern void SsStart(void);
extern void SsSetMVol(int voll, int volr);
extern void SsSetMono(void);
extern void SsSetStereo(void);

void InitSoundEffect(void)
{
    SsInit();
    SsSetTickMode(1);
    SsStart();
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    if (gSound != SOUND_MODE_MONO)
    {
        SsSetStereo();
    }
    else
    {
        SsSetMono();
    }
}
