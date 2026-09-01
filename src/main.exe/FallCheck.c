#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short FallCheck(void);
 *     MOTION.C:256, 31 src lines, frame 40 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct SVECTOR vect
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct MotionManager *dtM;
 *     extern short RefrectMove[16][2];
 *     extern struct VECTOR *dtL;
 *     extern short motMODE;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

/*
 * FallCheck (0x8001cc90, 0x220 bytes) — transition a sufficiently high
 * falling humanoid into MOT_STATE_FALL, push it off the coded map edge
 * by width/4 as the fall begins, and cancel the current attack.
 *
 * Matching notes:
 *  - The zero-attribute path is the positive wrapper around the height and
 *    status switch. This keeps the attribute guard as a direct branch to the
 *    epilogue and leaves the jump table's shared return-zero island between
 *    the STAT_SQUAT arm (jump-table index 7, biased by -4) and the
 *    default body.
 *  - The default and the looping STAT_SQUAT arm jump to `fall`, after the common
 *    return-zero statement. That source layout emits the target's physical
 *    case order: STAT_SQUAT, return-zero, default.
 *  - Comparing CVAhuman[] against the global Me_MOTION_C directly preserves
 *    the target's CSE copy in the scan prologue.
 */

extern Humanoid *Me_MOTION_C;


short FallCheck(void)
{
    if (motID == MOT_STATE_FALL)
    {
        return 1;
    }
    if (motID != MOT_ATTACK_DIVE)
    {
        if (Me_MOTION_C->status == STAT_JUMP && Me_MOTION_C->map.height > 0)
        {
            return 1;
        }
        if (((u16)Me_MOTION_C->attribute & ATTR_FLOAT) == 0)
        {
            if (Me_MOTION_C->map.height <= 1000)
            {
                return 0;
            }
            switch (Me_MOTION_C->status)
            {
            case STAT_SQUAT:
                if (dtM->loop != 0)
                {
                    goto fall;
                }
            case STAT_KAGI:
            case STAT_HANG:
            case STAT_CEILHANG:
            case STAT_DAMAGE:
            case STAT_DEAD:
                break;
            default:
                goto fall;
            }
        }
    }
    return 0;

fall:
    dtM->mask = MOTION_MASK_ALL;
    dtL->vx += (Me_MOTION_C->width *
                RefrectMove[Me_MOTION_C->map.angleH][0]) >> 2;
    dtL->vz += (Me_MOTION_C->width *
                RefrectMove[Me_MOTION_C->map.angleH][1]) >> 2;
    motMODE = MOTION_MOVE_NONE;
    motID = MOT_STATE_FALL;
    SET_NOW_MOTION_UNLESS_CVA(goto found);
found:
    if (Me_MOTION_C->status == STAT_SQUAT)
    {
        dtM->count >>= 2;
    }
    AttackCancelControl(ATTACK_CANCEL_ALL);
    return -1;
}
