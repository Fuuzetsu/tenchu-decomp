#include "common.h"
#include "terrain_geometry.h"

enum
{
    TRIAL_COLLISION_NODE_CAPACITY = TRIAL_MAP_WIDTH * TRIAL_MAP_HEIGHT +
                                   TRIAL_TERRAIN_ORNAMENT_CAPACITY,
    TRIAL_COLLISION_LEVEL_COUNT = 11,
    TRIAL_COLLISION_SOLID = 1,
    TRIAL_COLLISION_DISGUISE = 0x1000
};

typedef struct TrialCollisionData
{
    NodeIndexType levels[TRIAL_COLLISION_LEVEL_COUNT];
    AreaNodeType nodes[TRIAL_COLLISION_NODE_CAPACITY];
} TrialCollisionData;

extern TrialCollisionData TrialCollisionMap;

/* N/E/S/W edges of a node's 4x4 coverage mask. The central four cells
 * belong to a thin ornament; exposed edges extend that raised surface. */
static u16 TrialThinOrnamentEdgeMasks[];

/* Build and merge terrain surfaces, then serialize them by collision level. */
AreaMapType *BuildTrialCollisionMap(void)
{
    AreaNodeType nodes[TRIAL_COLLISION_NODE_CAPACITY];
    NodeIndexType *level = TrialCollisionMap.levels;
    AreaNodeType *node, *other;
    short x = 0;
    short i = 0;
    short z = TRIAL_MAP_HEIGHT - 1;
    short j, count;
    short neighbor_x, neighbor_z;
    short mask;
    short axis, merged;
    TrialCollisionData *map = &TrialCollisionMap;

    /* The collision map uses tenths of the renderer's world coordinates. */
    for (; z >= 0; z -= ++x >= TRIAL_MAP_WIDTH ? (x = 0, 1) : 0)
    {
        node = &nodes[i++];
        node->x1 = x * 200;
        node->x2 = x * 200 + 200;
        node->z2 = z * -200;
        node->z1 = z * -200 - 200;
        node->attribute = MAP_RESULT_FINAL;
        node->division = AREA_DIVISION_ALL;
        node->dy = 0;
        node->y = TrialEditorMap[z][x][0] != TRIAL_EMPTY_TILE ?
                  GetTrialTerrainHeight(x, z, TRIAL_HEIGHT_GROUND) : -99;
        switch (node->y)
        {
        case -99:
            i--;
            continue;
        case 99:
            node->y = 3000;
            node->attribute |= MAP_DEATH;
            break;
        default:
            node->y *= 100;
            switch (TrialTileDefinitions[TrialEditorMap[z][x][0] &
                                        TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_KIND_MASK)
            {
            case TRIAL_TERRAIN_RAMP_2M:
                node->dy += 100;
                /* Fall through: both ramp sizes share their axis and origin. */
            case TRIAL_TERRAIN_RAMP_1M:
                node->y = 0;
                node->dy += 100;
                merged = TrialEditorMap[z][x][0] >> 6;
                node->attribute |= TRIAL_COLLISION_SOLID |
                                   (merged & 1 ? MAP_SLOPE_X : MAP_SLOPE_Z);
                if (merged < 2)
                    node->dy = -node->dy;
                else
                    node->y -= node->dy;
                break;
            case TRIAL_TERRAIN_POND:
                node->attribute |= MAP_WATER;
                break;
            default:
                node->attribute |= TRIAL_COLLISION_SOLID;
                break;
            }
        }
        if (TrialEditorMap[z][x][1] != TRIAL_EMPTY_TILE)
        {
            if (TrialTileDefinitions[TrialEditorMap[z][x][1] & TRIAL_TILE_INDEX_MASK].flags &
                TRIAL_TILE_DISGUISED_ORNAMENT)
            {
                nodes[i] = *node;
                node = &nodes[i++];
                node->division = AREA_DIVISION_ALL;
                node->y = 0;
                node->attribute = MAP_RESULT_FINAL | TRIAL_COLLISION_DISGUISE |
                                  TRIAL_COLLISION_SOLID;
            }
            else if (TrialTileDefinitions[TrialEditorMap[z][x][1] & TRIAL_TILE_INDEX_MASK].flags &
                     TRIAL_TILE_ORNAMENT_THIN)
            {
                mask = 0x0660;
                axis = GetTrialTerrainHeight(x, z, TRIAL_HEIGHT_GROUND | TRIAL_HEIGHT_ORNAMENT);
                for (j = 0; j < 4; j++)
                {
                    neighbor_x = x + TrialTerrainNeighborOffsets[j].x;
                    neighbor_z = z + TrialTerrainNeighborOffsets[j].z;
                    if ((TrialEditorMap[neighbor_z][neighbor_x][1] == TRIAL_EMPTY_TILE ||
                         GetTrialTerrainHeight(neighbor_x, neighbor_z, TRIAL_HEIGHT_GROUND) * 100 < node->y ||
                         !(TrialTileDefinitions[TrialEditorMap[neighbor_z][neighbor_x][1] &
                                                TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_ORNAMENT_THIN)) &&
                        GetTrialTerrainHeight(neighbor_x, neighbor_z,
                                              TRIAL_HEIGHT_GROUND | TRIAL_HEIGHT_ORNAMENT) * 100 < node->y)
                        mask |= TrialThinOrnamentEdgeMasks[j];
                }
                if (mask != AREA_DIVISION_ALL)
                {
                    node->division = ~mask;
                    nodes[i] = *node;
                    node = &nodes[i++];
                    node->division = mask;
                    node->attribute = MAP_RESULT_FINAL | TRIAL_COLLISION_SOLID;
                }
                node->y = axis;
            }
            else
            {
                node->y = GetTrialTerrainHeight(x, z, TRIAL_HEIGHT_GROUND | TRIAL_HEIGHT_ORNAMENT);
                node->attribute = MAP_RESULT_FINAL | TRIAL_COLLISION_SOLID;
            }
            if (__builtin_abs(node->y) == 99)
                i--;
            else
                node->y *= 100;
        }
    }

    /* First double equal-size rectangles on alternating axes. A consumed
     * candidate ends this base node's scan on the following iteration. */
    count = i;
    axis = 0;
    do
    {
        axis = 1 - axis;
        for (merged = i = 0; i < count; i++)
        {
            if (nodes[i].division != AREA_DIVISION_ALL ||
                (nodes[i].attribute & TRIAL_COLLISION_DISGUISE) || nodes[i].dy != 0)
                continue;
            other = node = &nodes[i];
            for (j = i + 1; j < count; j++)
            {
                if (other->division == 0)
                    break;
                if (nodes[j].division != AREA_DIVISION_ALL ||
                    (nodes[j].attribute & TRIAL_COLLISION_DISGUISE) || nodes[j].dy != 0)
                    continue;
                other = &nodes[j];
                if (node->y == other->y &&
                    node->x2 - node->x1 == other->x2 - other->x1 &&
                    node->z2 - node->z1 == other->z2 - other->z1 &&
                    node->attribute == other->attribute)
                {
                    if (axis && node->x2 == other->x1 && node->z1 == other->z1)
                    {
                        merged++;
                        node->x2 = other->x2;
                        other->division = 0;
                    }
                    if (!axis && node->x1 == other->x1 && node->z2 == other->z1)
                    {
                        merged++;
                        node->z2 = other->z2;
                        other->division = 0;
                    }
                }
            }
        }
    } while (merged);

    /* Then merge unequal rectangles whenever their adjoining spans agree. */
    do
    {
        for (merged = i = 0; i < count; i++)
        {
            if (nodes[i].division != AREA_DIVISION_ALL ||
                (nodes[i].attribute & TRIAL_COLLISION_DISGUISE) || nodes[i].dy != 0)
                continue;
            other = node = &nodes[i];
            for (j = i + 1; j < count; j++)
            {
                if (other->division == 0)
                    break;
                if (nodes[j].division != AREA_DIVISION_ALL ||
                    (nodes[j].attribute & TRIAL_COLLISION_DISGUISE) || nodes[j].dy != 0)
                    continue;
                other = &nodes[j];
                if (node->y == other->y && node->attribute == other->attribute)
                {
                    if (node->z2 - node->z1 == other->z2 - other->z1 &&
                        node->z1 == other->z1 && node->x2 == other->x1)
                    {
                        merged++;
                        node->x2 = other->x2;
                        other->division = 0;
                    }
                    else if (node->x2 - node->x1 == other->x2 - other->x1 &&
                             node->x1 == other->x1 && node->z2 == other->z1)
                    {
                        merged++;
                        node->z2 = other->z2;
                        other->division = 0;
                    }
                }
            }
        }
    } while (merged);

    /* Emit levels -8..0 and the death floor at +30. The eleventh header
     * remains zero as the engine's index terminator. */
    memset(map, 0, sizeof(TrialCollisionMap));
    i = 0;
    for (axis = -8; axis <= 30; axis += axis < 0 ? 1 : 30)
    {
        level->n = 0;
        level->index = i;
        for (j = 0; j < count; j++)
        {
            if (nodes[j].division &&
                ((nodes[j].y == axis * 100 && nodes[j].dy == 0) ||
                 (axis == 0 && nodes[j].y <= 0)))
            {
                map->nodes[i] = nodes[j];
                if (map->nodes[i].attribute & MAP_WATER)
                    map->nodes[i].y += 10;
                nodes[j].division = 0;
                i++;
                level->n++;
            }
        }
        level->x1 = level->z1 = 32767;
        level->x2 = level->z2 = -32768;
        for (j = level->index; j < i; j++)
        {
            node = &map->nodes[j];
            if (node->x1 < level->x1) level->x1 = node->x1;
            if (node->x2 > level->x2) level->x2 = node->x2;
            if (node->z1 < level->z1) level->z1 = node->z1;
            if (node->z2 > level->z2) level->z2 = node->z2;
        }
        level->y = axis * 100;
        level->index = (level->index * sizeof(AreaNodeType)) + sizeof(TrialCollisionMap.levels);
        level++;
    }
    return (AreaMapType *)&TrialCollisionMap;
}

static u16 TrialThinOrnamentEdgeMasks[4] __attribute__((section(".data"))) = {
    0xf000, 0x8888, 0x000f, 0x1111
};
