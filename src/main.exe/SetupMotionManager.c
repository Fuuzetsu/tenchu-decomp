#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionManager * SetupMotionManager(struct ModelArchiveType *mad, struct MotionRegistType *mot);
 *     ACTION.C:139, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $a0       struct ModelArchiveType * mad
 *     param $a1       struct MotionRegistType * mot
 * END PSX.SYM */

/*
 * SetupMotionManager (0x8001c43c, 0x9c bytes) — allocate and initialize a
 * MotionManager for a model archive (item.h has the proven layout, recovered
 * from DisposeMotionManager/SetNowMotion). `n` (spline point count) defaults
 * to 2 with no model, else copies the model archive's own `n`; the spline
 * control block is allocated right after based on that count
 * ((n + 1) * sizeof(SplineControlType), 0x18 bytes each).
 *
 * Matching notes (docs/matching-cookbook.md): opposite-polarity if/else —
 * Ghidra renders `if (mad == 0) n=2; else n=mad->n;`, but the object lays the
 * mad->n body out FIRST (fallthrough of a `beqz mad,ELSE` testing mad==0,
 * i.e. the source condition is `mad != 0`), each branch storing directly to
 * `mmp->n` (no shared temp/join store).
 */
extern void *valloc(u32 size);

MotionManager *SetupMotionManager(ModelArchiveType *mad, MotionRegistType *mot)
{
    MotionManager *mmp;

    mmp = (MotionManager *)valloc(sizeof(MotionManager));
    mmp->mid = -1;
    mmp->mask = -1;
    mmp->loop = 0;
    mmp->count = 0;
    mmp->mode = 0;
    if (mad != 0) {
        mmp->n = mad->n;
    } else {
        mmp->n = 2;
    }
    mmp->motion = 0;
    mmp->model = mad;
    mmp->motreg = mot;
    mmp->control = valloc((mmp->n + 1) * 0x18);
    return mmp;
}
