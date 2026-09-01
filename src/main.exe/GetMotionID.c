#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetMotionID(struct MotionManager *mmp, short mid);
 *     ACTION.C:169, 9 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionManager * mmp
 *     param $a1       short mid
 * END PSX.SYM */

/*
 * GetMotionID (0x8001c510, 0x74 bytes) — search mmp's registered-motion
 * table (MotionRegistType[], sentinel mid == MOTION_ID_NONE) for the row whose `mid`
 * matches the requested id and return that row's `id`; on falling through to
 * the sentinel without a match, returns the sentinel row's own `id` instead.
 * Same search-with-break-then-read-index shape as GetAttackDBID.c (a near-
 * twin, which itself calls this to resolve the character's current motion
 * before its own BattleDB search) — just reading a struct field instead of
 * the loop index at the end.
 */

short GetMotionID(MotionManager *mmp, motion_id mid)
{
    MotionRegistType *registrations;
    s16 i;

    registrations = mmp->motreg;
    i = 0;
    while (registrations[i].mid != MOTION_ID_NONE)
    {
        if (registrations[i].mid == mid)
        {
            break;
        }
        i++;
    }
    return registrations[i].id;
}
