#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * PlayMotion (0x8001c584, 0xbc bytes) — per-frame motion-manager advance:
 * bails returning 0 if `loop` is negative (motion disabled/one-shot-done).
 * With mode == 0 (normal playback), advances `count` and, once it reaches
 * the current motion's `time` limit, resets `count` to 0 and bumps `loop`.
 * With mode != 0, either forwards to SweepMotion (when `count` is negative)
 * or to ActiveMotion, bumping `loop` only when ActiveMotion reports done
 * (0). Always returns the (possibly just-reset) `count`.
 *
 * item.h's MotionManager (mid/count/loop/n/mask/mode/model/motion/motreg/
 * control) and MotionDataType (n/sweep/orderspd/sidespd) are proven by
 * SetNowMotion.c and friends; this function adds MotionDataType's `time`
 * field @0x4 (a raw `lh` on mmp->motion), matching Ghidra's own
 * independently-built MotionDataType exactly (reference/ghidra_types.h).
 *
 * Matching notes (docs/matching-cookbook.md): both the SweepMotion path and
 * the ActiveMotion!=0 path skip the trailing `mmp->loop = result + 1;` via
 * an early `goto done` (shared final `return mmp->count;` — Ghidra's own
 * two-goto rendering matches the raw asm's shared tail directly, no
 * restructuring needed).
 */
extern s32 ActiveMotion(MotionManager *mmp);
extern void SweepMotion(MotionManager *mmp);

short PlayMotion(MotionManager *mmp, short mode)
{
    short result;

    if (mmp->loop < 0) {
        return 0;
    }
    if (mode != 0) {
        if (mmp->count < 0) {
            SweepMotion(mmp);
            goto done;
        }
        result = ActiveMotion(mmp);
        if (result != 0) {
            goto done;
        }
        result = mmp->loop;
    } else {
        result = mmp->count + 1;
        mmp->count = result;
        if (result <= mmp->motion->time) {
            goto done;
        }
        result = mmp->loop;
        mmp->count = 0;
    }
    mmp->loop = result + 1;
done:
    return mmp->count;
}
