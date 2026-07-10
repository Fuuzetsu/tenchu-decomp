#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeMotionManager(struct MotionManager *mmp);
 *     ACTION.C:160, 5 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct MotionManager * mmp
 * END PSX.SYM */

/*
 * DisposeMotionManager (0x8001c4d8) — free a motion-manager's spline
 * control block, then the manager itself (called from KillHumanoid). Same
 * null-check-then-free shape as DisposeModel/DisposeOrnament/DisposeAreaMap,
 * but with a nested field freed first — `mmp` survives the first vfree call
 * in a callee-saved register (still needed for the second free) with no
 * special handling; MotionManager's `control` field (0x18) is new here —
 * Ghidra's struct layout matches item.h's proven mid/count/loop/n/mask/mode/
 * model/motion prefix exactly, so it's added (with `motreg` @0x14) rather
 * than redefined.
 */
extern void vfree(void *p);

void DisposeMotionManager(MotionManager *mmp)
{
    if (mmp != 0)
    {
        vfree(mmp->control);
        vfree(mmp);
    }
}
