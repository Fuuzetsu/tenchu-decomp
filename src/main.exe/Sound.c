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
 * END PSX.SYM */

/*
 * Sound (0x8004fef8) — play a character's sound effect. `seid` with any of the
 * high nibble set (0xf0) is an explicit id: play it at the character's position.
 * Otherwise it's a "category" (< 0x10): ids >= 6 are gated by a global mute
 * (D_80097CB4) and the character's 0x80 attribute (e.g. dead/silent), and the
 * character's own default sound (Humanoid.sound) is OR'd in.
 *
 * Matching notes:
 *  - Written `if (seid & 0xf0) {simple} else {gated}` (opposite polarity to
 *    Ghidra's `== 0`): the beqz branches to the gated body and the simple body
 *    falls through with a `j` to the shared SoundEx call.
 *  - `seid` is a short: the >5 test and each arg pass sign-extend it (sll/sra).
 *  - SoundEx returns s16 here (item.h) — the result is Sound's return, so the
 *    `return -1` paths share its sign-extend tail (cross-jump merge).
 *
 * STATUS: NON_MATCHING — 5 bytes (a pure register-allocation tie). The draft is
 * arithmetically correct and the right length; only the `seid | human->sound`
 * else-branch differs: the target loads `human->sound` into the arg reg $a1 and
 * ORs `seid` (kept in $v1) into it (`lhu a1; or a1,v1,a1`), whereas cc1 here
 * loads sound into $v0 and reassigns `seid`/$v1 (`lhu v0; or v1,v1,v0; sll
 * a1,v1`). Every source form that avoids reassigning `seid` (inline arg, separate
 * temp, two shared-tail returns) makes cc1 repartition $a0/$a1 wholesale (worse).
 * A decomp-permuter run of ~447k iterations never beat the score-25 base, so the
 * swap is below the C level (a scheduler/allocator tie source can't steer here).
 */
extern s16 D_80097CB4;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Sound", Sound);
#else
short Sound(Humanoid *human, short seid)
{
    VECTOR *locate;

    if (seid & 0xf0) {
        locate = human->locate;
    } else {
        if (seid > 5) {
            if (D_80097CB4 != 0) {
                return -1;
            }
            if ((human->attribute & 0x80) != 0) {
                return -1;
            }
        }
        locate = human->locate;
        seid = seid | human->sound;
    }
    return SoundEx(locate, seid);
}
#endif
