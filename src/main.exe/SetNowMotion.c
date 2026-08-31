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

/*
 * SetNowMotion (0x80026f54) — start motion `mid` on a character, unless it's
 * already in the locked state 0x11 running a non-looping (loop == -1) motion.
 * On a successful UpdateMotion, latch the motion's high byte as the new status
 * and (when `move`) apply the motion's default order/side speed via MoveHumanoid.
 *
 * Matching notes:
 *  - `mid` is kept as `mid << 16` in $s0 across the UpdateMotion call and used
 *    twice: `sra ,0x10` = (short)mid for the call arg, `sra ,0x18` =
 *    (char)(mid >> 8) for the status latch.
 *  - The guard is a real `||` with a comma: `status != 0x11 || (ret = 0,
 *    motion->loop != -1)` — ret is zeroed in the loop-test's branch delay slot.
 *  - UpdateMotion returns s16 here (item.h): its result is `ret`, tested short.
 */
short SetNowMotion(Humanoid *human, short mid, short move)
{
    MotionDataType *md;

    if (human->status == STAT_DEAD && human->motion->loop == -1)
    {
        return 0;
    }
    if (UpdateMotion(human->motion, mid) == 0)
    {
        return 0;
    }
    human->status = (s8)(mid >> 8);
    if (move != 0)
    {
        md = human->motion->motion;
        MoveHumanoid(human, md->orderspd, md->sidespd);
    }
    return 1;
}
