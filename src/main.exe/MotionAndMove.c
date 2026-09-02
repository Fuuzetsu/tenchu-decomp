#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short MotionAndMove(void);
 *     MOTION.C:173, 11 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

short MotionAndMove(void)
{
    short i;
    short result;

    if (MotionUpdateMode != 0)
    {
        i = 0;
        do
        {
            if (CVAhuman[i].human == Me_MOTION_C)
            {
                return 0;
            }
            i++;
        } while (i < N_CVA_HUMANS);
    }
    result = SetNowMotion(Me_MOTION_C, motID, motMODE);
    motMODE = MOTION_MOVE_UNSET;
    return result;
}
