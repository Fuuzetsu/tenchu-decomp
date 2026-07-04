#include "common.h"
#include "main.exe.h"
#include "item.h"

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
