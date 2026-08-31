#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"
#include "humanoid.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackLong(void);
 *     THINK_3.C:450, 71 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
extern s32 AttackActionCount;

extern s16 ChasetoTarget(s32 distance);
extern s16 ItemUse(void);

short AttackLong(void)
{
    s16 ad;
    s16 pad;
    s16 attack_result;
    s32 degree;

    pad = 0;
    if (Me_THINK_C->status == STAT_ATTACK)
    {
        s32 deg;

        /* One-shot fence: byte-required (collapse measured; see cookbook). */
        do
        {
            if (Me_THINK_C->motion->count !=
                BattleDB[Me_THINK_C->warid].contfrm)
            {
                attack_result = 0;
                goto attack_return;
            }
            if (Distance < 3000)
            {
                deg = Degree;
                if (deg < 0)
                {
                    deg = -deg;
                }
                if (deg < 1500)
                {
                    goto choose_attack;
                }
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                attack_result = pad;
                goto attack_return;
            }
        } while (0);

    choose_attack:
        /* The doubled |= PADRleft around the goto is byte-required (the
         * flat else-if respell mismatches; measured). */
        if (Degree > 300)
        {
            pad = PADLright;
        }
        else
        {
            pad |= PADRleft;
            if (Degree < -300)
            {
                pad = PADLleft;
            }
            else
            {
                goto attack_value;
            }
        }
        pad |= PADRleft;

    attack_value:
        attack_result = pad;
    attack_return:
        return attack_result;
    }

    if (Me_THINK_C->status == STAT_JUMP)
    {
        return 0;
    }

    if (Me_THINK_C->motion->mid == MOT_ENGAGE)
    {
        return (rand() % (EngageLevel + 1) != 0) ? PADLdown : 0;
    }

    if (Me_THINK_C->actmode == 0)
    {
        pad = ChasetoTarget(3000);
        if (pad == 0 || (ATTRIB_BITS & ATTR_HIT) != 0)
        {
            Me_THINK_C->actmode = 1;
        }
        if (Me_THINK_C->motion->count == 0)
        {
            if (rand() % 5 == 0)
            {
                pad = PADLup | PADRdown;
            }
        }
        return pad;
    }

    if ((Me_THINK_C->motion->count & 0xf) != 0)
    {
        s32 deg;

        pad = Me_THINK_C->pad.data;
        if (Distance < 3000)
        {
            ad = Degree;
            deg = (ad >= 0) ? ad : -ad;
            if (deg < 500)
            {
                pad = PADLdown;
            }
            else if (deg > 1500)
            {
                pad = PADLup;
            }
        }
        return pad;
    }

    if (Distance > 5000)
    {
        Humanoid *me;

        /* The mid-sequence alias and mixed spellings are byte-required
         * (uniform spelling recolors the stores; measured). */
        Me_THINK_C->actmode = 0;
        me = Me_THINK_C;
        Me_THINK_C->chase[1] = 0;
        me->chase[0] = 0;
        ItemUse();
        return 0;
    }

    if ((ATTRIB_BITS & ATTR_WALL) != 0)
    {
        Me_THINK_C->actmode = 0;
    }

    if (Degree > 300)
    {
        pad = PADLright;
    }
    else if (Degree < -300)
    {
        pad = PADLleft;
    }

    if (Distance > 3000 && Distance < 4000)
    {
        if (rand() % (EngageLevel + 1) == 0)
        {
            AttackActionCount = GameClock;
            AttackActionCount += EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
            return pad | PADRleft;
        }
    }

    ad = Degree;
    degree = (ad >= 0) ? ad : -ad;

    if (degree > 1000 || Distance < 3000)
    {
        if (Distance < 1000)
        {
            pad = PADRleft | PADRright;
            if ((rand() & 1) != 0)
            {
                pad = PADLdown | PADRdown;
            }
        }
        else
        {
            pad = PADLright | PADRleft;
            if ((rand() & 1) != 0)
            {
                pad = PADLleft | PADRleft;
            }
        }
        return pad;
    }

    if (Distance <= 4000)
    {
        return pad;
    }

    if (degree < 50)
    {
        pad = SetCommand(&Me_THINK_C->pad, CMD_LUNGE);
    }
    else if (Me_THINK_C->motion->count != 0)
    {
        pad |= PADLup;
    }
    else if (ad > 50)
    {
        pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_RIGHT);
    }
    else if (ad < -50)
    {
        pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_LEFT);
    }
    else if ((rand() & 1) != 0)
    {
        if ((rand() & 1) != 0)
        {
            pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_FORWARD);
        }
        else
        {
            pad = PADRleft | PADRdown;
        }
    }
    else
    {
        ItemUse();
    }
    return pad;
}
