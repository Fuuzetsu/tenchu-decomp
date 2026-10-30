#include "common.h"
#include "terrain_geometry.h"
#include "../main.exe/model.h"
#include "../main.exe/tmdfile.h"

extern SVECTOR TrialEditorTileVertices[32];
extern TMD_P_TNG4 TrialEditorTilePrimitives[21];
TMDFile TrialEditorTileTmd = {
    0x41, { TMD_FLAG_MAPPED, 1, {
        {{ (u_long *)TrialEditorTileVertices, 32, NULL, 0,
           (u_long *)TrialEditorTilePrimitives, 0, 0 }}
    }}
};

static s16 TrialTileCachedScale = -1;
static s16 TrialTileCachedSkew = 0;
static s16 TrialTileCachedBrightness = -1;
static s16 TrialTileCachedGround = 255;

/* A null cell initializes the reusable model. Otherwise, draw its ornament
 * and rebuild cached terrain geometry only when the tile or display changes. */
void DrawTrialEditorTile(u8 *cell, short *edge_heights)
{
    /* The original reserves this 32-byte local before the scale vector. */
    u8 reserved[32];
    VECTOR scale;
    short level, i, count;
    TrialTileDefinition *tile;
    TrialTerrainFace *face;
    TMD_P_TNG4 *primitive;
    OrnamentType *ornament;
    short vertex;
    int rotation;
    short *indices;

    if (!TrialEditorTileModel)
        TrialEditorTileModel = LoadModel((u_long *)&TrialEditorTileTmd);
    if (!cell)
        return;
    tile = cell[0] != 0xff ? &TrialTileDefinitions[cell[0] & TRIAL_TILE_INDEX_MASK] : NULL;
    /* Level is the terrain kind until the face table has been selected. */
    level = 0;
    if (tile)
        level = ((u8)tile->flags >> 4) - 1;
    if (cell[1] != 0xff)
    {
        ornament = TrialThemeOrnaments[
            TrialTileDefinitions[cell[1] & TRIAL_TILE_INDEX_MASK].data.ornament_index];
        ornament->locate.super = &TrialEditorTileModel->locate;
        /* The short height temporary becomes the loop index below. */
        i = TrialTerrainHeights[level];
        if (__builtin_abs(i) == 99)
            i = 0;
        /* Ornaments use the previous cached skew, before the terrain refresh. */
        ornament->locate.coord.t[0] = TrialTileCachedSkew * -i;
        ornament->locate.coord.t[2] = TrialTileCachedSkew * i;
        ornament->locate.coord.t[1] = TrialTerrainScale * i;
        scale.vx = scale.vy = scale.vz = (TrialTerrainScale << 12) / 1000;
        UpdateOrnament(ornament, (cell[1] >> 6) * 1024);
        ScaleMatrix(&ornament->locate.coord, &scale);
        DrawOrnament(ornament);
    }
    if (!tile)
        return;
    if (TrialTileCachedScale != TrialTerrainScale || TrialTileCachedSkew != TrialTerrainHeightSkew)
    {
        TrialTileCachedScale = TrialTerrainScale;
        TrialTileCachedSkew = TrialTerrainHeightSkew;
        for (i = 0; i < 32; i++)
        {
            TrialEditorTileVertices[i].vx = TrialTerrainUnitVertices[i].vx * TrialTerrainScale -
                TrialTerrainHeightSkew * TrialTerrainUnitVertices[i].vy;
            TrialEditorTileVertices[i].vy = TrialTerrainUnitVertices[i].vy * TrialTerrainScale;
            TrialEditorTileVertices[i].vz = TrialTerrainUnitVertices[i].vz * TrialTerrainScale +
                TrialTerrainHeightSkew * TrialTerrainUnitVertices[i].vy;
        }
    }
    if (edge_heights || TrialTileCachedBrightness != TrialTerrainBrightness ||
        TrialTileCachedGround != cell[0])
    {
        face = TrialEditorFaces[level];
        count = 0;
        primitive = TrialEditorTilePrimitives;
        TrialTileCachedGround = cell[0];
        TrialTileCachedBrightness = TrialTerrainBrightness;
        /* Reuse the level for the darker side-face brightness. */
        level = TrialTerrainBrightness * 2 / 3;
        for (; face->edge != -1; face++)
        {
            indices = (short *)&primitive->v0;
            rotation = (u8)TrialTileCachedGround >> 6;
            for (i = 0; i < 4; i++)
            {
                vertex = face->vertex[i];
                /* Rotate within the four corners of the same height ring. */
                indices[i] = vertex - vertex % 4 + (vertex % 4 + rotation) % 4;
            }
            if (!edge_heights || face->edge < 0 || face->edge > 3 ||
                TrialTerrainUnitVertices[(short)primitive->v2].vy > 0 ||
                TrialTerrainUnitVertices[(short)primitive->v0].vy <
                    edge_heights[(face->edge + ((u8)TrialTileCachedGround >> 6)) % 4])
            {
                if ((short)SetupTrialTerrainPrimitive(primitive,
                        tile->data.textures[face->texture], face->half) >= 0)
                {
                    if (TrialTerrainUnitVertices[(short)primitive->v0].vy <
                        TrialTerrainUnitVertices[(short)primitive->v2].vy)
                    {
                        primitive->r0 = primitive->g0 = primitive->b0 =
                            primitive->r1 = primitive->g1 = primitive->b1 =
                            primitive->r2 = primitive->g2 = primitive->b2 =
                            primitive->r3 = primitive->g3 = primitive->b3 = level;
                    }
                    primitive++;
                    count++;
                }
            }
        }
        TrialEditorTileTmd.data.objects[0].linked.primitive_count = count;
    }
    DrawModel(TrialEditorTileModel);
}
