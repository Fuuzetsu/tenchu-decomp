#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short NowReturnNormal(struct Humanoid *human);
 *     MOTION.C:200, 6 src lines, frame 32 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 * END PSX.SYM */

/*
 * NowReturnNormal (0x80027004) — force a character back into its normal
 * stance. Latches the character into Me_MOTION_C, calls ReturnNormal() to
 * pick the "return to normal" motion id + move flag (written into the
 * globals motID / D_80097F0E), then applies them via the *exact*
 * guard/UpdateMotion/MoveHumanoid shape SetNowMotion uses on its parameters
 * (see SetNowMotion.c's header for the mid<<16/dual-sra + De Morgan guard
 * mechanics) — reload Me_MOTION_C/motID/D_80097F0E AFTER the call since it
 * may have written them (the compiler can't assume otherwise across a call).
 * motID/D_80097F0E are u16 (lhu on first read), but each is copied into a
 * `short` local before use (matching SetNowMotion's `short mid`/`short move`
 * parameter types) — that's what makes the later signed sra/sll+beqz
 * idioms (not andi) reappear here despite the globals being unsigned.
 */
extern void ReturnNormal(void);
extern Humanoid *Me_MOTION_C;
extern u16 motID;
extern u16 D_80097F0E;

short NowReturnNormal(Humanoid *human)
{
    Humanoid *h;
    MotionDataType *md;
    short mid;
    short move;

    Me_MOTION_C = human;
    ReturnNormal();
    h = Me_MOTION_C;
    mid = motID;
    move = D_80097F0E;
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
