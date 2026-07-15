#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateEvent(short n, short id);
 *     STAGE.C:249, 18 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       short n
 *     param $a1       short id
 *     reg   $a3       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct EventSeqType *StageEvent;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 98 of 368 bytes differ, all downstream of a
 * SINGLE 2-instruction gap (see residual below); every earlier byte diff
 * is address drift from that gap, not a separate issue.
 *
 * UpdateEvent (0x8004e624, 0x170 bytes) — searches `StageEvent[]` (a
 * proven `EventSeqType[]`, item.h/reference/psxsym-types.h, stride 0x14)
 * for the entry matching `id`, resolving sequence slot `n`'s cached found
 * entry (`D_80097F78[n]`) and its target `Humanoid *` (`D_80097F80[n]`) —
 * both plain arrays (word-sized elements: pointer/pointer), indexed by the
 * short parameter `n` via the ordinary 2-instruction sign-extend+scale
 * (`sll 16`/`sra 14` = `*4`), ABSOLUTE in this TU (STAGE.C defines them —
 * SetupStageSequence.c, same TU, already lists them gp-relative for ITS
 * OWN references; per-TU gp-vs-absolute, not a symbol property).
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
 * shared `D_80097F78[n]=0;` clear whenever `range` is false, clearing state
 * the target actually PRESERVES): the raw asm's range test branches
 * STRAIGHT to the epilogue on failure, bypassing the clear entirely. It's
 * TWO independent early returns: `if (!range) return; if (life>0)
 * return;` — a real behavioral difference from the nested-if reading, not
 * just a scheduling artifact (verified: the nested-if draft clears
 * `D_80097F78[n]` on out-of-range `id`, the target does not).
 * `h->motion->loop` is item.h's `MotionManager.loop` @0x4 (a different
 * struct than Ghidra's raw `*(int*)+0x5c` pointer-then-offset-4 rendering
 * suggests by name).
 *
 * `(u16)(id - 2) < 2` (id==2 or id==3) recomputes `id - 2` FRESH on each
 * incoming path (the guard-taken path and the guard-skipped path both
 * materialize their own `addiu`) rather than sharing one register — plain
 * repeated inline `id - 2` reproduces this (no named temp).
 *
 * Residual (root-caused): the final `D_80097F80[n]->life` read
 * RE-LOADS the array slot's address-then-value (`lw` via the cached
 * array-base register, then a second `lw`+`lh`) in the target, while this
 * draft's cc1 recognizes `D_80097F80[n]` as unchanged since the earlier
 * null/status/motion checks (no intervening call) and reuses THEIR
 * already-loaded value register directly — 2 fewer instructions (no
 * reload, no filler nop in the now-empty delay slot). Tried and had no
 * effect: naming vs. not naming a `Humanoid *h` local for the repeated
 * `D_80097F80[n]` dereferences (cc1's cse recognizes the equivalence
 * either way). Likely a genuine register-pressure/live-range difference
 * between the two builds' surrounding code, not reachable from source
 * spelling alone.
 */
typedef struct
{
    u8 id;     /* 0x0 */
    u8 event;  /* 0x1 */
    u8 next1;  /* 0x2 */
    u8 next2;  /* 0x3 */
    u8 target; /* 0x4 */
    u8 mode;   /* 0x5 */
    s16 status; /* 0x6 */
    s16 x[2];  /* 0x8 */
    s16 y[2];  /* 0xC */
    s16 z[2];  /* 0x10 */
} EventSeqType; /* 0x14 */

extern EventSeqType *D_80097F78[];
extern Humanoid *D_80097F80[];
extern EventSeqType *StageEvent;
extern Humanoid *StagePlayer;

extern Humanoid *GetHumanoid(short type);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/UpdateEvent", UpdateEvent);
#else
void UpdateEvent(short n, short id)
{
    EventSeqType *ev;
    short i;

    D_80097F78[n] = 0;
    if (id == 0xFF)
        return;
    if (*(s32 *)&StageEvent[0] == -1)
        return;

    i = 0;
    do
    {
        ev = &StageEvent[i];
        if (ev->id == id)
        {
            D_80097F78[n] = ev;
            if (ev->target == 0xFF)
            {
                D_80097F80[n] = StagePlayer;
            }
            else
            {
                D_80097F80[n] = GetHumanoid(ev->target);
            }
            if (D_80097F80[n] != 0)
            {
                if (!(D_80097F80[n]->status == 0x11 && D_80097F80[n]->motion->loop == -1))
                {
                    if (!((u16)(id - 2) < 2))
                    {
                        return;
                    }
                    if (D_80097F80[n]->life > 0)
                    {
                        return;
                    }
                }
            }
            D_80097F78[n] = 0;
            return;
        }
        i = i + 1;
    } while (*(s32 *)&StageEvent[i] != -1);
}
#endif
