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
 *     extern short Attrib;
 * END PSX.SYM */

s16 Think1sleep(void)
{
    MotionManager *temp_a0;
    u16 uVar1;

    temp_a0 = Me_THINK_C->motion;
    uVar1 = 0;
    if (temp_a0->mid == 0x100)
    {
        SR = -1;
    }
    else if (temp_a0->count == 0)
    {
        uVar1 = 0x1001;
    }
    if ((FRAMES_UNTIL_END_OF_ALERT != 0) || ((Attrib & 0x8000) != 0))
    {
        uVar1 = turn_towards_player_(0, 0);
        uVar1 = uVar1 & 0xA000;
    }
    return uVar1;
}
