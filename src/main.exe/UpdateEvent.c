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
 * The status/motion guard (`if (h->status==STAT_DEAD &&
 * h->motion->loop==MOTION_LOOP_DISABLED)
 * goto clear;`) bypasses the `id`/`life` check entirely when true — but
 * the `id`/`life` check itself is NOT a single nested
 * `if (range) { if (life>0) return; }` (that shape falls through to the
 * shared `Event[n]=0;` clear whenever `range` is false, clearing state
 * the target actually PRESERVES): the raw asm's range test branches
 * STRAIGHT to the epilogue on failure, bypassing the clear entirely. One
 * short-circuit return condition (`!range || life > 0`) emits those same
 * two independent machine guards — a real behavioral difference from the nested-if reading, not
 * just a scheduling artifact (verified: the nested-if draft clears
 * `Event[n]` on out-of-range `id`, the target does not).
 * `h->motion->loop` is item.h's `MotionManager.loop` @0x4 (a different
 * struct than Ghidra's raw `*(int*)+0x5c` pointer-then-offset-4 rendering
 * suggests by name).
 *
 * The EVENT_ROOT_FIRST..EVENT_ROOT_LAST range check recomputes
 * `id - EVENT_ROOT_FIRST` FRESH on each incoming path (the guard-taken path
 * and the guard-skipped path both materialize their own `addiu`) rather than
 * sharing one register — plain repeated inline subtraction reproduces this
 * (no named temp).
 *
 * Matching notes:
 *  - Both tables are accessed directly as `Event[n]` and `StageEvent[i]`.
 *    CSE and loop strength reduction create the cached addresses visible in
 *    the target; neither the byte offset nor the event cursor is a source
 *    local (matching PSX.SYM's declaration list).
 *  - Initializing `i` before the empty-list sentinel makes its zero value
 *    fill that guard's delay slot; the cached event-slot pointer follows it.
 *  - The final life test deliberately reads the pointer slot through a
 *    volatile-qualified lvalue. This preserves the target's fresh slot
 *    reload and its load-delay `nop` instead of CSE-reusing the earlier
 *    Humanoid pointer.
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
            if (eTarget[n] != 0 &&
                !(eTarget[n]->status == STAT_DEAD &&
                  eTarget[n]->motion->loop == MOTION_LOOP_DISABLED))
            {
                if ((u16)(id - EVENT_ROOT_FIRST) >= N_STAGE_EVENT_SLOTS ||
                    (*(Humanoid *volatile *)&eTarget[n])->life > 0)
                {
                    return;
                }
            }
            Event[n] = 0;
            return;
        }
        i++;
    } while (StageEvent[i].header.word != EVENT_TABLE_END);
}
