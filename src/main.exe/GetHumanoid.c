#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * GetHumanoid(short type);
 *     HUMAN.C:334, 9 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a0       short type
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

/*
 * GetHumanoid (0x8002946c, 0x70 bytes) — linear-search the live HumanGroup[]
 * pointer array (length Humans) for the first entry whose `type` field
 * (item.h's Humanoid.type @0x00) matches `type`, returning that pointer (or
 * 0 if none). Same "Humanoid control" TU as
 * is_character_state_present_on_stage_.c/GetDirection.c/MoveHumanoid.c
 * (address-contiguous; is_character_state_present_on_stage_ sits immediately
 * after this function at 0x800294dc and shares HumanGroup/Humans).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The loop counter `i` is `short`, NOT `int` like the sibling
 *    is_character_state_present_on_stage_'s `s32 i` (which strength-reduces
 *    HumanGroup[i] to a walking +4 pointer). A short index instead
 *    recomputes the array byte-offset every iteration via a FUSED
 *    sign-extend+scale shift pair (`sll $v0,i,16` / `sra $v0,$v0,14`) —
 *    this is the natural single-shift result of a HImode induction variable
 *    scaled by a pointer's 4-byte stride, distinct from the GetPad-class
 *    "no natural C form" idiom (that one splits a PURE sign-extend across
 *    two sra's with no scale folded in at all; this one folds scale AND
 *    extend into one op, which plain `arr[short_i]` produces directly).
 *  - `type` (the short parameter) is sign-extended once, hoisted above the
 *    loop (compared every iteration against the loop-invariant value).
 *  - Standard entry-duplicated for-loop shape (cookbook Loops): the `i=0 <
 *    Humans` initial test is hoisted to the top (`Humans <= 0` -> return 0),
 *    the real per-iteration test rotates to the bottom.
 */

Humanoid *GetHumanoid(short type)
{
    Humanoid *p;
    short i;

    for (i = 0; i < Humans; i++)
    {
        p = HumanGroup[i];
        if (p->type == type)
        {
            return p;
        }
    }
    return 0;
}
