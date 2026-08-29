#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3attack(void);
 *     THINK_3.C:70, 29 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $v0       short rng
 *     reg   $s0       short pad
 *     reg   $a2       short idx
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * Select the attack controls for an alerted humanoid.  The weapon class
 * determines the turn and distance thresholds; close targets are attacked,
 * distant targets are approached, and item use is considered while idle.
 *
 * The status-7 path deliberately has its own literal return.  GCC merges its
 * short-return conversion with the final return, but the extra control-flow
 * boundary keeps that conversion above the epilogue restores, as in retail.
 */

extern Humanoid *Me_THINK_C;
/* Per-range-class engagement distances (retail data: 3000/3500/4000
 * for the melee classes, 20000 for the ranged class — wpatk >> 4). */
extern s16 atkd[4];

extern s16 SuccessionAttack(s32 dist, s16 degree);
extern s16 ItemUse(void);

s16 Think3attack(void)
{
    s16 rng;
    s16 pad;
    s16 idx;

    pad = 0;
    idx = WPATK_CLASS(Me_THINK_C->wpatk);

    if (Me_THINK_C->status == STAT_ATTACK)
    {
        if (idx != 3)
        {
            pad = SuccessionAttack(3000, 1500);
        }
        else
        {
            pad = SuccessionAttack(20000, 500);
        }
        return pad;
    }

    if (SR != SR_GONE &&
        ((idx == WPATK_CLASS_RANGED && Distance < 14000) || Distance < SR_CLEAR_RANGE))
    {
        SR = SR_NONE;
    }

    if ((s16)((4 - idx) * Me_THINK_C->turn) < Degree)
    {
        pad = PADLright;
    }
    else if (Degree < -(s16)((4 - idx) * Me_THINK_C->turn))
    {
        pad = PADLleft;
    }

    if (idx != 3)
    {
        rng = atkd[idx] / 2;
    }
    else
    {
        rng = 4000;
    }

    if (Distance < rng)
    {
        if (__builtin_abs(Degree) < 1000 &&
            Me_THINK_C->motion->count == 0)
        {
            if (Distance < 2000)
            {
                if (rand() % (EngageLevel + 1) != 0)
                {
                    pad |= PADRleft;
                    goto action_ready;
                }
                pad = PADRleft | PADRright;
                goto action_ready;
            }
            pad |= PADRleft;
            goto action_ready;
        }
        pad |= PADLdown;
        goto action_ready;
    }

    if (Distance < atkd[idx])
    {
        if (idx == WPATK_CLASS_RANGED)
        {
            if (pad == 0 && rand() % (EngageLevel * 4) == 0)
            {
                pad = PADRleft;
            }
            goto action_ready;
        }

        if (__builtin_abs(Degree) < 100)
        {
            if (atkd[idx] - 1000 < Distance)
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_LUNGE);
                goto action_ready;
            }
        }

        if (Me_THINK_C->motion->count == 0)
        {
            if (__builtin_abs(Degree) < 1200)
            {
                pad |= PADRleft;
                goto action_ready;
            }
        }

        if (rng + 500 < Distance)
        {
            pad |= PADLup;
        }
        goto action_ready;
    }

    if (Me_THINK_C->status != STAT_ENGAGE)
    {
        goto action_ready;
    }

    if (StagePlayer->status == STAT_SYURI)
    {
        s32 command;
        s32 random;

        random = rand();
        command = CMD_DASH_RIGHT;
        if ((random & 1) != 0)
        {
            command = CMD_DASH_LEFT;
        }
        pad = SetCommand(&Me_THINK_C->pad, command);
        goto action_ready;
    }

    if (Me_THINK_C->motion->count != 0)
    {
        goto use_item;
    }
    if (rand() % 3 != 0)
    {
        goto use_item;
    }
    pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_FORWARD);
    goto action_ready;

use_item:
    ItemUse();

action_ready:
    if (Me_THINK_C->motion->count == 0 &&
        rand() % 30 == 0 &&
        Me_THINK_C->status == STAT_ENGAGE)
    {
        SetNowMotion(Me_THINK_C, MOT_ATTACK_TAUNT, 1); /* taunt */
    }

    return pad;
}
