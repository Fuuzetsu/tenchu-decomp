#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3firstattack(void);
 *     THINK_3.C:209, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     reg   $a0       short pad
 *     reg   $v1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

/*
 * Build a first-attack control word from the facing direction and weapon
 * class. Close targets clear SR, civilians set Attrib bit 0x10, ranged
 * weapons suppress non-turn bits while badly aimed, and targets inside the
 * per-class atkd2 range add the attack bit.
 *
 * The duplicated Degree assignment is an intentional cc1 2.8.1 input.
 * jump2 erases the condition and identical arms, but their dependency keeps
 * the mask ahead of the Degree load. That preserves the target's two delay
 * nops and register allocation. Keeping result and masked in SImode likewise
 * defers the function's s16 conversion to the shared return tail.
 */
extern Humanoid *Me_THINK_C;
extern s32 Distance;
extern s16 Degree;
extern s16 SR;
extern u16 Attrib;
extern s16 atkd2[4];
extern int turn_towards_player_(int x_diff, int z_diff);

s16 Think3firstattack(void)
{
    s32 result;
    s16 wclass;
    s32 degree;

    result = turn_towards_player_(0, 0);
    if (Distance < 10000 && SR != -2)
    {
        SR = 0;
    }
    if ((Me_THINK_C->type & 0xF0) == 0x90)
    {
        Attrib |= 0x10;
    }
    wclass = Me_THINK_C->wpatk >> 4;
    if (wclass == 3)
    {
        s32 masked;

        masked = result & ~0x5FFF;
        if (masked != 0)
        {
            degree = Degree;
        }
        else
        {
            degree = Degree;
        }
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree > 100)
        {
            return masked;
        }
        result = masked;
    }
    if (Distance < atkd2[wclass])
    {
        result |= 0x80;
        Attrib |= 0x10;
    }
    return result;
}
