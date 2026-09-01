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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *  - `pad` is the original s16 local.  `attack_result` is a distinct s16
 *    return island: keeping its three edge assignments separate produces the
 *    target's moves into $v0 before one shared sign-extension tail.
 *  - The one-shot `do` encloses a CONTIGUOUS RANGE of three statements, not
 *    just one `if`.  Its loop notes stop cse/local-copy propagation from
 *    replacing `attack_result = 0` with a copy of the already-zero `$s0`.
 *    A bounded permuter found this final one-byte fix.  This extends the
 *    cookbook's loop-fence rule: guided tooling should enumerate safe
 *    contiguous statement ranges as well as individual statements.
 *  - `close_not_aimed` sits before the long-range block so the 0x1000 island
 *    remains at the target address; a structured if/else moved it earlier.
 *  - Both runtime divisions require maspsx `--expand-div`; this file also
 *    defines the five gp-relative THINK_3.C globals listed by gpsyms.
 */

extern Humanoid *Me_THINK_C;

extern s16 GotoPosition(s32 vx, s32 vz);
extern s16 ItemUse(void);

short AttackIndirect(void)
{
    s16 pad;
    s16 attack_result;
    s32 degree;

    pad = 0;
    if (Me_THINK_C->status == STAT_ATTACK)
    {
        do
        {
            if (Me_THINK_C->motion->count !=
                BattleDB[Me_THINK_C->warid].contfrm)
            {
                attack_result = 0;
                goto attack_return;
            }
            if (Distance < INDIRECT_RANGE)
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
                attack_result = pad;
                goto attack_return;
            }
        } while (0);

    choose_attack:
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
        return pad;
    }

    if (Distance < INDIRECT_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
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

        pad = GotoPosition(0, 0) & (PADLleft | PADLright);
        if ((u32)(Distance - 1000) > 3000 - 1000)
        {
            pad |= PADLdown;
        }

        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 200 && Me_THINK_C->motion->mid == MOT_ENGAGE_STANCE)
        {
            pad = PADRleft;
        }
        goto action_ready;

    close_not_aimed:
        pad = PADLup;
        goto action_ready;
    }

    if (rand() % (EngageLevel * 4) == 0)
    {
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 200 && Me_THINK_C->motion->mid == MOT_ENGAGE_STANCE)
        {
            pad = PADRleft;
        }
    }

    if (Distance > 15000)
    {
        pad = GotoPosition(0, 0);
    }
    else if (Degree > 200)
    {
        pad = PADLright;
    }
    else if (Degree > 100)
    {
        pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_RIGHT);
    }
    else if (Degree < -200)
    {
        pad = PADLleft;
    }
    else if (Degree < -100)
    {
        pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_LEFT);
    }
    else
    {
        ItemUse();
    }

action_ready:
    if (pad == PADRleft)
    {
        Me_THINK_C->rotate->vy += Degree;
    }
    return pad;
}
