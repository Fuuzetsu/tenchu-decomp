#include "common.h"
#include "mission.h"
#include "trial.exe.h"

/* Return the number of reserved slots (player plus objective actors).
 * A full reset clears patrols and positions the required actors near the center. */
int SetupTrialObjectivePlacements(short reset_positions)
{
    int objective_count = 0;
    TrialPlacement *placements = TrialMission.placements;
    short i;

    if (reset_positions)
    {
        for (i = 0; i < TRIAL_PLACEMENT_COUNT; i++)
        {
            placements[i].kind = -1;
            placements[i].route_last = 0;
            memset(placements[i].route, 0, sizeof(placements[i].route));
        }
        placements[0].x = 16;
        placements[0].kind = RIKIMARU_0;
        placements[0].z = 18;
    }
    switch (TrialMission.header.flags & TRIAL_OBJECTIVE_MASK)
    {
    case TRIAL_REACH_LORD:
        objective_count = 1;
        placements[1].kind = TONO;
        if (reset_positions)
        {
            placements[1].x = 16;
            placements[1].z = 14;
        }
        break;
    case TRIAL_ASSASSINATE:
    case TRIAL_ELIMINATE_ENEMIES:
        placements[1].kind = StageAppearance[TrialMission.header.theme + 1][2];
        objective_count = 1;
        if (reset_positions)
        {
            placements[1].x = 16;
            placements[1].z = 14;
        }
        break;
    case TRIAL_PROTECT_PRINCESS:
        placements[1].kind = TONO;
        placements[2].kind = HIME;
        objective_count = 2;
        if (reset_positions)
        {
            placements[1].x = 16;
            placements[1].z = 14;
            placements[2].x = 16;
            placements[2].z = 16;
        }
        break;
    case TRIAL_FIND_HERB:
        objective_count = 1;
        placements[1].kind = TRIAL_HERB_OBJECT;
        if (reset_positions)
        {
            placements[1].x = 16;
            placements[1].z = 14;
        }
        break;
    }
    return objective_count + 1;
}
