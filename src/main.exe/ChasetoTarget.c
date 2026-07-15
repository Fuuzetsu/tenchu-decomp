#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ChasetoTarget(long length);
 *     THINK.C:269, 21 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $s5       long length
 *     reg   $s3       long xx
 *     reg   $s2       long zz
 *     reg   $s4       long * chase
 *     reg   $s3       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern long Distance;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — links 340 bytes vs the 328-byte target (3
 * instructions / 12 bytes LONG; 36 differing lines in the asmdiff, most of
 * them register-swap fallout from the one structural residual below).
 * Default build keeps the byte-identical INCLUDE_ASM stub.
 *
 * Matching notes (verified against the raw .s):
 *  - `chase = &Me_THINK_C->some_other_x_position;` is computed UNCONDITIONALLY
 *    at entry (before the target null check) — the asm's `addiu $s4,$s1,0x80`
 *    sits right after loading Me_THINK_C, ahead of the `beqz` guard.
 *  - Index 0 of the chase pair (`some_other_x_position`) is always addressed
 *    DIRECTLY off `Me_THINK_C` (struct-field syntax), never through the
 *    `chase` pointer; index 1 (`some_other_z_position`) is always addressed
 *    through `chase[1]` — both for the initial reads AND the later
 *    rand-reassignment stores. Two syntactically different address
 *    expressions for the "same" byte don't CSE across each other, so writing
 *    it this asymmetric way (matching the target exactly) is required — a
 *    uniform `Me_THINK_C->some_other_x_position`/`_z_position` pair or a
 *    uniform `chase[0]`/`chase[1]` pair both fail to reproduce the observed
 *    mixed s1-direct / s4-pointer addressing.
 *  - `Me_THINK_C` must be captured into a named local (`me`) at entry: the
 *    target keeps it resident in a callee-saved register across the
 *    rand()/rcos()/rsin() calls (its post-call store uses the SAME register
 *    loaded at function entry, never reloaded) — a call to an unprototyped
 *    external forces cc1 to conservatively reload any GLOBAL re-read after
 *    it, so persistence across the calls requires a real local variable
 *    copy, not repeated `Me_THINK_C->` field access.
 *  - The `(Attrib & 0xc000) != 0 || (chase0 == 0 && chase1 == 0)` guard
 *    reuses the SAME chase-field expressions already evaluated for xx/zz
 *    (confirmed: the guard's OR operands sit in the same registers ($a2/$a1)
 *    that held the xx/zz computation's chase reads, i.e. cc1 CSE'd them —
 *    only happens because the guard spells the field accesses identically,
 *    and only produces a single `or`+`bnez` when written as a REAL bitwise
 *    OR test `(c0 | c1) == 0` — the logical `c0 == 0 && c1 == 0` spelling
 *    compiles to two separate reload+branch pairs instead, confirmed against
 *    Think4contact.c's own `some_other_x_position == 0 &&
 *    some_other_z_position == 0` guard, which is NOT OR-combined).
 *  - The abs-clamp pair (`if (xx<0) t=-xx; if (t<500) { if (zz<0) t=-zz; if
 *    (t<500) return 0; } }`) is a single reused temp `t`, not two named abs
 *    locals — PSX.SYM's 8 recorded locals don't include one, and the asm's
 *    abs computation lands in the same scratch register ($v0) both times.
 *
 * RESIDUAL (12 bytes / 3 instructions long, a delay-slot-fill tie): the
 * null-check guard (`if (me->another_camera_related_perhaps != 0) {...}`)
 * needs `move $v0,zero` (the branch's own return-0 setup) in its delay
 * slot, duplicated from the shared epilogue — every draft tried here
 * instead has reorg steal a data-independent, unconditionally-needed
 * instruction from just before the branch (the `chase` pointer's `addiu`,
 * or the `length` parameter's register copy, whichever is textually
 * nearest) to fill the slot, which forces a SEPARATE small tail (`j` +
 * `move v0,zero`) to reach the real epilogue instead of branching straight
 * to it like target. Tried: computing `chase` before/after the null check;
 * naming the target pointer as its own local; wrapping the whole body in
 * `if (target != 0) {...} return 0;` instead of an early `goto`/`return`.
 * All byte-identical to each other in this respect — this is the cookbook's
 * NAMED "guard delay-slot fill tie" class (a branch's own return-value
 * setup vs. the fallthrough's first data-independent instruction — see
 * StickonCheck). `tools/permute.py ChasetoTarget -- --stop-on-zero -j4`,
 * one bounded run (~25000 iterations, best score 1010 vs base 1345, never
 * near zero — though note the permuter's own scorer is unreliable here
 * since the function is still the WRONG LENGTH, so its score doesn't track
 * true residual bytes). Parked per the cookbook's attempt-cap /
 * sub-C-level-residual guidance; the field layout, struct types, dispatch
 * shape, and store order above are all independently confirmed correct.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ChasetoTarget", ChasetoTarget);
#else
extern short Attrib;
extern long Distance;
extern int rand(void);
extern s32 rcos(s32 a);
extern s32 rsin(s32 a);

short ChasetoTarget(long length)
{
    character_state *me;
    long xx, zz;
    long *chase;
    long c0, c1;
    long vx, vz;
    short deg;
    long t;

    me = Me_THINK_C;
    chase = &me->some_other_x_position;
    if (me->another_camera_related_perhaps != 0)
    {
        c0 = me->some_other_x_position;
        xx = me->another_camera_related_perhaps->position.x + c0
             - me->some_kind_of_current_position->vx;
        c1 = chase[1];
        zz = me->another_camera_related_perhaps->position.z + c1
             - me->some_kind_of_current_position->vz;

        t = xx;
        if (xx < 0)
        {
            t = -xx;
        }
        if (t < 500)
        {
            t = zz;
            if (zz < 0)
            {
                t = -zz;
            }
            if (t < 500)
            {
                goto ret0;
            }
        }

        if ((Attrib & 0x400) == 0 && 999 < Distance)
        {
            if ((Attrib & 0xc000) != 0 || (c0 | c1) == 0)
            {
                deg = rand();
                vx = rcos(deg) * length >> 12;
                me->some_other_x_position = vx;
                vz = rsin(deg) * length >> 12;
                chase[1] = vz;
            }
            return turn_towards_player_(xx, zz);
        }
    }
ret0:
    return 0;
}
#endif
