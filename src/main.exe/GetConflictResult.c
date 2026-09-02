#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetConflictResult(struct ModelType *model, short index);
 *     CONFLICT.C:382, 25 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * model
 *     param $a2       short index
 *     reg   $t0       short i
 *     reg   $t1       short idx
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR ConflictDistance;
 *     extern struct ModelType *ConflictModel;
 * END PSX.SYM */

conflict_id GetConflictResult(ModelType *model, conflict_id index)
{
    conflict_id idx;
    int id;
    short i;
    int k;

    id = model->id;
    idx = model->id;
    if (id != CONFLICT_NONE)
    {
        if ((model->attribute & MODEL_ATTR_COLLIDE) == 0)
        {
        ret_m1:
            return CONFLICT_NONE;
        }
        i = 0;
        if (index < 0)
        {
            index = 0;
            if (index >= ConflictObjects)
            {
                goto ret_m1;
            }
            for (; index < ConflictObjects; index++)
            {
                if (ConflictObject[id].result[index] != 0)
                {
                    i++;
                    if (i > ConflictObject[id].offset.pad)
                    {
                        goto ret_m1;
                    }
                    if ((ConflictObject[id].result[index] & CONFLICT_CONSUMED) == 0)
                    {
                        break;
                    }
                }
            }
        }
        k = index;
        if (k < ConflictObjects)
        {
            if (ConflictObject[idx].result[k] != 0)
            {
                ConflictObject[idx].result[k] |= CONFLICT_CONSUMED;
                ConflictModel = ConflictObject[k].model;
                ConflictDistance.vx = (short)ConflictObject[k].position.vx - (short)ConflictObject[idx].position.vx;
                ConflictDistance.vy = (short)ConflictObject[k].position.vy - (short)ConflictObject[idx].position.vy;
                ConflictDistance.vz = (short)ConflictObject[k].position.vz - (short)ConflictObject[idx].position.vz;
                return index;
            }
        }
        goto ret_m1;
    }
    return CONFLICT_NONE;
}
