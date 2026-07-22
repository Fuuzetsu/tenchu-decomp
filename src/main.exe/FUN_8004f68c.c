#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gSoundLevel;
 * END PSX.SYM */

/*
 * FUN_8004f68c (0x8004f68c, 0x34 bytes) — thin forwarder: resets the CD-audio
 * volume to max on both channels (SsSetMVol(0x7F,0x7F)), then re-applies the
 * persisted volume byte gSoundLevel to both channels of the CD-audio status
 * via FUN_8004fbf4 (see FUN_8004fbf4.c — it writes TCdaStatus.voll/volr).
 * gSoundLevel is the standalone-symbol view of TLinkInfo.SoundLevel at
 * offset 0x5A in the 0x80010000 persistent-state blob; the original source
 * used both direct globals and pointer-based access to that shared state.
 */

extern void SsSetMVol(int voll, int volr);
extern void FUN_8004fbf4(u8 voll, u8 volr);

void FUN_8004f68c(void)
{
    SsSetMVol(0x7F, 0x7F);
    FUN_8004fbf4(gSoundLevel, gSoundLevel);
}
