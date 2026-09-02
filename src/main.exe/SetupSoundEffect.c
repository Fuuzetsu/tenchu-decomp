#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupSoundEffect(short mode, short stage);
 *     SEMNG.C:26, 12 src lines, frame 144 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short mode
 *     param $a1       short stage
 *     stack sp+24     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct SoundEffect *StageSE;
 * END PSX.SYM */

extern void DisposeSE(SoundEffect *se);
extern SoundEffect *SetupSE(u8 *vab);
extern void sprintf(char *s, char *fmt, ...);

extern u8 CHOSEN_LANGUAGE;
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
