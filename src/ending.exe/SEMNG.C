#include "common.h"
#include "ending.exe.h"
#include "../main.exe/item.h"
#include "../main.exe/sound.h"

/* Ending effects use the stage bank at full volume without spatial falloff. */

extern char path_stage_sound[];
extern char fmt_stage_vab[]; /* %s%d%c.VAB */

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
                path_stage_sound, stage,
                character == RIKIMARU_0 ? 'R' : 'A');
        StageSE = SetupSE((u8 *)FileRead(name));
    }
}

short Sound(Humanoid *human, short seid)
{
    return PlaySE(StageSE, seid, SOUND_SPATIAL(0, SOUND_VOLUME_MAX));
}

/* ENDING uses the fixed development-directory prefix for stage effects. */
char fmt_stage_vab[] = "%s%d%c.VAB";
char path_stage_sound[] = "K:\\WORK\\CDIMAGE\\SOUND\\STAGE";
