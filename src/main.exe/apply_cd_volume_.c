#include "common.h"
#include "tuning.h"
#include "main.exe.h"

/*
 * apply_cd_volume_ (0x8004f68c, 0x34 bytes) — thin forwarder: resets the CD-audio
 * volume to max on both channels (SsSetMVol(0x7F,0x7F)), then re-applies the
 * persisted volume byte gSoundLevel to both channels of the CD-audio status
 * via set_cda_volume_ (see set_cda_volume_.c — it writes TCdaStatus.voll/volr).
 * gSoundLevel is the standalone-symbol view of TLinkInfo.SoundLevel at
 * offset 0x5A in the 0x80010000 persistent-state blob; the original source
 * used both direct globals and pointer-based access to that shared state.
 */

extern void SsSetMVol(int voll, int volr);
extern void set_cda_volume_(u8 voll, u8 volr);

void apply_cd_volume_(void)
{
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    set_cda_volume_(gSoundLevel, gSoundLevel);
}
