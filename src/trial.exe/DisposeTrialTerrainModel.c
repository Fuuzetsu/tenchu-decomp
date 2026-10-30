#include "common.h"
#include "mission_editor.h"
#include "../main.exe/tmdfile.h"

/* Terrain models own their generated vertices, primitives, and TMD header. */
void DisposeTrialTerrainModel(ModelType *model)
{
    TmdObjectRecord *object = (TmdObjectRecord *)model->object.tmd;

    vfree(object->linked.vertices);
    vfree(object->linked.primitives);
    vfree((u8 *)object - (sizeof(TMDFile) - sizeof(TmdObjectRecord)));
    DisposeModel(model);
    TrialTerrainOrnamentCount = 0;
}
