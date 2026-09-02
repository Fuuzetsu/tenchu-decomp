#include "common.h"
#include "main.exe.h"
#include "item.h"

/* Think handlers return synthesized pad input; PADLup|PADL2 restarts the
 * scripted sleep action. */
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
    u16 pad;

    mmp = Me_THINK_C->motion;
    pad = 0;
    if (mmp->mid == MOT_ACTION)
    {
        SR = SR_UNSEEN;
    }
    else if (mmp->count == 0)
    {
        pad = PADLup | PADL2;
    }
    if ((EmergencyNotice != 0) || ((Attrib & ATTR_PUSH) != 0))
    {
        pad = GotoPosition(0, 0);
        pad = pad & (PADLleft | PADLright);
    }
    return pad;
}
