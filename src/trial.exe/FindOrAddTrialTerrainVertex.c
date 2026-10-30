#include "common.h"
#include "mission_editor.h"

/* Adjacent faces share vertices. Their fourth SVECTOR component carries
 * terrain shading, with height darkening and occasional darker speckles. */
short FindOrAddTrialTerrainVertex(short x, short y, short z)
{
    short i;

    x = TrialTerrainScale * x;
    z = TrialTerrainScale * z;
    y = (y != -6 ? y : -8) * TrialTerrainScale;
    for (i = 0; i < TrialTerrainVertexCount; i++)
    {
        if (TrialTerrainVertices[i].vx == x &&
            TrialTerrainVertices[i].vy == y &&
            TrialTerrainVertices[i].vz == z)
            return i;
    }
    TrialTerrainVertices[TrialTerrainVertexCount].vx = x;
    TrialTerrainVertices[TrialTerrainVertexCount].vy = y;
    TrialTerrainVertices[TrialTerrainVertexCount].vz = z;
    TrialTerrainVertices[TrialTerrainVertexCount].pad =
        (short)((rand() & 31) + 80) - __builtin_abs(y / TrialTerrainScale) * 8;
    if (rand() % 100 == 0)
        TrialTerrainVertices[TrialTerrainVertexCount].pad -= 16;
    return TrialTerrainVertexCount++;
}
