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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * `manager->n` (no shared temp/join store).
 */
extern void *valloc(u32 size);

MotionManager *SetupMotionManager(ModelArchiveType *mad, MotionRegistType *mot)
{
    MotionManager *manager;

    manager = (MotionManager *)valloc(sizeof(MotionManager));
    manager->mid = -1;
    manager->mask = -1;
    manager->loop = 0;
    manager->count = 0;
    manager->mode = 0;
    if (mad != 0)
    {
        manager->n = mad->n;
    }
    else
    {
        manager->n = 2;
    }
    manager->motion = 0;
    manager->model = mad;
    manager->motreg = mot;
    manager->control = valloc((manager->n + 1) * 0x18);
    return manager;
}
