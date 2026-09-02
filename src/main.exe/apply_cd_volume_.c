#include "common.h"
#include "tuning.h"
#include "main.exe.h"

extern void SsSetMVol(int voll, int volr);
extern void set_cda_volume_(u8 voll, u8 volr);

void apply_cd_volume_(void)
{
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    set_cda_volume_(gSoundLevel, gSoundLevel);
}
