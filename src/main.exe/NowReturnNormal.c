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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * `short` local (matching SetNowMotion's `short mid`/`short move` parameters).
 * That makes the later signed sra/sll+beqz idioms (not andi) reappear.
 */
extern void ReturnNormal(void);
extern Humanoid *Me_MOTION_C;

short NowReturnNormal(Humanoid *human)
{
    Humanoid *h;
    MotionDataType *md;
    short mid;
    short move;

    Me_MOTION_C = human;
    ReturnNormal();
    h = Me_MOTION_C;
    mid = *(u16 *)&motID;
    move = *(u16 *)&motMODE;
    if (h->status == 0x11 && h->motion->loop == -1) {
        return 0;
    }
    if (UpdateMotion(h->motion, mid) == 0) {
        return 0;
    }
    h->status = (s8)(mid >> 8);
    if (move != 0) {
        md = h->motion->motion;
        MoveHumanoid(h, md->orderspd, md->sidespd);
    }
    return 1;
}
