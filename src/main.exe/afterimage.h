#ifndef AFTERIMAGE_H
#define AFTERIMAGE_H

#include <psxsdk/libgpu.h>
#include "item.h"
#include "gpu_packets.h"

/* EFFECT.C's weapon-trail state. PSX.SYM supplies the complete record and
 * original names; SetupAfterimage, DrawAfterimage, and DisposeAfterimage
 * independently confirm every field and the 0x58-byte allocation. */
typedef struct AfterimageType
{
    ModelType *model; /* 0x00 */
    SVECTOR vector1;  /* 0x04 */
    SVECTOR vector2;  /* 0x0C */
    s16 maxn;         /* 0x14 */
    s16 n;            /* 0x16 */
    GpuScreenPosition *p1; /* 0x18: packed trail-edge screen points */
    GpuScreenPosition *p2; /* 0x1C */
    long sz;          /* 0x20 */
    GpuPolyGT4Packet poly; /* 0x24 */
} AfterimageType;     /* 0x58 */

extern GsIMAGE *AfterIMG;

AfterimageType *SetupAfterimage(ModelType *model, short len);
void DisposeAfterimage(AfterimageType *afi);
short DrawAfterimage(AfterimageType *afi, short disp);

/* MOTION.C expands this operation at the end of attacks and weapon-state
 * changes. The caller supplies the same weapon and cleanup values used by
 * its surrounding motion case. */
#define DISPOSE_WEAPON_AFTERIMAGE(human_, slot_)                             \
    if (human_->illusion[slot_] != 0)                                        \
    {                                                                        \
        DisposeAfterimage(human_->illusion[slot_]);                          \
        human_->illusion[slot_] = 0;                                         \
    }

#define CLEAR_WEAPON_ATTACK_EFFECTS(human_, weapon_, cleanup_)              \
    switch (weapon_)                                                         \
    {                                                                        \
    case FIST:                                                               \
        DeleteConflict(human_->model->object[MODEL_PART_ONININ_HAND_0]);     \
        DeleteConflict(human_->model->object[MODEL_PART_ONININ_HAND_1]);     \
        cleanup_ = ATTACK_CANCEL_ALL;                                        \
        break;                                                               \
    case JAW:                                                                \
        DeleteConflict(human_->model->object[MODEL_PART_BEAST_HAND_0]);      \
        cleanup_ = ATTACK_CANCEL_ALL;                                        \
        break;                                                               \
    case NO_WEAPON:                                                          \
        cleanup_ = ATTACK_CANCEL_ALL;                                        \
        break;                                                               \
    default:                                                                 \
        DeleteConflict(human_->model->object[MODEL_PART_WEAPON_HAND_0]);     \
        DeleteConflict(human_->model->object[MODEL_PART_WEAPON_HAND_1]);     \
        cleanup_ = ATTACK_CANCEL_ALL;                                        \
        break;                                                               \
    }                                                                        \
    if ((cleanup_ & ATTACK_CANCEL_AFTERIMAGES) != 0)                         \
    {                                                                        \
        DISPOSE_WEAPON_AFTERIMAGE(human_, WEAPON_HAND_0);                    \
        DISPOSE_WEAPON_AFTERIMAGE(human_, WEAPON_HAND_1);                    \
    }

#endif
