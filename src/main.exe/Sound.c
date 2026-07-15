#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Sound(struct Humanoid *human, short seid);
 *     SEMNG.C:61, 8 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct Humanoid * human
 *     param $a1       short seid
 *     reg   $a0       struct VECTOR * locate
 *     reg   $a1       short seid
 *     reg   $s0       long volume
 *     reg   $s1       long zz
 *     reg   $s2       long xx
 *     reg   $a1       struct VECTOR * player
 *     reg   $a0       struct VECTOR * locate
 *     reg   $a1       short seid
 *     reg   $s0       long volume
 *     reg   $s1       long zz
 *     reg   $s2       long xx
 *     reg   $a1       struct VECTOR * player
 *
 * Globals it touches, as the original declared them:
 *     extern short VoiceMode;
 * END PSX.SYM */

/*
 * Sound (0x8004ff10) — play a character's sound effect. `seid` with any of the
 * high nibble set (0xf0) is an explicit id: play it at the character's position.
 * Otherwise it's a "category" (< 0x10): ids >= 6 are gated by a global mute
 * (VoiceMode) and the character's 0x80 attribute (e.g. dead/silent), and the
 * character's own default sound (Humanoid.sound) is OR'd in.
 *
 * Matching notes (this was a parked 5-byte NON_MATCHING; the fix needed BOTH
 * edits below — RTL story from cc1 -dl/-dg dumps):
 *  - TWO literal `return SoundEx(...)` calls, not one shared call with a
 *    `locate`/`seid` funnel. cc1 itself gives a promoted `short seid` param two
 *    pseudos — the raw SImode $a1 copy (read by the & 0xf0 test, the >5 compare
 *    and the first call's arg) and the HImode declared variable (the target's
 *    `move v1,a1` in the beqz delay slot, read only by the second call's `or`).
 *    A single shared call reads ONE variable in both arms, so its sign-extend
 *    reads $v1 on the if-path too (5-byte residual, permuter-immune at ~447k
 *    iterations). Cross-jump merges the two calls' identical `sll/sra/jal`
 *    tails back into one physical copy.
 *  - The second call's arg must be `(short)(seid | human->sound)`. Without the
 *    cast it is an int expression: the sign-extend chain lands BEFORE the ior,
 *    and sched1 then floats the `$a0 = human->locate` load above the sound
 *    load, defining $a0 while `human`'s pseudo is still live — that conflict
 *    evicts human off $a0 (`move v1,a0` at entry, everything repartitioned).
 *    The cast folds the truncation into the ior (or-then-extend, the target's
 *    shape); the deeper or-chain then wins sched1's priority race, the sound
 *    lhu schedules first, human dies at the $a0 load, and human/seid coalesce
 *    to $a0/$v1 exactly as the target.
 *  - SoundEx returns s16 here (item.h) — the result is Sound's return, so the
 *    `return -1` guards branch straight to the epilogue past its sign-extend.
 */
extern s16 VoiceMode;

short Sound(Humanoid *human, short seid)
{
    if (seid & 0xf0) {
        return SoundEx(human->locate, seid);
    }
    if (seid > 5) {
        if (VoiceMode != 0) {
            return -1;
        }
        if ((human->attribute & 0x80) != 0) {
            return -1;
        }
    }
    return SoundEx(human->locate, (short)(seid | human->sound));
}
