#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3firstattack(void);
 *     THINK_3.C:209, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * nops and register allocation. Keeping pad and masked in SImode likewise
 * defers the function's s16 conversion to the shared return tail.
 */
extern Humanoid *Me_THINK_C;
/* Per-range-class first-attack distances, indexed by weapon attack class
 * (same shape as Think3attack.c's atkd table). */
extern s16 atkd2[N_WEAPON_ATTACK_CLASSES];
/* Retail's own prototype drift (def: s16(s32, s32)) -- byte-required: correcting it changes the caller. */
extern int GotoPosition(int vx, int vz);

s16 Think3firstattack(void)
{
    s32 pad;
    weapon_attack_class idx;
    s32 degree;

    pad = GotoPosition(0, 0);
    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if ((Me_THINK_C->type & PAGE_MASK) == PAGE_CIVILIAN)
    {
        Attrib |= ATTR_SEARCH;
    }
    idx = WEAPON_ATTACK_CLASS(Me_THINK_C->wpatk);
    if (idx == WEAPON_ATTACK_RANGED)
    {
        s32 masked;

        masked = pad & PAD_TURN_BUTTONS_SIGNED;
        /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
        do
        {
        } while (0);
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree > 100)
        {
            return masked;
        }
        pad = masked;
    }
    if (Distance < atkd2[idx])
    {
        pad |= PADRleft;
        Attrib |= ATTR_SEARCH;
    }
    return pad;
}
