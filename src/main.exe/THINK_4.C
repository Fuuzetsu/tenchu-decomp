#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"
#include "sound.h"

/*
 * PSX.SYM and the retail executable agree on this definition order for the
 * original THINK_4.C translation unit.
 */

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4abandon(void);
 *     THINK_4.C:14, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern short SR;
 *     extern long EmergencyNotice;
 * END PSX.SYM */

extern Humanoid *Me_THINK_C;
extern long EmergencyNotice;
extern int GotoPosition(s32 vx, s32 vz);
extern int rand(void);

s16 Think4abandon(void)
{
    u16 cleared;
    s16 pad;

    cleared = Attrib & ~(ATTR_SEARCH | ATTR_PHASE);
    Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
    Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
    if ((Me_THINK_C->type & PAGE_MASK) == PAGE_BOSS)
    {
        if ((u16)(SR - 1) < 2)
        {
            Attrib = cleared | PHASE_ALERT;
            if (Me_THINK_C->motion->count == 0)
            {
                s32 r;

                r = rand();
                if (r % 60 == 0)
                {
                    Sound(Me_THINK_C, CHAR_VOICE_ALERT);
                }
            }
        }
        return (GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED);
    }
    else if (EmergencyNotice != 0)
    {
        if (SR == SR_SEEN)
        {
            Attrib = cleared | PHASE_ALERT;
        }
        return (GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED);
    }
    else
    {
        if (Me_THINK_C->think[3] == Think4abandon)
        {
            pad = (GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED);
            if (pad != 0)
            {
                return pad;
            }
        }
        if (SR == SR_SEEN)
        {
            goto sr_seen;
        }
        if (SR >= 2)
        {
            if (SR == SR_GLIMPSE)
            {
                goto sr_glimpse;
            }
            return 0;
        }

        if (SR >= SR_NONE)
        {
            return 0;
        }
        if (SR < SR_GONE)
        {
            return 0;
        }
        /* Target lost: stand down (0x80f sheathes and returns to
         * idle — ActSTATE) with the give-up voice line. */
        Attrib = cleared;
        SetNowMotion(Me_THINK_C, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
        Sound(Me_THINK_C, CHAR_VOICE_REACTION);
        return 0;

    sr_glimpse:
        /* Only a glimpse: drop to suspicious and stand down. */
        Attrib = cleared | PHASE_SUSPICIOUS;
        SetNowMotion(Me_THINK_C, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
        return 0;

    sr_seen:
        Attrib = cleared | PHASE_ALERT;
        return 0;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4contact(void);
 *     THINK_4.C:36, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a1       short pad
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

s16 Think4contact(void)
{
    s32 pad;

    if (SR == SR_SEEN)
    {
        Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
        return 0;
    }

    if (Me_THINK_C->chase[HUMANOID_CHASE_X] == 0 && Me_THINK_C->chase[HUMANOID_CHASE_Z] == 0)
    {
        if (Me_THINK_C->actcnt >= THINK4_ABANDON_TICKS)
        {
            return Think4abandon();
        }
        else
        {
            Me_THINK_C->actcnt++;
            pad = 0;
            if (Me_THINK_C->turn < Degree)
            {
                pad = PADLright;
            }
            else if (Degree < -Me_THINK_C->turn)
            {
                pad = -PADLleft;
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me_THINK_C->actscnt++;
        dx = Me_THINK_C->chase[HUMANOID_CHASE_X] - Me_THINK_C->locate->vx;
        dz = Me_THINK_C->chase[HUMANOID_CHASE_Z] - Me_THINK_C->locate->vz;
        pad = GotoPosition(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < THINK4_ARRIVAL_DISTANCE ||
            Me_THINK_C->actscnt == 0)
        {
            Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
            Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
            Me_THINK_C->actcnt = 0;
        }
    }
    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4chase(void);
 *     THINK_4.C:75, 189 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       short pad
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

s16 Think4chase(void)
{
    s32 pad;

    if (SR == SR_SEEN)
    {
        Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
        return 0;
    }

    if (Me_THINK_C->chase[HUMANOID_CHASE_X] == 0 && Me_THINK_C->chase[HUMANOID_CHASE_Z] == 0)
    {
        if (Me_THINK_C->actcnt >= THINK4_ABANDON_TICKS)
        {
            return Think4abandon();
        }
        else
        {
            Me_THINK_C->actcnt++;
            pad = PADLup;
            if (Me_THINK_C->actcnt < THINK4_INITIAL_STEER_TICKS)
            {
                if (Degree > Me_THINK_C->turn)
                {
                    pad = PADLup | PADLright;
                }
                else if (Degree < -Me_THINK_C->turn)
                {
                    /* The signed view keeps this combination in addiu's
                     * immediate range. */
                    pad = (s16)(PADLleft | PADLup);
                }
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me_THINK_C->actscnt++;
        dx = Me_THINK_C->chase[HUMANOID_CHASE_X] - Me_THINK_C->locate->vx;
        dz = Me_THINK_C->chase[HUMANOID_CHASE_Z] - Me_THINK_C->locate->vz;
        pad = GotoPosition(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < THINK4_ARRIVAL_DISTANCE ||
            Me_THINK_C->actscnt == 0)
        {
            Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
            Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
            Me_THINK_C->actcnt = 0;
        }
    }
    return pad;
}
