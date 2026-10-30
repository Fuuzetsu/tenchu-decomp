#include "common.h"
#include "traps.h"

void InitTrialTrapMap(void)
{
    short x, z;
    u8 *cell;
    u16 flags;

    memset(TrialTrapMap, 0, sizeof(TrialTrapMap));
    z = x = 0;
    do
    {
        cell = (u8 *)TrialEditorMap +
            (x * sizeof(TrialMapCell) + z * TRIAL_MAP_WIDTH * sizeof(TrialMapCell));
        if (cell[0] != TRIAL_EMPTY_TILE)
        {
            flags = TrialTileDefinitions[cell[0] & TRIAL_TILE_INDEX_MASK].flags;
            if (flags & TRIAL_TILE_TRIGGER)
                TrialTrapMap[x + z * TRIAL_MAP_WIDTH] = TRIAL_TRAP_TRIGGER;
            else if (flags & TRIAL_TILE_SHOOTER)
                TrialTrapMap[x + z * TRIAL_MAP_WIDTH] =
                    (-GetTrialTerrainHeight(x, z, TRIAL_HEIGHT_GROUND) << 4) |
                    TRIAL_TRAP_SHOOTER | (cell[0] >> 6);
        }
        z += ++x >= TRIAL_MAP_WIDTH ? (x = 0, 1) : 0;
    } while (z < TRIAL_MAP_HEIGHT);
}
