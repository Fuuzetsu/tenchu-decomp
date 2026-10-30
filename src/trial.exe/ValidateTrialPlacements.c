#include "common.h"
#include "mission_editor.h"
#include "terrain_support.h"

/* Keep every placed character on clear, supported ground. A patrol ends
 * at its first obstructed walking step; waiting waypoints bypass that test. */
void ValidateTrialPlacements(void)
{
    TrialPlacement *placements = TrialMission.placements;
    short i, j, x, z;

    for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
    {
        if (placements[i].kind != -1)
        {
            TrialPatrolPoint *route;

            x = placements[i].x;
            z = placements[i].z;
            TrialEditorMap[z][x][1] = TRIAL_EMPTY_TILE;
            if (TrialEditorMap[z][x][0] == TRIAL_EMPTY_TILE ||
                ClassifyTrialTerrainSupport(TrialTileDefinitions[
                    TrialEditorMap[z][x][0] & TRIAL_TILE_INDEX_MASK].flags &
                    TRIAL_TILE_KIND_MASK) <= 0)
                TrialEditorMap[z][x][0] = 0;
            route = placements[i].route;
            for (j = 1; j <= placements[i].route_last; j++)
            {
                TrialPatrolPoint *point = (TrialPatrolPoint *)
                    (j * sizeof(*route) + (u_long)route);
                TrialPatrolPoint *previous = point - 1;
                if (previous->wait == 0 &&
                    IsTrialPatrolStepBlocked(previous->x, previous->z,
                                            point->x, point->z, previous->facing))
                    break;
            }
            placements[i].route_last = j - 1;
        }
    }
}
