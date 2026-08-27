#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR *dtL;
 * END PSX.SYM */

/*
 * publish_ground_point_ (0x80027304) — publish the current motion model's ground
 * position into `dtL` (a VECTOR*, vx/vz only — skips vy, same
 * copy-two-of-three-fields shape as GetConflictResult's ConflictDistance)
 * when its id is a valid conflict-pool slot.
 * `Me_MOTION_C->model->object[0]` is the first sub-model of the current
 * character's active ModelArchiveType (item.h); its `id` indexes ConflictObject,
 * the same conflict pool typed in GetConflictResult.c. Array INDEXING (not
 * just a pointer's leading fields) needs the real element stride, so unlike
 * a DisposeBG-style truncated pad view, this local copy must keep the FULL
 * 0x78-byte layout (a truncated 0x14-byte version computed id*0x14 instead
 * of id*0x78 — confirmed by the first matchdiff attempt) even though only
 * `model`/`position` are read here.
 * gp: Me_MOTION_C and dtL are gp-relative in this TU (tools/gpsyms.py) —
 * Me_MOTION_C was already in NowReturnNormal's list (same original TU);
 * dtL added here via `tools/gpsyms.py publish_ground_point_ --write`.
 */
extern Humanoid *Me_MOTION_C;

void publish_ground_point_(void)
{
    s32 id;

    id = (*Me_MOTION_C->model->object)->id;
    if (id >= 0)
    {
        dtL->vx = ConflictObject[id].position.vx;
        dtL->vz = ConflictObject[id].position.vz;
    }
}
