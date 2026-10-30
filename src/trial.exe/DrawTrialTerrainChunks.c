#include "common.h"
#include "mission_editor.h"
#include "../main.exe/tmdfast.h"
#include "../main.exe/item.h"

extern s32 SlightPoint;

typedef struct TrialVisibleChunk
{
    /* Positive index+1 uses subdivision; ~index uses the faster decoder. */
    s16 encoded_index;
    s16 depth;
} TrialVisibleChunk;

enum { TRIAL_VISIBLE_CHUNK_CAPACITY = 20, TRIAL_TERRAIN_PACKET_LIMIT = 0xde00 };

/* The mission/preview argument is unused in the original. Sort nearby
 * projected chunks front to back, then draw while enough packet space remains. */
void DrawTrialTerrainChunks(int mission)
{
    MATRIX matrix;
    SVECTOR screen;
    TrialVisibleChunk visible[TRIAL_VISIBLE_CHUNK_CAPACITY];
    short count = 0;
    short i, j, slot, camera_chunk;
    int x, z, depth, difference;
    ModelType *model;

    if (SkipFrame == 1)
        return;
    x = CamState.Owner->locate->vx;
    z = CamState.Owner->locate->vz;
    camera_chunk = ViewInfo.vpx / 8000 - ViewInfo.vpz / 8000 * TRIAL_TERRAIN_CHUNKS;
    for (i = 0; i < TRIAL_TERRAIN_CHUNKS * TRIAL_TERRAIN_CHUNKS; i++)
    {
        model = TrialTerrainChunks[i];
        if (__builtin_abs(model->locate.coord.t[0] - x) > 17000 ||
            __builtin_abs(model->locate.coord.t[2] - z) > 17000)
            continue;
        GsGetLs(&model->locate, &matrix);
        GsSetLsMatrix(&matrix);
        if (i != camera_chunk)
        {
            depth = RotTransPers(&UnitVector, (long *)&screen, NULL, NULL) >> 2;
            difference = __builtin_abs(i - camera_chunk);
            if (difference == 1 || difference == TRIAL_TERRAIN_CHUNKS)
            {
                depth = depth ? depth / 2 + 1 : 0;
                screen.vx /= 2;
            }
            screen.vx = __builtin_abs(screen.vx);
        }
        else
        {
            screen.vx = 1;
            depth = 1;
        }
        if (depth != 0 && depth <= 1550 && screen.vx <= 640)
        {
            if (depth > SlightPoint || screen.vx > 320)
                visible[count].encoded_index = ~i;
            else
                visible[count].encoded_index = i + 1;
            /* Keep the appended slot for its depth; reuse it in the sort below. */
            slot = count++;
            visible[slot].depth = depth;
        }
    }

    for (i = 0; i < count - 1; i++)
    {
        slot = i;
        depth = visible[i].depth;
        for (j = i + 1; j < count; j++)
        {
            if (visible[j].depth < depth)
            {
                slot = j;
                depth = visible[j].depth;
            }
        }
        if (slot != i)
        {
            /* Reuse the camera coordinates as the two swap temporaries. */
            x = visible[i].encoded_index;
            z = visible[i].depth;
            visible[i].encoded_index = visible[slot].encoded_index;
            visible[i].depth = visible[slot].depth;
            visible[slot].encoded_index = x;
            visible[slot].depth = z;
        }
    }

    DrawTMDmode = TMD_BANK_FOG;
    for (i = 0; i < count; i++)
    {
        int index = visible[i].encoded_index;
        model = TrialTerrainChunks[__builtin_abs(index) - 1];
        if ((u32)(GsGetWorkBase() - Packet[DrawingPage]) > TRIAL_TERRAIN_PACKET_LIMIT)
            return;
        GsGetLs(&model->locate, &matrix);
        GsSetLsMatrix(&matrix);
        if (index < 0)
            DrawTMD(&model->object, OTablePt, 0);
        else
            decode_tmd_adiv_(&model->object, OTablePt, 2,
                            (void *)TENCHU_SCRATCHPAD_ADDRESS);
    }
}
