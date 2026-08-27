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

#endif
