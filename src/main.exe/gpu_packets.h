#ifndef TENCHU_GPU_PACKETS_H
#define TENCHU_GPU_PACKETS_H

#include "common.h"
#include <psxsdk/libgpu.h>

/* The GPU consumes colour/code, screen XY, and texture/page data as packed
 * words even though PsyQ exposes their components as individual fields.
 * These dual views let arithmetic use named components while packet emission
 * copies the words the command stream actually carries. */
typedef union GpuColorWord GpuColorWord;
union GpuColorWord
{
    u_long word;
    CVECTOR channel;
}; /* 0x04 */

typedef union GpuScreenPosition GpuScreenPosition;
union GpuScreenPosition
{
    u_long word;
    DVECTOR component;
}; /* 0x04 */

typedef union GpuTextureWord GpuTextureWord;
union GpuTextureWord
{
    u_long word;
    u_short coordinates;
    struct
    {
        u_char u;
        u_char v;
        u_short metadata;
    } component;
}; /* 0x04 */

/* One complete textured-Gouraud vertex in the GPU command stream. */
typedef struct GpuTexturedGouraudVertex GpuTexturedGouraudVertex;
struct GpuTexturedGouraudVertex
{
    GpuColorWord color;
    GpuScreenPosition screen;
    GpuTextureWord texture;
}; /* 0x0C */

/* PsyQ's field view and the GPU's word-stream view of the same packets. */
typedef union GpuPolyGT3Packet GpuPolyGT3Packet;
union GpuPolyGT3Packet
{
    POLY_GT3 packet;
    struct
    {
        u_long tag;
        GpuTexturedGouraudVertex vertex[3];
    } gpu;
}; /* 0x28 */

typedef union GpuPolyGT4Packet GpuPolyGT4Packet;
union GpuPolyGT4Packet
{
    POLY_GT4 packet;
    struct
    {
        u_long tag;
        GpuTexturedGouraudVertex vertex[4];
    } gpu;
}; /* 0x34 */

#endif
