#include "common.h"
#include "main.exe.h"
#include "item.h"

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
