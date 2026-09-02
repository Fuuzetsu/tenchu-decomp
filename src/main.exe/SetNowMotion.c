#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SetNowMotion(struct Humanoid *human, short mid, short move);
 *     MOTION.C:188, 7 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short mid
 *     param $a2       short move
 * END PSX.SYM */

short SetNowMotion(Humanoid *human, motion_id mid, motion_move_mode move)
{
    if (human->status == STAT_DEAD &&
        human->motion->loop == MOTION_LOOP_DISABLED)
    {
        return 0;
    }
    if (UpdateMotion(human->motion, mid) == 0)
    {
        return 0;
    }
    human->status = (s8)MOTION_STATUS(mid);
    if (move != MOTION_MOVE_NONE)
    {
        MoveHumanoid(human, human->motion->motion->orderspd,
                     human->motion->motion->sidespd);
    }
    return 1;
}
