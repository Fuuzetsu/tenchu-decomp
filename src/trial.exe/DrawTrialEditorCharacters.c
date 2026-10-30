#include "common.h"
#include "mission_editor.h"
#include <psxsdk/libgte.h>

/* Project each visible placement onto the terrain viewport. The selected
 * placement blinks with its tile and receives a larger foreground portrait. */
void DrawTrialEditorCharacters(short cursor_x, short cursor_z,
                               short center_x, short center_z)
{
    MATRIX matrix;
    SVECTOR point;
    DVECTOR screen;
    short offset_z = -4;
    short offset_x = -6;
    short x, z, i, height;
    short placement_x, placement_z;
    int depth, scale;
    TrialPlacement *placements = TrialMission.placements;

    do
    {
        x = offset_x + center_x;
        z = offset_z + center_z;
        if (x >= 0 && x < TRIAL_MAP_WIDTH && z >= 0 && z < TRIAL_MAP_HEIGHT &&
            (x != cursor_x || z != cursor_z || (GameClock & 15) >= 4))
        {
            for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
            {
                if (placements[i].kind != CHARACTER_KIND_END &&
                    (placement_x = placements[i].x) == x &&
                    (placement_z = placements[i].z) == z)
                {
                    height = GetTrialTerrainHeight(placement_x, placement_z, TRIAL_HEIGHT_GROUND);
                    point.vx = offset_x * 250 - TrialTerrainHeightSkew * height;
                    point.vy = offset_z * 250 - TrialTerrainHeightSkew * height;
                    point.vz = height * TrialTerrainScale + 2000;
                    GsGetLs(&World.locate, &matrix);
                    GsSetLsMatrix(&matrix);
                    depth = (RotTransPers(&point, (long *)&screen, NULL, NULL) >> 2) - 30;
                    screen.vy += 8;
                    if (placement_x == cursor_x && placement_z == cursor_z)
                    {
                        depth = 10;
                        scale = 70;
                    }
                    else
                        scale = 50;
                    DrawTrialCharacterThumbnail(TrialMission.placements[i].kind,
                        screen.vx, screen.vy, depth, scale);
                    break;
                }
            }
        }
        offset_z += ++offset_x >= 7 ? (offset_x = -6, 1) : 0;
    } while (offset_z < 4);
}
