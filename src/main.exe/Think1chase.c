#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1chase(void);
 *     THINK_1.C:119, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     reg   $a0       struct Humanoid * enemy
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s3       short pad
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

/*
 * Think1chase (0x8002c39c, 0x178 bytes) — think-handler, same "think" TU as
 * Think1random.c/Think1sleep.c. On the first tick of a new cycle (actcnt
 * rolls to 1), picks the chase target: the nearest other Humanoid within
 * 5000 units if one exists, else the same random-offset-from-spawn roll as
 * Think1random. On later ticks, steers towards the chase target via
 * turn_towards_player_; when it returns 0 (facing the target already),
 * resets actcnt to 0 and forces the result to 0x80 instead.
 *
 * GetNearestHumanoid uses the shared `Humanoid *` view, matching this TU's
 * `Me_THINK_C` and the character APIs in humanoid.h.
 *
 * The null check reads OPPOSITE of Ghidra's literal `if (enemy == 0)
 * {random} else {real}` rendering: the asm's `beqz` branches to the
 * random-roll block and FALLS THROUGH to the enemy-based assignment, i.e.
 * the real source is `if (enemy != 0) {real} else {random}` — the
 * "branch-if-equal into a later block, opposite polarity" cookbook rule
 * (not the null-guard-with-two-returns exception: this is a plain
 * side-effecting if/else with a shared join, not two returns).
 *
 * `result` (Ghidra's `sVar2`) is a WIDE s32 local even though the function
 * returns `s16`: the call result is `move`d straight from $v0 with no
 * immediate sll/sra, and the SAME register also gets `ori $s1,$s1,0x80` in
 * the guard branch's delay slot (the "reset" block's own first statement,
 * hoisted) — only ONE truncation happens, at the shared `return result;`
 * (same "Ghidra's short-typed call-result variable can be int in source"
 * rule as ThinkBasicHuman1's pad).
 */
extern Humanoid *GetNearestHumanoid(Humanoid *human, s16 distance);

s16 Think1chase(void)
{
    u8 actcnt;
    s32 result;

    actcnt = Me_THINK_C->actcnt + 1;
    Me_THINK_C->actcnt = actcnt;
    result = 0;
    if (actcnt == 1)
    {
        Humanoid *enemy;

        enemy = GetNearestHumanoid(Me_THINK_C, 5000);
        if (enemy != 0)
        {
            Me_THINK_C->chase[0] = enemy->locate->vx;
            Me_THINK_C->chase[1] = enemy->locate->vz;
        }
        else
        {
            Me_THINK_C->chase[0] = Me_THINK_C->point[0] + rand() % 10000 - 5000;
            Me_THINK_C->chase[1] = Me_THINK_C->point[1] + rand() % 10000 - 5000;
        }
    }
    else
    {
        result = turn_towards_player_(Me_THINK_C->chase[0] - Me_THINK_C->locate->vx,
                                       Me_THINK_C->chase[1] - Me_THINK_C->locate->vz);
        if ((s16)result == 0)
        {
            result |= 0x80;
            Me_THINK_C->actcnt = 0;
        }
    }
    return result;
}
