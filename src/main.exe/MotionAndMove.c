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

/*
 * MotionAndMove (0x80027210, 0x90 bytes) — advance the current motion
 * command onto Me_MOTION_C via SetNowMotion, but first guard against
 * clobbering a motion already mid-update on another humanoid: when
 * MotionUpdateMode is set, scan the 5-entry CVAhuman[] table and bail early
 * (return 0, no call) if Me_MOTION_C is already one of the tracked
 * entries. CVAhuman[5] stride 8 (0x800c2cc8..0x800c2cf0 = 0x28 = 5*8);
 * only the leading `human` pointer field is ever read here, so the
 * trailing 4 bytes are undetermined padding.
 *
 * Matching notes:
 *  - `i` is `short`: indexing CVAhuman[i] (8-byte stride) compiles the
 *    index as a fused sign-extend+scale-by-8 (sll 16 / sra 13) — the
 *    ordinary 2-instruction short-index form (cookbook toolchain
 *    gotchas: distinct from the blocked 3-instruction sign-extend-split
 *    class, which needs a THIRD sra).
 *  - MotionUpdateMode/motID/motMODE are proven u16 in other TUs
 *    (NowReturnNormal.c reads motID with `lhu`), but THIS TU's own
 *    accesses are all signed (`lh` reads, `addiu -1` not `ori 0xffff`
 *    for the terminator store) — per-TU divergent type, same precedent
 *    as other localized signed/unsigned field views (cookbook Expressions).
 */
extern Humanoid *Me_MOTION_C;
extern s16 motID;
extern s16 motMODE;

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
            i = i + 1;
        } while (i < 5);
    }
    result = SetNowMotion(Me_MOTION_C, motID, motMODE);
    motMODE = -1;
    return result;
}
