#include "common.h"
#include "menu.exe.h"
#include "../main.exe/item.h"
#include "../main.exe/sound.h"

/* Menu effects use the stage bank at full volume without spatial falloff. */

extern char *STAGE_SOUND_PREFICES[N_LANGUAGES];
extern char fmt_stage_vab[]; /* %sSTAGE%d%c.VAB */

void SetupSoundEffect(character_kind character, short stage)
{
    u8 name[100];

    if (StageSE != 0)
    {
        DisposeSE(StageSE);
    }
    StageSE = 0;
    if (stage >= 0)
    {
        sprintf((char *)name, fmt_stage_vab,
                STAGE_SOUND_PREFICES[CHOSEN_LANGUAGE], stage,
                character == RIKIMARU_0 ? 'R' : 'A');
        StageSE = SetupSE((u8 *)FileRead(name));
    }
}

short Sound(Humanoid *human, short seid)
{
    return PlaySE(StageSE, seid, SOUND_SPATIAL(0, SOUND_VOLUME_MAX));
}
