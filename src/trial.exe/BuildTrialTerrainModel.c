#include "common.h"
#include <stdlib.h>
#include "terrain_geometry.h"
#include "../main.exe/model.h"
#include "../main.exe/tmdfile.h"
#include "../main.exe/vmemory.h"

/* Eight height rings; each uses the same four X/Z corners. */
SVECTOR TrialTerrainUnitVertices[TRIAL_TERRAIN_UNIT_VERTEX_COUNT] = {
    { -1, 4, -1, 0 },
    { -1, 4, 1, 0 },
    { 1, 4, 1, 0 },
    { 1, 4, -1, 0 },
    { -1, 2, -1, 0 },
    { -1, 2, 1, 0 },
    { 1, 2, 1, 0 },
    { 1, 2, -1, 0 },
    { -1, 0, -1, 0 },
    { -1, 0, 1, 0 },
    { 1, 0, 1, 0 },
    { 1, 0, -1, 0 },
    { -1, -1, -1, 0 },
    { -1, -1, 1, 0 },
    { 1, -1, 1, 0 },
    { 1, -1, -1, 0 },
    { -1, -2, -1, 0 },
    { -1, -2, 1, 0 },
    { 1, -2, 1, 0 },
    { 1, -2, -1, 0 },
    { -1, -3, -1, 0 },
    { -1, -3, 1, 0 },
    { 1, -3, 1, 0 },
    { 1, -3, -1, 0 },
    { -1, -4, -1, 0 },
    { -1, -4, 1, 0 },
    { 1, -4, 1, 0 },
    { 1, -4, -1, 0 },
    { -1, -6, -1, 0 },
    { -1, -6, 1, 0 },
    { 1, -6, 1, 0 },
    { 1, -6, -1, 0 },
};

/* Mission faces split tall sides into texture half-height bands. */
TrialTerrainFace TrialTerrainFaces[10][TRIAL_TERRAIN_FACE_CAPACITY] = {
    { /* ground */
        { 4, 0, 1, { 9, 10, 8, 11 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 1 m ledge */
        { 4, 0, 1, { 13, 14, 12, 15 } },
        { 0, 2, 2, { 14, 13, 10, 9 } },
        { 1, 2, 3, { 15, 14, 11, 10 } },
        { 2, 2, 4, { 12, 15, 8, 11 } },
        { 3, 2, 5, { 13, 12, 9, 8 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 2 m ledge */
        { 4, 0, 1, { 17, 18, 16, 19 } },
        { 0, 2, 2, { 14, 13, 10, 9 } },
        { 1, 2, 3, { 15, 14, 11, 10 } },
        { 2, 2, 4, { 12, 15, 8, 11 } },
        { 3, 2, 5, { 13, 12, 9, 8 } },
        { 0, 1, 2, { 18, 17, 14, 13 } },
        { 1, 1, 3, { 19, 18, 15, 14 } },
        { 2, 1, 4, { 16, 19, 12, 15 } },
        { 3, 1, 5, { 17, 16, 13, 12 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 3 m ledge */
        { 4, 0, 1, { 21, 22, 20, 23 } },
        { 0, 2, 6, { 14, 13, 10, 9 } },
        { 1, 2, 7, { 15, 14, 11, 10 } },
        { 2, 2, 8, { 12, 15, 8, 11 } },
        { 3, 2, 9, { 13, 12, 9, 8 } },
        { 0, 1, 6, { 18, 17, 14, 13 } },
        { 1, 1, 7, { 19, 18, 15, 14 } },
        { 2, 1, 8, { 16, 19, 12, 15 } },
        { 3, 1, 9, { 17, 16, 13, 12 } },
        { 0, 2, 2, { 22, 21, 18, 17 } },
        { 1, 2, 3, { 23, 22, 19, 18 } },
        { 2, 2, 4, { 20, 23, 16, 19 } },
        { 3, 2, 5, { 21, 20, 17, 16 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 4 m ledge */
        { 4, 0, 1, { 25, 26, 24, 27 } },
        { 0, 2, 6, { 14, 13, 10, 9 } },
        { 1, 2, 7, { 15, 14, 11, 10 } },
        { 2, 2, 8, { 12, 15, 8, 11 } },
        { 3, 2, 9, { 13, 12, 9, 8 } },
        { 0, 1, 6, { 18, 17, 14, 13 } },
        { 1, 1, 7, { 19, 18, 15, 14 } },
        { 2, 1, 8, { 16, 19, 12, 15 } },
        { 3, 1, 9, { 17, 16, 13, 12 } },
        { 0, 2, 2, { 22, 21, 18, 17 } },
        { 1, 2, 3, { 23, 22, 19, 18 } },
        { 2, 2, 4, { 20, 23, 16, 19 } },
        { 3, 2, 5, { 21, 20, 17, 16 } },
        { 0, 1, 2, { 26, 25, 22, 21 } },
        { 1, 1, 3, { 27, 26, 23, 22 } },
        { 2, 1, 4, { 24, 27, 20, 23 } },
        { 3, 1, 5, { 25, 24, 21, 20 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 1 m ramp */
        { 4, 0, 1, { 13, 14, 8, 11 } },
        { 0, 2, 2, { 14, 13, 10, 9 } },
        { 1, 2, 3, { 15, 14, 11, 10 } },
        { 3, 2, 5, { 13, 12, 9, 8 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 2 m ramp */
        { 4, 0, 1, { 17, 18, 8, 11 } },
        { 0, 1, 2, { 18, 17, 14, 13 } },
        { 0, 2, 2, { 14, 13, 10, 9 } },
        { 1, 1, 3, { 19, 18, 15, 14 } },
        { 1, 2, 3, { 15, 14, 11, 10 } },
        { 3, 1, 5, { 17, 16, 13, 12 } },
        { 3, 2, 5, { 13, 12, 9, 8 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* wall */
        { 4, 0, 1, { 29, 30, 28, 31 } },
        { 0, 2, 6, { 14, 13, 10, 9 } },
        { 1, 2, 7, { 15, 14, 11, 10 } },
        { 2, 2, 8, { 12, 15, 8, 11 } },
        { 3, 2, 9, { 13, 12, 9, 8 } },
        { 0, 1, 6, { 18, 17, 14, 13 } },
        { 1, 1, 7, { 19, 18, 15, 14 } },
        { 2, 1, 8, { 16, 19, 12, 15 } },
        { 3, 1, 9, { 17, 16, 13, 12 } },
        { 0, 2, 2, { 22, 21, 18, 17 } },
        { 1, 2, 3, { 23, 22, 19, 18 } },
        { 2, 2, 4, { 20, 23, 16, 19 } },
        { 3, 2, 5, { 21, 20, 17, 16 } },
        { 0, 1, 2, { 26, 25, 22, 21 } },
        { 1, 1, 3, { 27, 26, 23, 22 } },
        { 2, 1, 4, { 24, 27, 20, 23 } },
        { 3, 1, 5, { 25, 24, 21, 20 } },
        { 0, 0, 2, { 30, 29, 26, 25 } },
        { 1, 0, 3, { 31, 30, 27, 26 } },
        { 2, 0, 4, { 28, 31, 24, 27 } },
        { 3, 0, 5, { 29, 28, 25, 24 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* hole */
        { 0, 0, 2, { 9, 10, 5, 6 } },
        { 1, 0, 3, { 10, 11, 6, 7 } },
        { 2, 0, 4, { 11, 8, 7, 4 } },
        { 3, 0, 5, { 8, 9, 4, 5 } },
        { 0, 0, 6, { 5, 6, 1, 2 } },
        { 1, 0, 7, { 6, 7, 2, 3 } },
        { 2, 0, 8, { 7, 4, 3, 0 } },
        { 3, 0, 9, { 4, 5, 0, 1 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* pond */
        { 4, 0, 1, { 9, 10, 8, 11 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
};

/* Editor faces merge those bands where possible for the small viewport. */
TrialTerrainFace TrialEditorFaces[10][TRIAL_TERRAIN_FACE_CAPACITY] = {
    { /* ground */
        { 4, 0, 1, { 9, 10, 8, 11 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 1 m ledge */
        { 4, 0, 1, { 13, 14, 12, 15 } },
        { 0, 2, 2, { 14, 13, 10, 9 } },
        { 1, 2, 3, { 15, 14, 11, 10 } },
        { 2, 2, 4, { 12, 15, 8, 11 } },
        { 3, 2, 5, { 13, 12, 9, 8 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 2 m ledge */
        { 4, 0, 1, { 17, 18, 16, 19 } },
        { 0, 0, 2, { 18, 17, 10, 9 } },
        { 1, 0, 3, { 19, 18, 11, 10 } },
        { 2, 0, 4, { 16, 19, 8, 11 } },
        { 3, 0, 5, { 17, 16, 9, 8 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 3 m ledge */
        { 4, 0, 1, { 21, 22, 20, 23 } },
        { 0, 0, 6, { 18, 17, 10, 9 } },
        { 1, 0, 7, { 19, 18, 11, 10 } },
        { 2, 0, 8, { 16, 19, 8, 11 } },
        { 3, 0, 9, { 17, 16, 9, 8 } },
        { 0, 2, 2, { 22, 21, 18, 17 } },
        { 1, 2, 3, { 23, 22, 19, 18 } },
        { 2, 2, 4, { 20, 23, 16, 19 } },
        { 3, 2, 5, { 21, 20, 17, 16 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 4 m ledge */
        { 4, 0, 1, { 25, 26, 24, 27 } },
        { 0, 0, 6, { 18, 17, 10, 9 } },
        { 1, 0, 7, { 19, 18, 11, 10 } },
        { 2, 0, 8, { 16, 19, 8, 11 } },
        { 3, 0, 9, { 17, 16, 9, 8 } },
        { 0, 0, 2, { 26, 25, 18, 17 } },
        { 1, 0, 3, { 27, 26, 19, 18 } },
        { 2, 0, 4, { 24, 27, 16, 19 } },
        { 3, 0, 5, { 25, 24, 17, 16 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 1 m ramp */
        { 4, 0, 1, { 13, 14, 8, 11 } },
        { 0, 2, 2, { 14, 13, 10, 9 } },
        { 1, 2, 3, { 15, 14, 11, 10 } },
        { 3, 2, 5, { 13, 12, 9, 8 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* 2 m ramp */
        { 4, 0, 1, { 17, 18, 8, 11 } },
        { 0, 0, 2, { 18, 17, 10, 9 } },
        { 1, 0, 3, { 19, 18, 11, 10 } },
        { 3, 0, 5, { 17, 16, 9, 8 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* wall */
        { 4, 0, 1, { 29, 30, 28, 31 } },
        { 0, 0, 6, { 18, 17, 10, 9 } },
        { 1, 0, 7, { 19, 18, 11, 10 } },
        { 2, 0, 8, { 16, 19, 8, 11 } },
        { 3, 0, 9, { 17, 16, 9, 8 } },
        { 0, 0, 2, { 26, 25, 18, 17 } },
        { 1, 0, 3, { 27, 26, 19, 18 } },
        { 2, 0, 4, { 24, 27, 16, 19 } },
        { 3, 0, 5, { 25, 24, 17, 16 } },
        { 0, 0, 2, { 30, 29, 26, 25 } },
        { 1, 0, 3, { 31, 30, 27, 26 } },
        { 2, 0, 4, { 28, 31, 24, 27 } },
        { 3, 0, 5, { 29, 28, 25, 24 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* hole */
        { 0, 0, 2, { 9, 10, 5, 6 } },
        { 1, 0, 3, { 10, 11, 6, 7 } },
        { 2, 0, 4, { 11, 8, 7, 4 } },
        { 3, 0, 5, { 8, 9, 4, 5 } },
        { 0, 0, 6, { 5, 6, 1, 2 } },
        { 1, 0, 7, { 6, 7, 2, 3 } },
        { 2, 0, 8, { 7, 4, 3, 0 } },
        { 3, 0, 9, { 4, 5, 0, 1 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
    { /* pond */
        { 4, 0, 1, { 9, 10, 8, 11 } },
        { -1, 0, 0, { 0, 0, 0, 0 } },
    },
};

/* Cardinal neighbors in map coordinates: north, east, south, west. */
TrialTerrainGridOffset TrialTerrainNeighborOffsets[4] = {
    { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
};

enum
{
    TRIAL_CORNER_MISSING_GROUND = 1,
    TRIAL_CORNER_LIGHT = 2,
    TRIAL_CORNER_HIGHER_NEIGHBOR = 4
};

/* Build a chunk in three passes: corner lighting, exposed textured faces,
 * and ornament instances. Heights and coordinates are in terrain units until
 * FindOrAddTrialTerrainVertex applies the current scale. */
ModelType *BuildTrialTerrainModel(short center_x, short center_z, short size)
{
    TmdTexturedGouraudQuadRecord primitive;
    /* Four neighboring heights followed by the current tile height. */
    struct { short edge[4]; short current; } heights;
    short min_x = center_x - size / 2, max_x = size + min_x;
    short min_z = center_z - size / 2, max_z = size + min_z;
    short x, z, neighbor_x, neighbor_z;
    short i, j, vertex, kind, edge;
    short *indices;
    u8 *cell;
    TrialTerrainFace *face;
    TMDFile *tmd;

    if (min_x < 0)
        min_x = 0;
    else if (max_x >= 32)
        max_x = 32;
    if (min_z < 0)
        min_z = 0;
    else if (max_z >= 32)
        max_z = 32;
    z = min_z;
    x = min_x;
    /* A cell needs at most 24 vertices and 21 textured faces. */
    TrialTerrainVertices = valloc((short)(size * size * 24) * sizeof(SVECTOR));
    TrialTerrainPrimitives = valloc((short)(size * size * 21) * sizeof(TmdTexturedGouraudQuadRecord));
    TrialTerrainVertexCount = TrialTerrainPrimitiveCount = 0;
    for (; z < max_z; z += ++x >= max_x ? (x = min_x, 1) : 0)
    {
        /* This short temporary carries the surface height for lighting. */
        vertex = GetTrialTerrainHeight(x, z, TRIAL_HEIGHT_GROUND);
        if (__builtin_abs(vertex) != 99 &&
            (TrialTileDefinitions[TrialEditorMap[z][x][0] & TRIAL_TILE_INDEX_MASK].flags &
             TRIAL_TILE_KIND_MASK) != TRIAL_TERRAIN_POND)
        {
            for (i = 0; i < 4; i++)
            {
                /* Before the face pass, kind accumulates corner-light flags:
                 * missing ground, a lamp, or a taller neighboring surface. */
                for (j = 0, kind = 0; j < 4; j++)
                {
                    /* The four cells sharing this corner: this cell, its
                     * two cardinal neighbors, and the diagonal neighbor. */
                    switch (j)
                    {
                    case 0: neighbor_x = x; neighbor_z = z; break;
                    case 1:
                        neighbor_x = x + TrialTerrainNeighborOffsets[i].z;
                        neighbor_z = z - TrialTerrainNeighborOffsets[i].x;
                        break;
                    case 2:
                        neighbor_x = x + TrialTerrainNeighborOffsets[(i + 1) % 4].z;
                        neighbor_z = z - TrialTerrainNeighborOffsets[(i + 1) % 4].x;
                        break;
                    case 3:
                        neighbor_x = x + TrialTerrainUnitVertices[i].vz;
                        neighbor_z = z + TrialTerrainUnitVertices[i].vx;
                        break;
                    }
                    if ((u16)neighbor_x >= 32 || neighbor_z < 0 || neighbor_z >= 32 ||
                        TrialEditorMap[neighbor_z][neighbor_x][0] == 0xff)
                        kind |= TRIAL_CORNER_MISSING_GROUND;
                    else if (TrialEditorMap[neighbor_z][neighbor_x][1] != 0xff &&
                             (TrialTileDefinitions[TrialEditorMap[neighbor_z][neighbor_x][1] &
                                                   TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_LIGHT))
                        kind |= TRIAL_CORNER_LIGHT;
                    else if ((short)GetTrialTerrainHeight(neighbor_x, neighbor_z,
                                TRIAL_HEIGHT_GROUND | TRIAL_HEIGHT_ORNAMENT) < vertex)
                        kind |= TRIAL_CORNER_HIGHER_NEIGHBOR;
                }
                if (kind)
                {
                    if (kind & TRIAL_CORNER_MISSING_GROUND)
                        j = 16;
                    else if (kind & TRIAL_CORNER_LIGHT)
                        j = (rand() & 15) | 192;
                    else
                        j = (short)((rand() & 15) + 48) - __builtin_abs(vertex) * 8;
                    TrialTerrainVertices[(short)FindOrAddTrialTerrainVertex(
                        (x - center_x) * 2 + TrialTerrainUnitVertices[i].vz, vertex,
                        (center_z - z) * 2 - TrialTerrainUnitVertices[i].vx)].pad = j;
                }
            }
        }
    }

    z = min_z;
    x = min_x;
    for (; z < max_z; z += ++x >= max_x ? (x = min_x, 1) : 0)
    {
        cell = TrialEditorMap[z][x];
        if (cell[0] != 0xff)
        {
            heights.current = GetTrialTerrainHeight(x, z, TRIAL_HEIGHT_GROUND);
            kind = TrialTileDefinitions[cell[0] & TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_KIND_MASK;
            for (i = 0; i < 4; i++)
            {
                neighbor_x = x + TrialTerrainNeighborOffsets[i].x;
                neighbor_z = z + TrialTerrainNeighborOffsets[i].z;
                heights.edge[i] = GetTrialTerrainHeight(neighbor_x, neighbor_z, TRIAL_HEIGHT_GROUND);
                /* Preserve the original read before the coordinate bounds check. */
                vertex = TrialEditorMap[neighbor_z][neighbor_x][0];
                if ((u16)neighbor_x >= 32 || neighbor_z < 0 || neighbor_z >= 32 || vertex == 0xff)
                {
                    if (kind == TRIAL_TERRAIN_HOLE)
                        heights.edge[i] = 99;
                }
                else
                {
                    vertex = TrialTileDefinitions[vertex & TRIAL_TILE_INDEX_MASK].flags & TRIAL_TILE_KIND_MASK;
                    /* Joined ramps hide their common edge only when their
                     * height, rotation, and slope direction permit it. */
                    if (kind == TRIAL_TERRAIN_RAMP_1M || kind == TRIAL_TERRAIN_RAMP_2M)
                    {
                        if ((vertex == TRIAL_TERRAIN_RAMP_1M || vertex == TRIAL_TERRAIN_RAMP_2M) &&
                            (kind != vertex ||
                             (cell[0] & 0xc0) != (TrialEditorMap[neighbor_z][neighbor_x][0] & 0xc0) ||
                             ((!TrialTerrainNeighborOffsets[i].x || ((cell[0] >> 6) & 1)) &&
                              (!TrialTerrainNeighborOffsets[i].z || !((cell[0] >> 6) & 1)))))
                        {
                            if ((cell[0] >> 6) != i || i != ((TrialEditorMap[neighbor_z][neighbor_x][0] + 128) >> 6 & 3))
                            {
                                /* Turning ramps and equal-height ramps each
                                 * expose the edge down to ground level. */
                                if ((cell[0] & 0xc0) != (TrialEditorMap[neighbor_z][neighbor_x][0] & 0xc0))
                                    heights.edge[i] = 0;
                                else if (kind == vertex)
                                    heights.edge[i] = 0;
                                else
                                    /* Parallel 1 m / 2 m ramps meet one level up. */
                                    heights.edge[i]++;
                            }
                        }
                    }
                    else if ((vertex == TRIAL_TERRAIN_RAMP_1M || vertex == TRIAL_TERRAIN_RAMP_2M) &&
                             (((TrialEditorMap[neighbor_z][neighbor_x][0] + 128) >> 6 & 3) != i))
                        heights.edge[i] = 0;
                }
            }
            kind = (kind >> 4) - 1;
            neighbor_x = (x - center_x) * 2;
            neighbor_z = (center_z - z) * 2;
            for (face = TrialTerrainFaces[kind]; face->edge != -1; face++)
            {
                edge = (face->edge + (cell[0] >> 6)) % 4;
                if (heights.current == 99)
                {
                    if (heights.edge[edge] == 99)
                        continue;
                }
                else if (face->edge != 4 &&
                    (heights.current >= heights.edge[edge] ||
                     TrialTerrainUnitVertices[face->vertex[0]].vy >= heights.edge[edge]))
                    continue;
                indices = (short *)&primitive.packet.v0;
                for (i = 0; i < 4; i++)
                {
                    vertex = face->vertex[i];
                    vertex = vertex - vertex % 4 + (vertex % 4 + (cell[0] >> 6)) % 4;
                    if (kind != (TRIAL_TERRAIN_POND >> 4) - 1)
                        indices[i] = FindOrAddTrialTerrainVertex(
                            TrialTerrainUnitVertices[vertex].vx + neighbor_x,
                            TrialTerrainUnitVertices[vertex].vy,
                            TrialTerrainUnitVertices[vertex].vz + neighbor_z);
                    else
                    {
                        /* Water is one tenth of a terrain unit below its
                         * surface; temporarily use a finer vertex scale. */
                        TrialTerrainScale /= 10;
                        indices[i] = FindOrAddTrialTerrainVertex(
                            (TrialTerrainUnitVertices[vertex].vx + neighbor_x) * 10,
                            TrialTerrainUnitVertices[vertex].vy * 10 + 1,
                            (TrialTerrainUnitVertices[vertex].vz + neighbor_z) * 10);
                        TrialTerrainScale *= 10;
                    }
                }
                if ((short)SetupTrialTerrainPrimitive(&primitive.packet,
                        TrialTileDefinitions[cell[0] & TRIAL_TILE_INDEX_MASK].data.textures[face->texture],
                        face->half) != -1)
                {
                    primitive.packet.r0 = primitive.packet.g0 = primitive.packet.b0 =
                        TrialTerrainVertices[(short)primitive.packet.v0].pad;
                    primitive.packet.r1 = primitive.packet.g1 = primitive.packet.b1 =
                        TrialTerrainVertices[(short)primitive.packet.v1].pad;
                    primitive.packet.r2 = primitive.packet.g2 = primitive.packet.b2 =
                        TrialTerrainVertices[(short)primitive.packet.v2].pad;
                    primitive.packet.r3 = primitive.packet.g3 = primitive.packet.b3 =
                        TrialTerrainVertices[(short)primitive.packet.v3].pad;
                    TrialTerrainPrimitives[TrialTerrainPrimitiveCount++] = primitive;
                }
            }
        }
    }

    z = min_z;
    TrialTerrainVertices = vmemoryGC(vrealloc(TrialTerrainVertices,
                                            TrialTerrainVertexCount * sizeof(SVECTOR)));
    x = min_x;
    TrialTerrainPrimitives = vmemoryGC(vrealloc(TrialTerrainPrimitives,
                                              TrialTerrainPrimitiveCount * sizeof(TmdTexturedGouraudQuadRecord)));
    tmd = vcalloc(sizeof(TMDFile), 0);
    tmd->id = 0x41;
    tmd->data.flags = tmd->data.object_count = 1;
    tmd->data.objects[0].linked.vertices = (VERT *)TrialTerrainVertices;
    tmd->data.objects[0].linked.vertex_count = TrialTerrainVertexCount;
    tmd->data.objects[0].linked.primitives = (TmdPrimitiveRecord *)TrialTerrainPrimitives;
    tmd->data.objects[0].linked.primitive_count = TrialTerrainPrimitiveCount;
    /* The fast terrain renderer stores the batch size in the first header. */
    *(u_long *)TrialTerrainPrimitives = (*(u_long *)TrialTerrainPrimitives & 0xffff0000) + TrialTerrainPrimitiveCount;
    for (; z < max_z; z += ++x >= max_x ? (x = min_x, 1) : 0)
    {
        if (TrialTerrainOrnamentCount >= TRIAL_TERRAIN_ORNAMENT_CAPACITY)
            break;
        cell = TrialEditorMap[z][x];
        if (cell[1] != 0xff)
        {
            i = GetTrialTerrainHeight(x, z, TRIAL_HEIGHT_GROUND);
            if (__builtin_abs(i) == 99)
                i = 0;
            TrialTerrainOrnaments[TrialTerrainOrnamentCount].x = (x - center_x) * (TrialTerrainScale << 1);
            TrialTerrainOrnaments[TrialTerrainOrnamentCount].z = (center_z - z) * (TrialTerrainScale << 1);
            TrialTerrainOrnaments[TrialTerrainOrnamentCount].y = i * TrialTerrainScale;
            TrialTerrainOrnaments[TrialTerrainOrnamentCount].model_rotation =
                (cell[1] & 0xc0) << 2 | TrialTileDefinitions[cell[1] & TRIAL_TILE_INDEX_MASK].data.ornament_index;
            TrialTerrainOrnamentCount++;
        }
    }
    return LoadModel((u_long *)tmd);
}
