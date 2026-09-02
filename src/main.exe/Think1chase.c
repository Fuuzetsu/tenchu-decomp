#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1chase(void);
 *     THINK_1.C:119, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * Think1random.c/Think1sleep.c. On the first tick of a new cycle (++Me_THINK_C->actcnt
 * rolls to 1), picks the chase target: the nearest other Humanoid within
 * 5000 units if one exists, else the same random-offset-from-spawn roll as
 * Think1random. On later ticks, steers towards the chase target via
 * GotoPosition; when it returns 0 (facing the target already),
 * resets ++Me_THINK_C->actcnt to 0 and adds the Square attack button.
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
 * `pad` (Ghidra's `sVar2`) is a WIDE s32 local even though the function
 * returns `s16`: the call result is `move`d straight from $v0 with no
 * immediate sll/sra, and the SAME register also gets `ori $s1,$s1,0x80` in
 * the guard branch's delay slot (the "reset" block's own first statement,
 * hoisted) — only ONE truncation happens, at the shared `return pad;`
 * (same "Ghidra's short-typed call-result variable can be int in source"
 * rule as ThinkBasicHuman1's pad).
 */
s16 Think1chase(void)
{
    s32 pad;
    pad = 0;
    if (++Me_THINK_C->actcnt == 1)
    {
        Humanoid *enemy;

        enemy = GetNearestHumanoid(Me_THINK_C, 5000);
        if (enemy != 0)
        {
            Me_THINK_C->chase.point[HUMANOID_CHASE_X] = enemy->locate->vx;
            Me_THINK_C->chase.point[HUMANOID_CHASE_Z] = enemy->locate->vz;
        }
        else
        {
            Me_THINK_C->chase.point[HUMANOID_CHASE_X] = Me_THINK_C->point[HUMANOID_HOME_X] + rand() % 10000 - 5000;
            Me_THINK_C->chase.point[HUMANOID_CHASE_Z] = Me_THINK_C->point[HUMANOID_HOME_Z] + rand() % 10000 - 5000;
        }
    }
    else
    {
        pad = GotoPosition(Me_THINK_C->chase.point[HUMANOID_CHASE_X] - Me_THINK_C->locate->vx,
                                  Me_THINK_C->chase.point[HUMANOID_CHASE_Z] - Me_THINK_C->locate->vz);
        if ((s16)pad == 0)
        {
            pad |= PADRleft;
            Me_THINK_C->actcnt = 0;
        }
    }
    return pad;
}
