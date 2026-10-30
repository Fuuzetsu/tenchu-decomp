#include "common.h"
#include "mission_editor.h"
#include "terrain_support.h"

static __inline__ short ValidateTileIndex(u8 tile, u16 kind_mask)
{
    short index = tile;
    if (index >= TrialTileDefinitionCount ||
        !(TrialTileDefinitions[index].flags & kind_mask))
        index = -1;
    return index;
}

/* Remove tile indices unsupported by the current theme. Ornaments need
 * regular ground, except disguised covers allowed over holes. */
void ValidateTrialEditorMap(void)
{
    short x, z;

    for (z = 0; z < TRIAL_MAP_HEIGHT; z++)
    {
        for (x = 0; x < TRIAL_MAP_WIDTH; x++)
        {
            if (TrialEditorMap[z][x][0] != TRIAL_EMPTY_TILE &&
                ValidateTileIndex(TrialEditorMap[z][x][0] & TRIAL_TILE_INDEX_MASK,
                                  TRIAL_TILE_KIND_MASK) == -1)
                TrialEditorMap[z][x][0] = TRIAL_EMPTY_TILE;
            if (TrialEditorMap[z][x][1] != TRIAL_EMPTY_TILE)
            {
                if (ValidateTileIndex(TrialEditorMap[z][x][1] & TRIAL_TILE_INDEX_MASK,
                                      TRIAL_TILE_ORNAMENT_MASK) != -1)
                {
                    if ((TrialTileDefinitions[TrialEditorMap[z][x][1] &
                                              TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_DISGUISED_ORNAMENT) &&
                        (TrialTileDefinitions[TrialEditorMap[z][x][0] &
                                              TRIAL_TILE_INDEX_MASK].flags &
                         TRIAL_TILE_KIND_MASK) == TRIAL_TERRAIN_HOLE)
                        continue;
                    if (ClassifyTrialTerrainSupport(TrialTileDefinitions[
                            TrialEditorMap[z][x][0] & TRIAL_TILE_INDEX_MASK].flags &
                            TRIAL_TILE_KIND_MASK) > 0)
                        continue;
                }
                TrialEditorMap[z][x][1] = TRIAL_EMPTY_TILE;
            }
        }
    }
    ValidateTrialPlacements();
}
