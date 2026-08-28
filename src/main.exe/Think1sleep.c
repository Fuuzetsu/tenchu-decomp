#include "common.h"
#include "main.exe.h"
#include "item.h"

/* Sleeping guard: wake into the damage flinch when the nap animation runs
 * out, clear the search request on a scripted action, and hand control to
 * the pursue logic when the alarm or the attribute alert bit is up. */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1sleep(void);
 *     THINK_1.C:106, 8 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern long EmergencyNotice;
 *     extern short Attrib;
 * END PSX.SYM */

s16 Think1sleep(void)
{
    MotionManager *mmp;
    u16 mot;

    mmp = Me_THINK_C->motion;
    mot = 0;
    if (mmp->mid == MOT_ACTION)
    {
        SR = -1;
    }
    else if (mmp->count == 0)
    {
        mot = PADLup | PADL2;
    }
    if ((EmergencyNotice != 0) || ((Attrib & 0x8000) != 0))
    {
        mot = turn_towards_player_(0, 0);
        mot = mot & (PADLleft | PADLright);
    }
    return mot;
}
