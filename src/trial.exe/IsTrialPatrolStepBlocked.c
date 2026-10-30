#include "common.h"
#include "mission_editor.h"

/* The high two bits of a map tile encode its rotation. Ramps
 * connect the base level to the first/second raised level only along that
 * direction. Reverse traversal uses the opposite cardinal direction. */
short IsTrialPatrolStepBlocked(short x, short z, short next_x, short next_z,
                              short facing)
{
    short from_kind, to_kind;

    if (TrialEditorMap[next_z][next_x][1] != TRIAL_EMPTY_TILE ||
        TrialEditorMap[next_z][next_x][0] == TRIAL_EMPTY_TILE)
        return 1;
    from_kind = TrialTileDefinitions[
        TrialEditorMap[z][x][0] & TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_KIND_MASK;
    to_kind = TrialTileDefinitions[
        TrialEditorMap[next_z][next_x][0] & TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_KIND_MASK;

    if ((from_kind == TRIAL_TERRAIN_GROUND || from_kind == TRIAL_TERRAIN_RAMP_1M || from_kind == TRIAL_TERRAIN_RAMP_2M) &&
        (to_kind == TRIAL_TERRAIN_RAMP_1M || to_kind == TRIAL_TERRAIN_RAMP_2M) &&
        facing == TrialEditorMap[next_z][next_x][0] >> 6)
        return 0;
    if (((from_kind == TRIAL_TERRAIN_RAMP_1M && to_kind == TRIAL_TERRAIN_LEDGE_1M) ||
         (from_kind == TRIAL_TERRAIN_RAMP_2M && to_kind == TRIAL_TERRAIN_LEDGE_2M)) &&
        facing == TrialEditorMap[z][x][0] >> 6)
        return 0;
    if (((to_kind == TRIAL_TERRAIN_RAMP_1M && from_kind == TRIAL_TERRAIN_LEDGE_1M) ||
         (to_kind == TRIAL_TERRAIN_RAMP_2M && from_kind == TRIAL_TERRAIN_LEDGE_2M)) &&
        (facing + 2) % 4 == TrialEditorMap[next_z][next_x][0] >> 6)
        return 0;
    if ((to_kind == TRIAL_TERRAIN_GROUND || to_kind == TRIAL_TERRAIN_RAMP_1M || to_kind == TRIAL_TERRAIN_RAMP_2M) &&
        (from_kind == TRIAL_TERRAIN_RAMP_1M || from_kind == TRIAL_TERRAIN_RAMP_2M) &&
        (facing + 2) % 4 == TrialEditorMap[z][x][0] >> 6)
        return 0;
    return from_kind != to_kind;
}
