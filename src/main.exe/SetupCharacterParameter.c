#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * SetupCharacterParameter(short type, struct Humanoid *human);
 *     APPEAR.C:173, 25 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       short type
 *     param $s1       struct Humanoid * human
 *     reg   $a0       int idx
 *     reg   $a2       short * idtbl
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern short NowStage;
 *     extern short *StageAppearance[10];
 * END PSX.SYM */

Humanoid *SetupCharacterParameter(character_kind type, Humanoid *human)
{
    int idx;
    character_kind *idtbl;

    idx = 0;
    while (HumanData[idx].type != CHARACTER_KIND_END)
    {
        if (HumanData[idx].type == type)
        {
            break;
        }
        idx++;
    }
    human->turn = HumanData[idx].turn;
    human->width = HumanData[idx].width;
    human->height = HumanData[idx].height;
    if (HumanData[idx].mtbl->motion == 0)
    {
        SetupMotionRegist(HumanData[idx].mtbl);
    }
    human->motion = SetupMotionManager(human->model, HumanData[idx].mtbl);
    human->life = human->lifemax = HumanData[idx].life;

    idx = -1;
    /* (u16): the sltiu range test is in the bytes. */
    if ((u16)type >= N_PLAYABLE_CHARACTERS)
    {
        idtbl = StageAppearance[NowStage];
        idx = 0;
        while (idtbl[idx] != type)
        {
            if (idtbl[idx] == CHARACTER_KIND_END)
            {
                break;
            }
            idx++;
        }
    }
    /* VAB program 5 for player/partner (idx -1), then 6+ per stage. */
    human->sound = SOUND_PROGRAM_BASE(idx + 6);
    return human;
}
