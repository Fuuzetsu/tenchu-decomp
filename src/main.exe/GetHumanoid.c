#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * GetHumanoid(short type);
 *     HUMAN.C:334, 9 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short type
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

Humanoid *GetHumanoid(character_kind type)
{
    short i;

    for (i = 0; i < Humans; i++)
    {
        if (HumanGroup[i]->type == type)
        {
            return HumanGroup[i];
        }
    }
    return 0;
}
