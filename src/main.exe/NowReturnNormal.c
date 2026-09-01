#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short NowReturnNormal(struct Humanoid *human);
 *     MOTION.C:200, 6 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern short motMODE;
 * END PSX.SYM */

/*
 * NowReturnNormal (0x80027004) — force a character back into its normal
 * stance. Latches the character into Me_MOTION_C, calls ReturnNormal() to
 * pick the "return to normal" motion id + move flag (written into the
 * globals motID / motMODE), then applies them via the *exact*
 * guard/UpdateMotion/MoveHumanoid shape SetNowMotion uses on its parameters
 * (see SetNowMotion.c's header for the mid<<16/dual-sra + De Morgan guard
 * mechanics) — reload Me_MOTION_C/motID/motMODE AFTER the call since it
 * may have written them (the compiler can't assume otherwise across a call).
 * motID/motMODE have their recovered signed object types, but this retail
 * caller reads their raw halfwords with `lhu` before copying each into a
 * `motion_id`/`motion_move_mode` locals (matching SetNowMotion's underlying
 * parameter widths). That makes the later signed sra/sll+beqz idioms (not
 * andi) reappear.
 */
extern void ReturnNormal(void);
extern Humanoid *Me_MOTION_C;

short NowReturnNormal(Humanoid *human)
{
    Humanoid *current;
    MotionDataType *motion;
    motion_id next_motion;
    motion_move_mode apply_movement;

    Me_MOTION_C = human;
    ReturnNormal();
    current = Me_MOTION_C;
    next_motion = motID;
    apply_movement = motMODE;
    if (current->status == STAT_DEAD &&
        current->motion->loop == MOTION_LOOP_DISABLED)
    {
        return 0;
    }
    if (UpdateMotion(current->motion, next_motion) == 0)
    {
        return 0;
    }
    current->status = (s8)MOTION_STATUS(next_motion);
    if (apply_movement != MOTION_MOVE_NONE)
    {
        motion = current->motion->motion;
        MoveHumanoid(current, motion->orderspd, motion->sidespd);
    }
    return 1;
}
