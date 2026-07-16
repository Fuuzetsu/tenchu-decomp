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
 * instructions / 12 bytes LONG; only 10 differing asmdiff lines, ALL of them
 * from the three reorg delay-slot residuals below — register allocation now
 * matches the target exactly). Default build keeps the byte-identical
 * INCLUDE_ASM stub.
 *
 * PROGRESS (this session, from 36 differing lines -> 10): the whole register
 * allocation (me=$s1, xx=$s3, zz=$s2, chase=$s4, c0=$a2, c1=$a1-reused, the
 * abs temp in $v0) now matches the target. Three source rewrites did it,
 * each verified against the demo build (siblingdiff --demo) as the structure
 * oracle and the .greg allocation-order dump:
 *   1. The abs clamp is written INLINE in the comparison
 *      (`if ((xx >= 0 ? xx : -xx) < 500)`), NOT as `t = xx; if (xx<0) t=-xx;`.
 *      The named-temp form (a) keeps `t` scheduled early into $a1 instead of
 *      the target's $v0 (its live range overlaps $v0's zz use), and (b) its
 *      neg reads zz (`t = -zz`) — 5 zz refs vs the target's 4 — which flips
 *      the .greg allocno priority so zz beats me for $s1 (me lands in $s2).
 *      Inlining drops the temp, lands the abs in $v0, and gives zz 4 refs so
 *      me wins $s1. (A statement `t = -t` does NOT help: cse copy-propagates
 *      it back to `t = -zz`; only the inline `?:` / an unnamed abs keeps the
 *      neg off zz. This is a NEW reusable rule — see report.)
 *   2. c1 (`chase[1]`) is INLINED into the zz expression, not read into a
 *      named `c1` before zz. A separate `c1 = chase[1];` load schedules early
 *      (into $a2/$a3), before the target-ptr $a1 dies at `position.z`; inline,
 *      it loads during zz AFTER $a1 is free, so it reuses $a1 exactly like the
 *      target. c0 stays a named temp (target loads it distinctly into $a2).
 *   3. Me_THINK_C captured into `me` (unchanged, still required — see below).
 *
 * Matching notes (verified against the raw .s):
 *  - `chase = &Me_THINK_C->some_other_x_position;` is computed at entry,
 *    before the target null check (asm's `addiu $s4,$s1,0x80` sits ahead of
 *    the `beqz`). Index 0 (`some_other_x_position`) is always addressed
 *    DIRECTLY off `Me_THINK_C`; index 1 through `chase[1]` ($s4+4). The two
 *    spellings don't CSE, so this asymmetry is required.
 *  - `Me_THINK_C` captured into a named local (`me`): the target keeps it in
 *    a callee-saved reg across rand()/rcos()/rsin() (post-call store reuses
 *    the entry-loaded reg), which needs a real local copy.
 *  - The guard `(Attrib & 0xc000) != 0 || (c0 | chase[1]) == 0` must be a
 *    REAL bitwise-OR test `(c0 | chase[1]) == 0` (one `or`+`bnez`); the
 *    logical `c0==0 && c1==0` spelling compiles to two reload+branch pairs.
 *
 * RESIDUAL (12 bytes / 3 instructions long, ALL reorg delay-slot fills; the
 * source structure, layout, and register allocation are byte-confirmed
 * correct):
 *   (a) The null-check guard `beqz $a1` gets the `chase` `addiu` in its delay
 *       slot instead of the target's `move $v0,zero`. cc1's reorg
 *       fill_simple_delay_slots does a BACKWARD scan and steals `chase` (an
 *       eligible, independent insn right before the branch) before
 *       fill_eager_delay_slots can duplicate the ret0 `move $v0,zero` in.
 *       This forces a `nop` where the `addiu` was (extra #1) and a separate
 *       `j`+`move $v0,zero` tail island (extra #2+relocated move). The other
 *       THREE return-0 branches (abs-zz, Attrib, Distance) all correctly get
 *       `move $v0,zero` because their predecessor sets the branch condition
 *       ($v0), leaving nothing for fill_simple to steal. The guard is the
 *       only branch with an independent stealable predecessor. Confirmed via
 *       reorg.c: chase is `length 1, dslot no` so eligible; it sets $s4 which
 *       is not in the branch's `needed` set, so nothing vetoes the steal.
 *       Moving chase into the body just makes reorg steal the `length`
 *       parameter copy instead (still a `nop`); the demo AND retail both keep
 *       chase before the branch WITH move-v0 in the slot, so it is reachable,
 *       but no C spelling found here reproduces which insn reorg elects.
 *   (b) The `(Attrib & 0xc000)` branch's delay slot is a `nop` (extra #3);
 *       the target puts the `or $v0,$a2,$a1` (the c0|c1 computation, the
 *       fallthrough's first insn) there. fill_eager does not steal it here.
 *
 * NOT a register tie anymore: the 10 differing lines are purely these 3
 * reorg fills plus their address shift. Permuter is inapplicable (refuses a
 * >1-instruction length delta). The residual is a genuine sub-C-level reorg
 * delay-slot election; the demo build proves the target shape is what the
 * original source produced.
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
    long c0;
    long vx, vz;
    short deg;

    me = Me_THINK_C;
    chase = &me->some_other_x_position;
    if (me->another_camera_related_perhaps != 0)
    {
        c0 = me->some_other_x_position;
        xx = me->another_camera_related_perhaps->position.x + c0
             - me->some_kind_of_current_position->vx;
        zz = me->another_camera_related_perhaps->position.z + chase[1]
             - me->some_kind_of_current_position->vz;

        if ((xx >= 0 ? xx : -xx) < 500)
        {
            if ((zz >= 0 ? zz : -zz) < 500)
            {
                goto ret0;
            }
        }

        if ((Attrib & 0x400) == 0 && 999 < Distance)
        {
            if ((Attrib & 0xc000) != 0 || (c0 | chase[1]) == 0)
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
