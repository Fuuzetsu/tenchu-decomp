#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DeleteConflict(struct ModelType *model);
 *     CONFLICT.C:310, 15 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t3       struct ModelType * model
 *     reg   $t1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

void DeleteConflict(ModelType *model)
{
    short i;
    short count;

    if (model->id != CONFLICT_NONE)
    {
        i = 0;
        while (i < ConflictObjects)
        {
            count = ConflictObjects - 1;
            if (ConflictObject[i].model == model)
            {
                ConflictObjects = count;
                ConflictObject[i] = ConflictObject[count];
                ConflictObject[i].model->id = i;
            }
            else
            {
                i++;
            }
        }
        model->id = CONFLICT_NONE;
        model->attribute = model->attribute & ~(MODEL_ATTR_CONFLICT | MODEL_ATTR_COLLIDE);
    }
}
