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

/*
 * STATUS: MATCHING — 368 bytes.
 *
 * UpdateEvent (0x8004e624, 0x170 bytes) — searches `StageEvent[]` (a
 * proven `EventSeqType[]`, item.h/reference/psxsym-types.h, stride 0x14)
 * for the entry matching `id`, resolving sequence slot `n`'s cached found
 * entry (`Event[n]`) and its target `Humanoid *` (`eTarget[n]`) —
 * both plain arrays (word-sized elements: pointer/pointer), indexed by the
 * short parameter `n` via the ordinary 2-instruction sign-extend+scale
 * (`sll 16`/`sra 14` = `*4`), ABSOLUTE in this reconstructed TU.
 * StartStageSequence.c and StageSequence.c need gp-relative views for their
 * own references; gp-vs-absolute is a per-object codegen choice, not a
 * property of the shared symbol.
 *
 * EventHeader overlays `id/event/next1/next2` with the table-marker word.
 * The initial empty-list guard and loop exit can therefore use `header.word`
 * for the target's single `lw`/`-1` comparison without an aliasing cast.
 *
 * A scoped `target` snapshot handles the null target and the completed-death
 * motion as one clear-and-return condition. The subsequent range/life guard
 * preserves the event when the id is outside the root slots or the current
 * target is still alive. This distinction is behavioral: nesting the life
 * test under an in-range check would clear an out-of-range event that retail
 * preserves. `target->motion->loop` is item.h's `MotionManager.loop` @0x4,
 * not the raw pointer-plus-offset shape suggested by the decompiler.
 *
 * The inline EVENT_ROOT_FIRST..EVENT_ROOT_LAST range expression is duplicated
 * on the guard's two incoming paths, so each path materializes its own
 * `id - EVENT_ROOT_FIRST` rather than sharing a named temporary.
 *
 * Matching notes:
 *  - Both tables are accessed directly as `Event[n]` and `StageEvent[i]`.
 *    CSE and loop strength reduction create the cached addresses visible in
 *    the target; neither the byte offset nor the event cursor is a source
 *    local (matching PSX.SYM's declaration list).
 *  - Initializing `i` before the empty-list sentinel makes its zero value
 *    fill that guard's delay slot; the cached event-slot pointer follows it.
 *  - Ending the guard snapshot's scope before the range/life check makes the
 *    latter an authoritative `eTarget[n]` read. The structured early clear
 *    gives retail's fresh slot reload and load-delay `nop` without volatile.
 */
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
