#include "common.h"
#include "mission_editor.h"

/* Heights for the ten terrain kinds (0x10 through 0xa0), in units of
 * TrialTerrainScale. The extreme entries also mark pits and tall obstacles. */
s16 TrialTerrainHeights[10] = { 0, -1, -2, -3, -4, -1, -2, -99, 99, 0 };

short GetTrialTerrainHeight(short x, short z, short layers)
{
    short height = 0;
    u8 tile;

    if (x < 0 || x >= TRIAL_MAP_WIDTH || z < 0 || z >= TRIAL_MAP_HEIGHT)
        return -99;
    if (layers & TRIAL_HEIGHT_GROUND)
    {
        tile = TrialEditorMap[z][x][0];
        if (tile != TRIAL_EMPTY_TILE)
            height = TrialTerrainHeights[
                ((TrialTileDefinitions[tile & TRIAL_TILE_INDEX_MASK].flags &
                  TRIAL_TILE_KIND_MASK) >> 4) - 1];
    }
    if (layers & TRIAL_HEIGHT_ORNAMENT)
    {
        tile = TrialEditorMap[z][x][1];
        if (tile != TRIAL_EMPTY_TILE)
            height += TrialTerrainHeights[
                ((TrialTileDefinitions[tile & TRIAL_TILE_INDEX_MASK].flags &
                  TRIAL_TILE_KIND_MASK) >> 4) - 1];
    }
    return __builtin_abs(height) > 99 ? (height > 0 ? 99 : -99) : height;
}
