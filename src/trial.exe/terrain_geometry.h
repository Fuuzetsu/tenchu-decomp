#ifndef TENCHU_TRIAL_TERRAIN_GEOMETRY_H
#define TENCHU_TRIAL_TERRAIN_GEOMETRY_H

#include "mission_editor.h"

enum { TRIAL_TERRAIN_UNIT_VERTEX_COUNT = 32, TRIAL_TERRAIN_FACE_CAPACITY = 22 };

/* Each face names a cardinal side (0..3), a top (4), or the end marker (-1).
 * The four vertex indices address rings of four corners at successive heights. */
typedef struct TrialTerrainFace
{
    s16 edge;
    u8 half, texture;
    u8 vertex[4];
} TrialTerrainFace;

extern SVECTOR TrialTerrainUnitVertices[TRIAL_TERRAIN_UNIT_VERTEX_COUNT];
extern TrialTerrainFace TrialTerrainFaces[10][TRIAL_TERRAIN_FACE_CAPACITY];
extern TrialTerrainFace TrialEditorFaces[10][TRIAL_TERRAIN_FACE_CAPACITY];

#endif
