#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short SuccessionAttack(long dist, short deg);
 *     THINK_3.C:247, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long dist
 *     param $a1       short deg
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 * END PSX.SYM */

/*
 * MATCHED: SuccessionAttack (0x8002fabc, 268 bytes) decides which follow-up
 * attack buttons to synthesize from the current continuation frame, distance,
 * angle, and engagement-level random gate.
 *
 * Matching constraints:
 *  - The entry guard compares motion->count with
 *    BattleDB[Me_THINK_C->warid].contfrm. BattleType is the proven
 *    eight-short, 0x10-byte layout.
 *  - This file requires maspsx --expand-div so rand() % (EngageLevel + 1)
 *    retains cc1's divide guards.
 *  - buttons is s16. u8 destroys the 0x8000/0x2000 values; u16 changes the
 *    target's signed -0x8000 addiu into an ori.
 *  - d = deg is the first statement inside the Distance block. Its independent
 *    sign extension fills the outer guard's delay slot and restores the
 *    otherwise missing instruction.
 *  - Assign t = raw < d before testing it, so the comparison reuses Degree's
 *    dying carrier rather than allocating a fresh result register.
 *  - Mutate raw with __builtin_abs. The opaque abssi2 pattern preserves the
 *    target's in-place branch/nop/negu sequence through delay-slot reorg;
 *    a manual sign fix or separate destination changes the schedule.
 *  - Preserve the tail topology: Degree > 300 selects PADLright; otherwise
 *    PADRleft is set before the Degree < -300 test, and the neutral-angle arm
 *    jumps to the shared ret label. The modulo-failure edge also jumps there.
 *    These gotos keep that signed epilogue shared while leaving the initial
 *    zero return distinct.
 */
extern Humanoid *Me_THINK_C;
extern int rand(void);

short SuccessionAttack(long dist, short deg)
{
    int t;
    int lev;
    s16 buttons;

    buttons = 0;
    if (Me_THINK_C->motion->count !=
        BattleDB[Me_THINK_C->warid].contfrm)
    {
        return 0;
    }
    if (Distance < dist)
    {
        int d;
        int raw;

        d = deg;
        raw = (int)Degree;
        raw = __builtin_abs(raw);
        t = raw < d;
        if (t)
            goto in_range;
    }
    t = rand();
    lev = EngageLevel + 1;
    if (t % lev != 0)
    {
        goto ret;
    }
in_range:
    if (Degree > 300)
    {
        buttons = PADLright;
    }
    else
    {
        buttons |= PADRleft;
        if (Degree < -300)
        {
            /* PADLleft, spelled negative so the constant fits
             * addiu's signed immediate (fits-andi/addiu lever). */
            buttons = -0x8000;
        }
        else
        {
            goto ret;
        }
    }
    buttons |= PADRleft;
ret:
    return buttons;
}
