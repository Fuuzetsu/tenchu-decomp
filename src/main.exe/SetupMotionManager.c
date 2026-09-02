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

extern void *valloc(u32 size);

MotionManager *SetupMotionManager(ModelArchiveType *mad, MotionRegistType *mot)
{
    MotionManager *manager;

    manager = (MotionManager *)valloc(sizeof(MotionManager));
    manager->mid = MOTION_ID_NONE;
    manager->mask = MOTION_MASK_EVERY_PART;
    manager->loop = 0;
    manager->count = 0;
    manager->mode = MOTION_MODE_DEFAULT;
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
    manager->control =
        valloc((manager->n + 1) * sizeof(SplineControlType));
    return manager;
}
