#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1watch(void);
 *     THINK_1.C:50, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * actflg (0x2000 if set, else -0x8000), and once actscnt exceeds 10 it
 * re-rolls actflg via `rand() & 1`, resets actscnt, and advances actcnt into
 * the next 128-frame cycle. Confirms game_types.h's `actflg` (previously an
 * unnamed placeholder, field53_0x89): Ghidra's own independent decompilation
 * of THIS function names the same offset `actflg` (tested `!= 0`, later
 * assigned `rand() & 1`), matching Ghidra's own Humanoid struct in
 * reference/ghidra_types.h (its actmode/actflg/actcnt/actscnt run).
 *
 * `actcnt` is a real local: the else branch's `actcnt + 1` store reuses the
 * SAME register the entry test read (no reload of the global in between).
 * `old_actscnt` mirrors Think1trace's idiom: `actscnt + 1` stores back
 * unconditionally before the `> 10` test reads the OLD value. `rand()` takes
 * no argument — the asm's $a0 still holds Me_THINK_C (unclobbered since
 * entry) at the call site, which m2c mis-renders as an argument (the
 * cookbook's m2c-over-counts-call-args tell); every other proven call site
 * (item.h, ReqItemHappou.c) declares `rand(void)`.
 */
extern int rand(void);

s16 Think1watch(void)
{
    u8 actcnt;
    u8 old_actscnt;
    s16 result;

    actcnt = Me_THINK_C->actcnt;
    result = 0;
    if ((actcnt & 0x7F) == 0)
    {
        result = -0x8000;
        if (Me_THINK_C->actflg != 0)
        {
            result = 0x2000;
        }
        old_actscnt = Me_THINK_C->actscnt;
        Me_THINK_C->actscnt = old_actscnt + 1;
        if (old_actscnt > 10)
        {
            Me_THINK_C->actflg = rand() & 1;
            Me_THINK_C->actscnt = 0;
            Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
        }
    }
    else
    {
        Me_THINK_C->actcnt = actcnt + 1;
    }
    return result;
}
