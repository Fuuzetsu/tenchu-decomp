#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "stage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateEvent(short n, short id);
 *     STAGE.C:249, 18 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short n
 *     param $a1       short id
 *     reg   $a3       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct EventSeqType *Event[2];
 *     extern struct EventSeqType *StageEvent;
 *     extern struct Humanoid *eTarget[2];
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

void UpdateEvent(short n, short id)
{
    short i;

    Event[n] = 0;
    if (id == EVENT_ID_NONE)
        return;
    i = 0;
    if (StageEvent[0].header.word == EVENT_TABLE_END)
        return;

    do
    {
        if (StageEvent[i].header.route.id == id)
        {
            Event[n] = &StageEvent[i];
            if (StageEvent[i].target == EVENT_TARGET_PLAYER)
            {
                eTarget[n] = StagePlayer;
            }
            else
            {
                eTarget[n] = GetHumanoid(StageEvent[i].target);
            }
            {
                Humanoid *target;

                target = eTarget[n];
                if (target == 0 ||
                    (target->status == STAT_DEAD &&
                     target->motion->loop == MOTION_LOOP_DISABLED))
                {
                    Event[n] = 0;
                    return;
                }
            }
            if ((u16)(id - EVENT_ROOT_FIRST) >= N_STAGE_EVENT_SLOTS ||
                eTarget[n]->life > 0)
            {
                return;
            }
            Event[n] = 0;
            return;
        }
        i++;
    } while (StageEvent[i].header.word != EVENT_TABLE_END);
}
