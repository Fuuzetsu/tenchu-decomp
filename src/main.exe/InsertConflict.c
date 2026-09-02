#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short InsertConflict(struct ModelType *model);
 *     CONFLICT.C:283, 23 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct ModelType * model
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR UnitVector2;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

extern char msg_conflict_regist_failure[]; /* "CONFLICT REGIST FAILURE" */

conflict_id InsertConflict(ModelType *model)
{
    u16 cnt;
    int idx;
    int id;

    id = model->id;
    if (id != CONFLICT_NONE)
    {
        return id;
    }
    if (ConflictObjects >= N_CONFLICT_OBJECTS)
    {
        SystemOut(msg_conflict_regist_failure);
    }
    cnt = ConflictObjects;
    ConflictObjects = cnt + 1;
    idx = (short)cnt;
    ConflictObject[idx].model = model;
    ConflictObject[idx].common = (void *)CONFLICT_OWNER_NONE;
    ConflictObject[idx].position = UnitVector2;
    ConflictObject[idx].offset = UnitVector;
    ConflictObject[idx].size = UnitVector;
    memset(ConflictObject[idx].result, 0, sizeof(ConflictObject[idx].result));
    model->id = cnt;
    model->attribute = (model->attribute | MODEL_ATTR_COLLIDE) & ~MODEL_ATTR_CONFLICT;
    return idx;
}
