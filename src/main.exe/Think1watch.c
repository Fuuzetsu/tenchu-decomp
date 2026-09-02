#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1watch(void);
 *     THINK_1.C:50, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $s0       short pad
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

/*
 * Think1watch (0x8002f8e8, 0xb0 bytes) — think-handler, same "think" TU as
 * Think1trace.c/Think1sleep.c. The body only runs every 0x80 (128) frames
 * (gated by `actcnt & 0x7f`); while gated it returns a turn signal from
 * actflg (right if set, otherwise left), and once actscnt exceeds 10 it
 * re-rolls actflg via `rand() & 1`, resets actscnt, and advances actcnt into
 * the next 128-frame cycle. Confirms game_types.h's `actflg` (previously an
 * unnamed placeholder, field53_0x89): Ghidra's own independent decompilation
 * of THIS function names the same offset `actflg` (tested `!= 0`, later
 * assigned `rand() & 1`), matching Ghidra's own Humanoid struct in
 * reference/ghidra_types.h (its actmode/actflg/actcnt/actscnt run).
 *
 * The direct actcnt field test and the actscnt post-increment retain the
 * target's byte loads and old-value comparison without staging locals.
 * `rand()` takes
 * no argument — the asm's $a0 still holds Me_THINK_C (unclobbered since
 * entry) at the call site, which m2c mis-renders as an argument (the
 * cookbook's m2c-over-counts-call-args tell); every other proven call site
 * (item.h, ReqItemHappou.c) declares `rand(void)`.
 */
extern int rand(void);

s16 Think1watch(void)
{
    s16 pad;

    UPDATE_IDLE_LOOK_PAD(pad);
    return pad;
}
