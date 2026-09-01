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
 * `StageEvent[i].id/.event/.next1/.next2` (the first 4 packed `u8` fields)
 * are read and compared to -1 as ONE `s32` — a `0xFFFFFFFF` terminator
 * sentinel across all four bytes at once, both for the initial "list
 * empty" guard and the loop's own exit test. Ghidra's per-byte
 * `._0_1_`/`._1_1_` rendering is exactly this union read; write it as a
 * direct `*(s32 *)&StageEvent[i]` cast, matching the raw `lw`+`-1` compare.
 *
 * The status/motion guard (`if (h->status==0x11 && h->motion->loop==-1)
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
 *  - A named byte offset keeps the short sign-extension/scale chain
 *    together before the first array base is materialized. Writing both
 *    pointer sums with the scaled integer first also selects the target's
 *    commutative `addu` operand order.
 *  - Initializing `i` before the empty-list sentinel makes its zero value
 *    fill that guard's delay slot; the cached event-slot pointer follows it.
 *  - The final life test deliberately reads the pointer slot through a
 *    volatile-qualified lvalue. This preserves the target's fresh slot
 *    reload and its load-delay `nop` instead of CSE-reusing the earlier
 *    Humanoid pointer.
 */
void UpdateEvent(short n, short id)
{
    EventSeqType *ev;
    s32 offset;
    short i;

    offset = (s16)n * 4;
    *(EventSeqType **)(offset + (s32)Event) = 0;
    if (id == EVENT_ID_NONE)
        return;
    i = 0;
    if (*(s32 *)&StageEvent[0] == EVENT_TABLE_END)
        return;

    do
    {
        /* Offset spelling is byte-required (&StageEvent[i] flips the
         * address addu to base-first; measured). */
        ev = (EventSeqType *)(i * 20 + (s32)StageEvent);
        if (ev->id == id)
        {
            Event[n] = ev;
            if (ev->target == EVENT_TARGET_PLAYER)
            {
                eTarget[n] = StagePlayer;
            }
            else
            {
                eTarget[n] = GetHumanoid(ev->target);
            }
            if (eTarget[n] != 0 &&
                !(eTarget[n]->status == STAT_DEAD &&
                  eTarget[n]->motion->loop == -1))
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
    } while (*(s32 *)&StageEvent[i] != EVENT_TABLE_END);
}
