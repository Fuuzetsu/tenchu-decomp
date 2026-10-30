#include "common.h"
#include "terrain_model.h"

/* Modes 1/2 build the full 32x32 map as sixty-four 4x4 chunks. Mode 2 also
 * installs collision; mode 0 releases the generated models and ornaments. */
void UpdateTrialTerrainChunks(short mode)
{
    short x, z, first_ornament;
    short chunk_size = 4;
    int center_x, center_z;
    ModelType *model;

    if (mode == 0)
    {
        z = x = 0;
        do
        {
            DisposeTrialTerrainModel(TrialTerrainChunks[z * TRIAL_TERRAIN_CHUNKS + x]);
            z += ++x >= TRIAL_TERRAIN_CHUNKS ? (x = 0, 1) : 0;
        } while (z < TRIAL_TERRAIN_CHUNKS);
        TrialTerrainOrnamentCount = 0;
    }
    else
    {
        TrialTerrainScale = 1000;
        z = x = 0;
        do
        {
            first_ornament = TrialTerrainOrnamentCount;
            center_x = x * chunk_size + chunk_size / 2;
            center_z = z * chunk_size + chunk_size / 2;
            model = BuildTrialTerrainModel(center_x, center_z, chunk_size);
            center_x = (center_x * 2 + 1) * TrialTerrainScale;
            center_z = (-center_z * 2 - 1) * TrialTerrainScale;
            model->locate.coord.t[0] = center_x;
            model->locate.coord.t[1] = 0;
            model->locate.coord.t[2] = center_z;
            model->object.attribute |= GS_DOBJ_DIVISION_DEPTH_BITS(2);
            model->attribute |= MODEL_ATTR_NOCULL;
            TrialTerrainChunks[z * TRIAL_TERRAIN_CHUNKS + x] = model;
            UpdateCoordinate2(model);
            for (; first_ornament < TrialTerrainOrnamentCount; first_ornament++)
            {
                TrialTerrainOrnaments[first_ornament].x += center_x;
                TrialTerrainOrnaments[first_ornament].z += center_z;
            }
            z += ++x >= TRIAL_TERRAIN_CHUNKS ? (x = 0, 1) : 0;
        } while (z < TRIAL_TERRAIN_CHUNKS);
        if (mode == 2)
            LoadAreaMap(BuildTrialCollisionMap());
    }
}
