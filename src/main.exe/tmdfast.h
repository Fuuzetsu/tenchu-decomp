#ifndef TMDFAST_H
#define TMDFAST_H

#include "common.h"
#include <psxsdk/libgpu.h>
#include <psxsdk/libgs.h>

/*
 * Tenchu's own modified copies of the libgs linked-TMD renderers live in game
 * code at 0x80057b80..0x8005a7a4 (the stock SDK builds sit separately at
 * their library addresses, e.g. GsA4divTNF4/GsTMDfastTNF3).  The demo's
 * PSX.SYM does not cover this TU, so the FUN_ names stay until real names are
 * recovered; the data shapes below are reconstructed from the matched bytes.
 *
 * Both renderer clusters run out of one caller-supplied scratch workspace
 * (GsSortObject4-style: ot, shift, scratch):
 *  - the "fast" cluster (FUN_800593a0 dispatching FUN_8005961c/FUN_80059b08/
 *    FUN_80059ff4/FUN_8005a3cc) stages one whole output packet per primitive
 *    in the workspace, clip-tests it, then block-copies it to the packet
 *    list — TMD_FAST_WORK below;
 *  - the active-subdivision cluster (FUN_80058a54 dispatching FUN_80058c70/
 *    FUN_80059008 into the recursive FUN_80057b80) keeps per-vertex records
 *    and a recursion stack there instead.
 */

/*
 * Workspace of the fast (non-dividing) renderers.  Quads stage their
 * POLY_GT4 at +0, the triangle pair stages its POLY_GT3 at +0x34; the
 * context fields from +0x5c are shared by all four.  The dispatcher
 * FUN_800593a0 fills the constants: farz 0x4a98, fogz 15000, and the
 * 320x240 screen clip box.
 */
typedef struct
{
    POLY_GT4 gt4;  /* 0x00 quad staging packet */
    POLY_GT3 gt3;  /* 0x34 triangle staging packet */
    long sz[4];    /* 0x5c per-vertex screen Z (gte_stsz3/gte_stsz4) */
    long pad6c[2]; /* 0x6c */
    long flag;     /* 0x74 GTE FLAG staging (RTPT/RTPS overflow reject) */
    long otz;      /* 0x78 OT bucket index (max SZ / 4) */
    long pad7c;    /* 0x7c */
    long opz;      /* 0x80 NCLIP outer product (backface reject) */
    long farz;     /* 0x84 far-Z reject threshold */
    long shift;    /* 0x88 OT bucket shift */
    long fogz;     /* 0x8c depth-cue (DPCS) start Z */
    GsOT *ot;      /* 0x90 output ordering table */
    long clipx0;   /* 0x94 screen clip box */
    long clipx1;   /* 0x98 */
    long clipy0;   /* 0x9c */
    long clipy1;   /* 0xa0 */
} TMD_FAST_WORK;

/* The renderers take (primitive stream, vertex-array base, output packet
 * list, record count, work).  The dispatcher deliberately declares the
 * count parameter u_short at its call sites (the retail caller codegen
 * depends on it), while the definitions widen it to int — so the
 * prototypes live with the callers, not here. */

/*
 * One subdivision vertex of the active-subdivision cluster: object-space
 * position, (interpolated) colour with the primitive code byte, projected
 * screen XY/Z, and texture coordinates.  FUN_80057b80 averages two of
 * these into an edge midpoint field by field.
 */
typedef struct
{
    SVECTOR pos;   /* 0x00 object-space vertex (pad unused) */
    CVECTOR col;   /* 0x08 colour + primitive code */
    DVECTOR sxy;   /* 0x0c projected screen XY */
    long sz;       /* 0x10 projected screen Z */
    u_char tu, tv; /* 0x14 texture coords */
    u_short pad;   /* 0x16 */
} ADIV_VERT; /* 0x18 bytes */

/*
 * One recursion level of the subdivider: the four corners of the quad under
 * subdivision (by pointer — corners are shared between levels) and the five
 * midpoints this level computes: edge 01, edge 02, edge 23, edge 31, and
 * the centre (the 03 diagonal of the vertex-strip order).  The next level's
 * frame follows immediately.
 */
typedef struct
{
    ADIV_VERT *vp[4]; /* 0x00 */
    ADIV_VERT mid[5]; /* 0x10 */
} ADIV_FRAME; /* 0x88 bytes */

/*
 * Workspace of the active-subdivision cluster (FUN_80058a54 dispatching
 * FUN_80058c70/FUN_80059008 into the recursive FUN_80057b80).  The entry
 * renderer fills the header and the four root vertices; the subdivider
 * walks frames from frame[0] down the remaining scratch.
 *
 * NOTE: the entry renderers must keep their index/cast spelling off the
 * u_long* scratch — struct-member stores un-pin the volatile parameter
 * reads they retail-interleave with (cookbook 3.13) — so this layout is
 * their documentation, matched by the per-store field comments.
 */
typedef struct
{
    long limit;      /* 0x00 recursion depth limit (4) */
    long pad04;      /* 0x04 */
    long pad08;      /* 0x08 */
    long shift;      /* 0x0c OT bucket shift */
    u_long *org;     /* 0x10 ot->org */
    u_long *out;     /* 0x14 output packet cursor (the return value) */
    long zmax;       /* 0x18 max SZ, then the OTZ bucket index */
    long zmin;       /* 0x1c min SZ */
    long adivz;      /* 0x20 subdivide-when-nearer-than threshold (0x96) */
    long pad24;      /* 0x24 */
    long pad28;      /* 0x28 */
    short minx;      /* 0x2c screen extent of the current quad */
    short miny;      /* 0x2e */
    short maxx;      /* 0x30 */
    short maxy;      /* 0x32 */
    short adivw;     /* 0x34 HWD0/2 clip half-width */
    short adivh;     /* 0x36 VWD0/2 clip half-height */
    u_long *otp;     /* 0x38 cached OT slot */
    long pad3c[4];   /* 0x3c */
    POLY_GT4 packet; /* 0x4c leaf-quad packet template (clut/tpage staged) */
    ADIV_VERT v[4];  /* 0x80 the root quad's vertices */
    ADIV_FRAME frame[1]; /* 0xe0 recursion frames, one per depth level */
} ADIV_WORK;

#endif
