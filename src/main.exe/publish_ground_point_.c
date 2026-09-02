#include "common.h"
#include "main.exe.h"
#include "item.h"

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
