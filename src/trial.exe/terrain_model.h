#ifndef TENCHU_TRIAL_TERRAIN_MODEL_H
#define TENCHU_TRIAL_TERRAIN_MODEL_H

#include "mission_editor.h"
#include "../main.exe/tmdfile.h"
#include "../main.exe/model.h"

/* Terrain models own their generated vertices, primitives, and TMD header.
 * Chunk disposal inlines the same operation as the callable editor helper. */
#ifndef TRIAL_DEFINE_DISPOSE_TERRAIN
extern __inline__
#endif
void DisposeTrialTerrainModel(ModelType *model)
{
    TmdObjectRecord *object = (TmdObjectRecord *)model->object.tmd;

    vfree(object->linked.vertices);
    vfree(object->linked.primitives);
    vfree((u8 *)object - (sizeof(TMDFile) - sizeof(TmdObjectRecord)));
    DisposeModel(model);
    TrialTerrainOrnamentCount = 0;
}

#endif
