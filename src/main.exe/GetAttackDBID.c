#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetAttackDBID(struct Humanoid *human, short mid);
 *     APPEAR.C:258, 8 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short mid
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 * END PSX.SYM */

s16 GetAttackDBID(Humanoid *human, motion_id mid)
{
    s16 i;

    mid = GetMotionID(human->motion, mid);
    i = 0;
    while (BattleDB[i].mid != MOTION_ID_NONE)
    {
        if (BattleDB[i].mid == mid)
        {
            break;
        }
        i++;
    }
    return i;
}
