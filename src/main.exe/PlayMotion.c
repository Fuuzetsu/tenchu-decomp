#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short PlayMotion(struct MotionManager *mmp, short mode);
 *     ACTION.C:220, 17 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct MotionManager * mmp
 *     param $a1       short mode
 * END PSX.SYM */

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
extern short SweepMotion(MotionManager *mmp);

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
