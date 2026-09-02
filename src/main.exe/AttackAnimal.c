#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackAnimal(void);
 *     THINK_3.C:583, 26 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short Degree;
 * END PSX.SYM */

short AttackAnimal(void)
{
    s32 deg;
    s32 pad;
    animal_attack_timer am;

    if (Me_THINK_C->status == STAT_ATTACK || Me_THINK_C->status == STAT_JUMP)
    {
        Me_THINK_C->actmode = ANIMAL_ATTACK_TIMER_RESET;
        return 0;
    }
    if (Distance < 2000)
    {
        deg = Degree;
        if (deg < 0)
        {
            deg = -deg;
        }
        if (deg < 200)
        {
            return PADRleft; /* bite */
        }
    }
    Me_THINK_C->actmode++;
    pad = GotoPosition(0, 0);
    am = Me_THINK_C->actmode;
    if (am < ANIMAL_ATTACK_NOTICE_FRAME)
    {
        pad = PADLup;
    }
    else if (am == ANIMAL_ATTACK_NOTICE_FRAME)
    {
        Sound(Me_THINK_C, CHAR_VOICE_NOTICE);
    }
    else if (am < ANIMAL_ATTACK_FULL_STEER_FRAME)
    {
        pad = pad & (PADLleft | PADLright);
    }
    return pad;
}
