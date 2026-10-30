#include "common.h"
#include "mission_editor.h"
#include "../main.exe/model.h"

typedef struct TrialGridOffset
{
    s16 x, z;
} TrialGridOffset;

static TrialGridOffset TrialTerrainEdgeOffsets[4] = {
    { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
};

/* Draw the small terrain models around the viewport center. The cursor
 * cell blinks; nearby cells expose their edges, and ramp neighbors do not
 * hide the adjoining face at their one/two-unit base heights. */
void DrawTrialEditorTerrain(short cursor_x, short cursor_z,
                            short center_x, short center_z)
{
    short heights[4];
    short offset_z = -4;
    short offset_x = -6;
    short x, z, edge;
    u16 kind;
    ModelType *model = TrialEditorTileModel;

    TrialTerrainScale = 125;
    TrialTerrainBrightness = 128;
    TrialTerrainHeightSkew = -30;
    model->rotate.vx = 1024;
    model->rotate.vy = 0;
    model->rotate.vz = 0;
    do
    {
        x = offset_x + center_x;
        z = offset_z + center_z;
        if (x >= 0 && x < TRIAL_MAP_WIDTH && z >= 0 && z < TRIAL_MAP_HEIGHT &&
            (x != cursor_x || z != cursor_z || (GameClock & 15) >= 4))
        {
            model->locate.coord.t[0] = offset_x * 250;
            model->locate.coord.t[1] = offset_z * 250;
            model->locate.coord.t[2] = 2000;
            UpdateCoordinate2(model);
            for (edge = 0; edge < 4; edge++)
            {
                if (__builtin_abs(x - cursor_x) < 2 && __builtin_abs(z - cursor_z) < 2)
                    heights[edge] = 0;
                else
                {
                    heights[edge] = GetTrialTerrainHeight(
                        x + TrialTerrainEdgeOffsets[edge].x,
                        z + TrialTerrainEdgeOffsets[edge].z, TRIAL_HEIGHT_GROUND);
                    if ((u16)(heights[edge] + 2) < 2)
                    {
                        kind = TrialTileDefinitions[
                            TrialEditorMap[z + TrialTerrainEdgeOffsets[edge].z]
                                          [x + TrialTerrainEdgeOffsets[edge].x][0] &
                            TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_KIND_MASK;
                        if (kind == TRIAL_TERRAIN_RAMP_1M || kind == TRIAL_TERRAIN_RAMP_2M)
                            heights[edge] = 0;
                    }
                }
            }
            DrawTrialEditorTile(TrialEditorMap[z][x], heights);
        }
        offset_z += ++offset_x >= 7 ? (offset_x = -6, 1) : 0;
    } while (offset_z < 4);
}
