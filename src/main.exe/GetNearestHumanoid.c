#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * GetNearestHumanoid(struct Humanoid *human, short distance);
 *     HUMAN.C:408, 24 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * human
 *     param $a1       short distance
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — compiles 2 instructions (8 bytes, 93 vs the
 * original's 91) LONG. Root cause isolated to the dx/dz distance-squared
 * computation: the target issues the `dx*dx` MULT immediately after dx's
 * abs-value resolves (address 0xdc in the raw asm), long before dz's own
 * subtraction even starts, so its 2-cycle MULT/MFLO hazard is hidden by
 * dz's independent load/subtract/sign-test work with zero waste; our build
 * instead schedules that same MULT much later (only in the delay slot of
 * dz's OWN sign-test branch), leaving no independent work left to hide the
 * SECOND (dz*dz) MULT's hazard, so cc1 emits two bare `nop`s. This is a
 * scheduler priority/readiness decision, not a data or control-flow
 * difference (the VALUE computed is identical either way). Tried and
 * reverted, no effect on the schedule: an explicit `dxsq = dx*dx;`
 * statement placed right after dx's abs-value (worse — cc1 speculatively
 * duplicated the MULT into BOTH arms of the abs-value `if`, since nothing
 * then forced it to wait for the join); a separate `absdx`/`absdz` pair
 * re-reading their own destination for the negate (byte-identical to plain
 * dx/dz — cc1 already coalesces the trivial copy); swapping the addition
 * operand order (`dz*dz + dx*dx`); a `do { ...dx... } while (0);` fence
 * around just the dx computation. `tools/asmdiff.py --structural` under-
 * reports this residual (it silently truncates its comparison window at
 * the ORIGINAL 91-instruction size instead of noticing our extra 2 — a
 * tool gap worth fixing centrally); the raw byte count and a manual
 * objdump of the compiled .o are what actually caught it. The instruction
 * COUNT/LENGTH mismatch is real and NOT eligible for the cookbook's
 * same-length permuter-immune early-stop — it needs a genuine source fix
 * that hasn't been found yet.
 *
 * GetNearestHumanoid (0x80029864) — nearest other live, non-invisible,
 * non-boss-type, alive Humanoid to `human` within `distance`, or 0. Only
 * runs at all when `human` itself is at ground level (map.height == 0).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `human->map` (item.h's MapVector, byte-proven `level` elsewhere) has
 *    its `height` field (+0x4) exercised here for the FIRST time — a plain
 *    `lw`/`== 0` test, no cast needed.
 *  - The combined `if (human->map.height == 0 && 0 < Humans) { loop } return
 *    best;` (a single `&&`, NOT two early `return best;` guards) matches
 *    Ghidra's literal rendering directly — both short-circuit branches land
 *    on the SAME tail. An earlier two-early-return draft compiled the
 *    `best_dist = 20000;` assignment into the prologue instead of the first
 *    guard's delay slot (matched instruction-for-instruction once switched
 *    back to the combined form) — a real, sizeable regalloc/scheduling
 *    lever, not cosmetic.
 *  - Unlike GetHumanoid/KillHumanoid's `short i` array-index shape, this
 *    loop walks `HumanGroup` as an explicit `Humanoid **` pointer
 *    (`pp = HumanGroup; ...; pp++`) alongside a plain `int` counter used
 *    ONLY for the bound test — the two-touched-fields-per-element case
 *    still strength-reduces to a walking pointer since only ONE field
 *    (`*pp`) is read off it per iteration.
 *  - The five guard tests (`!= human`, `status != 0x11`, `attribute & 0x80
 *    == 0`, `type & 0xf0 != 0x90`, `life >= 0`) are a plain `&&` chain —
 *    every one branches to the SAME "next iteration" target, which a
 *    straight `if (A && B && C && D && E) {...}` reproduces directly (no
 *    De-Morgan inversion needed: this isn't an if/else, just a guarded
 *    body).
 *  - `type`/`attribute` (item.h, s16) are read `lhu` for their pure
 *    bitwise-mask tests (narrowing use, no field-type change); `status`/
 *    `life` are read `lh` (signed) for their direct compares, matching
 *    item.h's existing s16 declarations.
 */
extern s32 SquareRoot0(s32 val);

extern s16 Humans;
extern Humanoid *HumanGroup[];

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetNearestHumanoid", GetNearestHumanoid);
#else
Humanoid *GetNearestHumanoid(Humanoid *human, short distance)
{
    Humanoid *cur;
    Humanoid *best;
    Humanoid **pp;
    s32 dx, dz;
    s32 dist;
    s32 best_dist;
    int i;

    best = 0;
    best_dist = 20000;
    if (human->map.height == 0 && 0 < Humans)
    {
        i = 0;
        pp = HumanGroup;
        do
        {
            cur = *pp;
            if (cur != human && cur->status != 0x11 &&
                (cur->attribute & 0x80) == 0 &&
                (cur->type & 0xf0) != 0x90 &&
                -1 < cur->life)
            {
                dx = cur->locate->vx - human->locate->vx;
                if (dx < 0)
                {
                    dx = -dx;
                }
                dz = cur->locate->vz - human->locate->vz;
                if (dz < 0)
                {
                    dz = -dz;
                }
                dist = SquareRoot0(dx * dx + dz * dz);
                if (dist < distance && dist < best_dist)
                {
                    best_dist = dist;
                    best = cur;
                }
            }
            i = i + 1;
            pp = pp + 1;
        } while (i < Humans);
    }
    return best;
}
#endif /* NON_MATCHING */
