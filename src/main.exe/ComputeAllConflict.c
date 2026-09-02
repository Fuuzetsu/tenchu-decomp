#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ComputeAllConflict(void);
 *     CONFLICT.C:329, 50 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       struct ConflictObjectType * confop
 *     reg   $s0       struct ModelType * model
 *     stack sp+16     struct MATRIX mat
 *     reg   $s2       short i
 *     reg   $t0       short j
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct ModelType World;
 * END PSX.SYM */

extern void *memset(void *s, int c, u32 n);

void ComputeAllConflict(void)
{
    short i;
    short j;
    ModelType *model;
    ConflictObjectType *confop;
    MATRIX mat;
    int d;

    for (i = 0; i < ConflictObjects; i++)
    {
        confop = &ConflictObject[i];
        model = confop->model;
        if (model->attribute & MODEL_ATTR_COLLIDE)
        {
            memset(confop->result, 0, sizeof(confop->result));
            confop->offset.pad = 0;
            model->attribute &= ~MODEL_ATTR_CONFLICT;
            if (model->locate.super == &World.locate)
            {
                confop->position.vx = model->locate.coord.t[0] + confop->offset.vx;
                confop->position.vy = model->locate.coord.t[1] + confop->offset.vy;
                confop->position.vz = model->locate.coord.t[2] + confop->offset.vz;
            }
            else
            {
                GsGetLw(&model->locate, &mat);
                GsSetLsMatrix(&mat);
                RotTrans(&confop->offset, &confop->position, (long *)0);
            }
        }
    }

    for (i = 0; i < ConflictObjects; i++)
    {
        if (ConflictObject[i].model->attribute & MODEL_ATTR_COLLIDE)
        {
            for (j = i + 1; j < ConflictObjects; j++)
            {
                ConflictObjectType *other = &ConflictObject[j];

                if (other->model->attribute & MODEL_ATTR_COLLIDE)
                {
                    d = __builtin_abs(other->position.vy - ConflictObject[i].position.vy);
                    if (d <= ConflictObject[i].size.vy + other->size.vy)
                    {
                        d = __builtin_abs(other->position.vz - ConflictObject[i].position.vz);
                        if (d <= ConflictObject[i].size.vz + other->size.vz)
                        {
                            d = __builtin_abs(other->position.vx - ConflictObject[i].position.vx);
                            if (d <= ConflictObject[i].size.vx + other->size.vx)
                            {
                                ConflictObject[i].result[j] =
                                    other->size.pad |
                                    CONFLICT_LIVE;
                                ConflictObject[j].result[i] =
                                    ConflictObject[i].size.pad |
                                    CONFLICT_LIVE;
                                ConflictObject[i].model->attribute =
                                    ConflictObject[i].model->attribute | MODEL_ATTR_CONFLICT;
                                other->model->attribute = other->model->attribute | MODEL_ATTR_CONFLICT;
                                ConflictObject[i].offset.pad++;
                                other->offset.pad++;
                            }
                        }
                    }
                }
            }
        }
    }
}
