#include "common.h"
#include "main.exe.h"
#include "item.h"

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
    if (mmp->mid == 0x100)
    {
        SR = -1;
    }
    else if (mmp->count == 0)
    {
        mot = 0x1001;
    }
    if ((EmergencyNotice != 0) || ((Attrib & 0x8000) != 0))
    {
        mot = turn_towards_player_(0, 0);
        mot = mot & 0xA000;
    }
    return mot;
}
