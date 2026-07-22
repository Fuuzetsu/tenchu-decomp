#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3escape(void);
 *     THINK_3.C:103, 21 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

/*
 * Think3escape clears SR at close range, chooses a turn/run command from
 * Degree, and optionally arms a timed escape hint in the current Humanoid.
 * The division is a real runtime divide, so this file remains in the
 * maspsx --expand-div list.
 *
 * The three byte-neutral identical-arm spellings are intentional cc1 2.8.1
 * inputs. The first gives the long-lived result enough allocation weight for
 * $a0. The second keeps the raw Degree and its absolute-value copy distinct
 * through cse, producing the target's self-referencing `negu $v0,$v0`. The
 * last preserves the post-guard Humanoid copy (`$v1` to `$a1`). jump2 merges
 * every pair, so none of the tests or duplicate assignments survives in the
 * emitted function.
 */
extern Humanoid *Me_THINK_C;

s16 Think3escape(void)
{
    s32 result;
    s32 degree;

    result = 0;
    if (Distance < 0x4074 && SR != -2)
    {
        SR = 0;
    }
    if (Degree > 0)
    {
        result = -0x8000;
    }
    else
    {
        if (Degree < 0)
        {
            if (result != 0)
            {
                result = 0x2000;
            }
            else
            {
                result = 0x2000;
            }
        }
    }
    degree = Degree;
    if (degree < 0)
    {
        degree = -degree;
    }
    if (degree < 1000)
    {
        result |= 0x4000;
    }
    else
    {
        result |= 0x1000;
    }
    if (ATTRIB_BITS & 0x400)
    {
        Humanoid *human;
        s32 degree2;
        s32 abs_degree2;

        degree2 = Degree;
        if (result != 0)
        {
            abs_degree2 = degree2;
        }
        else
        {
            abs_degree2 = degree2;
        }
        if (degree2 < 0)
        {
            abs_degree2 = -abs_degree2;
        }
        if (abs_degree2 > 1000 && Me_THINK_C->field76_0xb0 == 0)
        {
            s32 quotient;

            if (result != 0)
            {
                human = Me_THINK_C;
            }
            else
            {
                human = Me_THINK_C;
            }
            quotient = 1000 / human->turn;
            human->field76_0xb0 = quotient |
                (degree2 > 0 ? 0x20000000 : 0x80000000);
        }
    }
    return result;
}
