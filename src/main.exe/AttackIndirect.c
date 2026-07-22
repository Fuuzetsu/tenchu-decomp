#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackIndirect(void);
 *     THINK_3.C:525, 54 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern short SR;
 * END PSX.SYM */

/*
 * AttackIndirect (0x8002ee20, 0x350 bytes) — indirect/ranged attack chooser.
 * Status 7 waits for the current BattleDB continuation frame and then rolls
 * an EngageLevel-gated attack; status 9 does nothing.  The ordinary path
 * clears a stale search result, turns or issues PAD commands according to
 * range/facing, and applies a small rotation correction for the 0x80 result.
 *
 * Matching notes:
 *  - `pad` is the original s16 local.  `status7_result` is a distinct s16
 *    return island: keeping its three edge assignments separate produces the
 *    target's moves into $v0 before one shared sign-extension tail.
 *  - The one-shot `do` encloses a CONTIGUOUS RANGE of three statements, not
 *    just one `if`.  Its loop notes stop cse/local-copy propagation from
 *    replacing `status7_result = 0` with a copy of the already-zero `$s0`.
 *    A bounded permuter found this final one-byte fix.  This extends the
 *    cookbook's loop-fence rule: guided tooling should enumerate safe
 *    contiguous statement ranges as well as individual statements.
 *  - `close_not_aimed` sits before the long-range block so the 0x1000 island
 *    remains at the target address; a structured if/else moved it earlier.
 *  - Both runtime divisions require maspsx `--expand-div`; this file also
 *    defines the five gp-relative THINK_3.C globals listed by gpsyms.
 */

extern Humanoid *Me_THINK_C;

extern s16 turn_towards_player_(s32 x, s32 z);
extern s16 ItemUse(void);

short AttackIndirect(void)
{
    s16 pad;
    s16 status7_result;
    s32 degree;

    pad = 0;
    if (Me_THINK_C->status == 7)
    {
        do
        {
            if (Me_THINK_C->motion->count !=
                BattleDB[Me_THINK_C->warid].contfrm)
            {
                status7_result = 0;
                goto status7_return;
            }
            if (Distance < 20000)
            {
                degree = Degree;
                if (degree < 0)
                {
                    degree = -degree;
                }
                if (degree < 500)
                {
                    goto choose_attack;
                }
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                status7_result = pad;
                goto status7_return;
            }
        } while (0);

choose_attack:
        if (Degree >= 301)
        {
            pad = 0x2000;
        }
        else
        {
            pad |= 0x80;
            if (Degree < -300)
            {
                pad = -0x8000;
            }
            else
            {
                goto status7_value;
            }
        }
        pad |= 0x80;

status7_value:
        status7_result = pad;
status7_return:
        return status7_result;
    }
    if (Me_THINK_C->status == 9)
    {
        return pad;
    }

    if (Distance < 20000 && SR != -2)
    {
        SR = 0;
    }

    if (Distance < 5000)
    {
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree >= 1000)
        {
            goto close_not_aimed;
        }

        pad = turn_towards_player_(0, 0) & ~0x5fff;
        if ((u32)(Distance - 1000) > 2000)
        {
            pad |= 0x4000;
        }

        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 200 && Me_THINK_C->motion->mid == 0x501)
        {
            pad = 0x80;
        }
        goto action_ready;

close_not_aimed:
        pad = 0x1000;
        goto action_ready;
    }

    {
        if (rand() % (EngageLevel << 2) == 0)
        {
            degree = Degree;
            if (degree < 0)
            {
                degree = -degree;
            }
            if (degree < 200 && Me_THINK_C->motion->mid == 0x501)
            {
                pad = 0x80;
            }
        }

        if (Distance >= 15001)
        {
            pad = turn_towards_player_(0, 0);
        }
        else if (Degree >= 201)
        {
            pad = 0x2000;
        }
        else if (Degree >= 101)
        {
            pad = SetCommand(&Me_THINK_C->pad, 4);
        }
        else if (Degree < -200)
        {
            pad = -0x8000;
        }
        else if (Degree < -100)
        {
            pad = SetCommand(&Me_THINK_C->pad, 3);
        }
        else
        {
            ItemUse();
        }
    }

action_ready:
    if (pad == 0x80)
    {
        Me_THINK_C->rotate->vy += Degree;
    }
    return pad;
}
