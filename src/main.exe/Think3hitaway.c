#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"
#include "humanoid.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3hitaway(void);
 *     THINK_3.C:172, 33 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     reg   $a1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short (*AttackFunc[4])();
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;

extern s32 rand(void);
extern short ChasetoTarget(long length);
extern short SuccessionAttack(long dist, short deg);
extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);

/*
 * Think3hitaway (0x8002d984) — think-handler, same "think" TU as
 * Think3chase.c/Think3escape.c (s16 return convention; gp-relative
 * Distance/SR/Degree/Attrib — see gpsyms).
 *
 * If close (Distance < 10000) and not already in the "-2" SR state, clear
 * SR. If the character just got hit (character_status == 7): clear actflg,
 * zero out `some_other_x_position`/`some_other_z_position` (Ghidra's
 * `chase[0]`/`chase[1]`), and SuccessionAttack(3000, 1500) for the result.
 * Else if not already acting (actflg == 0): face the player if aim is
 * close (abs(Degree) < 1000, via turn_towards_player_ OR'd with a walk/run
 * hint) or ChasetoTarget(5000) otherwise; roll a 1-in-30 chance (Distance <
 * 2000) to add a "hit" flag (0x40); arm actflg once far enough away
 * (Distance > 4000) or once Attrib bit 0x400 is set. Else (already
 * acting): dispatch through AttackFunc[wpatk>>4]() — same
 * zero-argument indirect-call idiom as Think3chase.c.
 *
 * Matching notes:
 *  - `character_status` is read SIGNED (`lh`) at this one call site even
 *    though game_types.h proves the field itself `u16` (needed elsewhere
 *    to avoid a bad sign-extend) — reached via an offset-cast read,
 *    `*(s16 *)&Me_THINK_C->status`, matching the cookbook's
 *    "reach a divergent-width access via an offset cast off the same
 *    proven pointer" rule, rather than retyping the shared struct field.
 *  - The AttackFunc dispatch (SHORT body, one call + return) must be the
 *    SECOND arm (`else if (actflg != 0)`) and the long "not yet acting"
 *    body LAST/else — Ghidra's own textual order (`else if (actflg==0)
 *    {long body} else {dispatch}`) is the OPPOSITE polarity and cost 220+
 *    bytes of misalignment; this is the general "put the short body
 *    earlier, the long body last so it can fall into the shared epilogue"
 *    lever, not specific to this function.
 *  - The hit-status arm returns SuccessionAttack directly. This leaves its
 *    result in $v0 and lets the jump delay slot start the s16 conversion;
 *    assigning it to the shared result would add an unnecessary $s0 copy
 *    and move the conversion below the epilogue restores.
 */
s16 Think3hitaway(void)
{
    u16 result;
    s32 degree;

    if (Distance < 10000 && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if (*(s16 *)&Me_THINK_C->status == STAT_ATTACK)
    {
        Me_THINK_C->actflg = 0;
        Me_THINK_C->chase[1] = 0;
        Me_THINK_C->chase[0] = 0;
        return SuccessionAttack(3000, 0x5dc);
    }
    else if (Me_THINK_C->actflg != 0)
    {
        result = AttackFunc[Me_THINK_C->wpatk >> 4]();
    }
    else
    {
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 1000)
        {
            result = turn_towards_player_(0, 0);
            result = (result & 0xe000) | 0x4000;
        }
        else
        {
            result = ChasetoTarget(5000);
        }
        if (Distance < 2000)
        {
            s32 r;

            r = rand();
            if (r == (r / 30) * 30)
            {
                result |= 0x40;
            }
        }
        if (Distance > 4000 || (ATTRIB_BITS & ATTR_WALL))
        {
            Me_THINK_C->actflg = 1;
        }
    }
    return result;
}
