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
 * static short AttackLong(void);
 *     THINK_3.C:450, 71 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $a0       short ad
 *     reg   $s0       short pad
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern short Attrib;
 *     extern long GameClock;
 *     extern long AttackActionCount;
 * END PSX.SYM */

/*
 * STATUS: MATCHING
 *
 * Three source-shape facts close the function:
 *  - Assigning GameClock to AttackActionCount before compound-adding
 *    EngageLevel*10 preserves the target's a0 accumulator/writeback.
 *  - `(raw >= 0) ? raw : -raw` reaches GCC's abssi2 expansion and emits the
 *    target's copy-then-self-negu form.  The LT ternary does not.
 *  - Spelling the two random choices as a nested positive arm makes ItemUse
 *    the cold final arm and lets jump2 merge the four SetCommand calls into
 *    the target's single call tail.
 */

extern Humanoid *Me_THINK_C;
extern u16 Attrib;
extern s32 AttackActionCount;

extern s16 ChasetoTarget(s32 distance);
extern s16 ItemUse(void);

short AttackLong(void)
{
    s16 ad;
    s16 pad;
    s16 status7_result;
    s32 degree;

    pad = 0;
    if (Me_THINK_C->status == 7)
    {
        s32 status_degree;

        do
        {
            if (Me_THINK_C->motion->count !=
                BattleDB[Me_THINK_C->warid].contfrm)
            {
                status7_result = 0;
                goto status7_return;
            }
            if (Distance < 3000)
            {
                status_degree = Degree;
                if (status_degree < 0)
                {
                    status_degree = -status_degree;
                }
                if (status_degree < 1500)
                {
                    goto choose_status7;
                }
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                status7_result = pad;
                goto status7_return;
            }
        } while (0);

choose_status7:
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
        return 0;
    }

    if (Me_THINK_C->motion->mid == 0x500)
    {
        return (u16)(rand() % (EngageLevel + 1) != 0) << 14;
    }

    if (Me_THINK_C->actmode == 0)
    {
        pad = ChasetoTarget(3000);
        if (pad == 0 || (Attrib & 0x4000) != 0)
        {
            Me_THINK_C->actmode = 1;
        }
        if (Me_THINK_C->motion->count != 0)
        {
            goto return_pad;
        }
        if (rand() % 5 != 0)
        {
            goto return_pad;
        }
        pad = 0x1040;
        goto return_pad;
    }

    if ((Me_THINK_C->motion->count & 0xf) != 0)
    {
        s32 motion_degree;

        pad = Me_THINK_C->pad.data;
        if (Distance >= 3000)
        {
            goto return_pad;
        }
        ad = Degree;
        motion_degree = (ad >= 0) ? ad : -ad;
        if (motion_degree < 500)
        {
            pad = 0x4000;
            goto return_pad;
        }
        if (motion_degree < 1501)
        {
            goto return_pad;
        }
        pad = 0x1000;
        goto return_pad;
    }

    if (Distance >= 5001)
    {
        Humanoid *me;

        Me_THINK_C->actmode = 0;
        me = Me_THINK_C;
        Me_THINK_C->chase[1] = 0;
        me->chase[0] = 0;
        ItemUse();
        return 0;
    }

    if ((Attrib & 0x400) != 0)
    {
        Me_THINK_C->actmode = 0;
    }

    if (Degree >= 301)
    {
        pad = 0x2000;
    }
    else if (Degree < -300)
    {
        pad = -0x8000;
    }

    if ((u32)(Distance - 3001) < 999)
    {
        if (rand() % (EngageLevel + 1) == 0)
        {
            AttackActionCount = GameClock;
            AttackActionCount += EngageLevel * 10;
            return pad | 0x80;
        }
    }

    ad = Degree;
    degree = (ad >= 0) ? ad : -ad;

    if (degree > 1000 || Distance < 3000)
    {
        if (Distance < 1000)
        {
            pad = 0xa0;
            if ((rand() & 1) != 0)
            {
                pad = 0x4040;
            }
        }
        else
        {
            pad = 0x2080;
            if ((rand() & 1) != 0)
            {
                pad = -0x7f80;
            }
        }
        goto return_pad;
    }

    if (Distance < 4001)
    {
        goto return_pad;
    }

    if (degree < 50)
    {
        pad = SetCommand(&Me_THINK_C->pad, 0x21);
        goto return_pad;
    }
    else
    {
        if (Me_THINK_C->motion->count != 0)
        {
            pad |= 0x1000;
            goto return_pad;
        }
        if (ad > 50)
        {
            pad = SetCommand(&Me_THINK_C->pad, 4);
            goto return_pad;
        }
        if (ad < -50)
        {
            pad = SetCommand(&Me_THINK_C->pad, 3);
            goto return_pad;
        }
        if ((rand() & 1) != 0)
        {
            if ((rand() & 1) != 0)
            {
                pad = SetCommand(&Me_THINK_C->pad, 1);
                goto return_pad;
            }
            pad = 0xc0;
        }
        else
        {
            ItemUse();
        }
        goto return_pad;
    }

return_pad:
    return pad;
}
