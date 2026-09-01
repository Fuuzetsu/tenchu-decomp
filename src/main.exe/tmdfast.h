#ifndef TMDFAST_H
#define TMDFAST_H

/* DrawTMDmode: byte offset added into the per-primitive renderer
 * dispatch table — 0 selects the plain (tnf) bank, TMD_BANK_FOG the
 * depth-queued gradation (tng) bank the draw family switches to for
 * far objects (sz >= 300). */
/* A TMD primitive's mode byte (packet offset 3), as the decoders test it
 * after masking off ABE with & 0xfd. The varying bits are TEXTURED 0x04,
 * QUAD 0x08 and GOURAUD 0x10 over a common 0x21; the resulting eight
 * values are exactly the draw* / fast_tn?? family this file dispatches
 * to, which is what confirms the decode -- 0x3d reaches fast_tng4_,
 * 0x2d fast_tnf4_, 0x25 fast_tnf3_, 0x35 fast_tng3_. */
typedef u8 tmd_primitive_mode;
enum tmd_primitive_mode
{
    TMD_PRIM_F3 = 0x21,  /* flat, triangle */
    TMD_PRIM_FT3 = 0x25, /* flat, triangle, textured */
    TMD_PRIM_F4 = 0x29,  /* flat, quad */
    TMD_PRIM_FT4 = 0x2d, /* flat, quad, textured */
    TMD_PRIM_G3 = 0x31,  /* gouraud, triangle */
    TMD_PRIM_GT3 = 0x35, /* gouraud, triangle, textured */
    TMD_PRIM_G4 = 0x39,  /* gouraud, quad */
    TMD_PRIM_GT4 = 0x3d  /* gouraud, quad, textured */
};

typedef enum tmd_renderer_bank tmd_renderer_bank;
enum tmd_renderer_bank
{
    TMD_BANK_PLAIN = 0,
    TMD_BANK_FOG = 0x20
};

extern tmd_renderer_bank DrawTMDmode;

enum
{
    TMD_PRIMITIVE_MODE_MASK = 0xfd
};

#include "common.h"
#include <psxsdk/libgpu.h>
#include <psxsdk/libgs.h>

/* Header of one same-format run in the linked primitive stream. */
typedef struct
{
    u16 count;
    u8 dummy;
    tmd_primitive_mode mode;
} TmdPrimitiveBatch;

#define TMD_BATCH_BYTE_OFFSET(member) \
    ((u_long)&((TmdPrimitiveBatch *)0)->member)
#define TMD_BATCH_COUNT(primitive)                                  \
    (*(u_short *)((int)(primitive) + TMD_BATCH_BYTE_OFFSET(count)))
#define TMD_BATCH_MODE(primitive)                                  \
    (*(u_char *)((int)(primitive) + TMD_BATCH_BYTE_OFFSET(mode)))
#define TMD_RECORD_BYTES(type) ((int)sizeof(type))
#define TMD_RECORD_WORDS(type) ((int)(sizeof(type) / sizeof(u_long)))
#define TMD_NEXT_BATCH(primitive, type)                              \
    ((u_short *)((int)(primitive) + TMD_BATCH_COUNT(primitive) *     \
                                        TMD_RECORD_BYTES(type)))

/* Primitive vertex indices address Sony's packed VERT table. Keeping the
 * scaled index first also retains the original add operand order. */
#define TMD_VERTEX_AT(vertices, index)                               \
    ((VERT *)((index) * sizeof(VERT) + (u_long)(vertices)))

/*
 * Tenchu's own modified copies of the libgs linked-TMD renderers live in game
 * code at 0x80057b80..0x8005a7a4 (the stock SDK builds sit separately at
 * their library addresses, e.g. GsA4divTNF4/GsTMDfastTNF3).  The demo's
 * PSX.SYM does not cover this TU, so the functions carry descriptive
 * trailing-underscore names (the repo's invented-name convention; addresses
 * in each file header keep the identity); the data shapes below are
 * reconstructed from the matched bytes.
 *
 * Both renderer clusters run out of one caller-supplied scratch workspace
 * (GsSortObject4-style: ot, shift, scratch):
 *  - the "fast" cluster (decode_tmd_fast_ dispatching fast_tng4_/fast_tnf4_/
 *    fast_tnf3_/fast_tng3_) stages one whole output packet per primitive
 *    in the workspace, clip-tests it, then block-copies it to the packet
 *    list — TMD_FAST_WORK below;
 *  - the active-subdivision cluster (decode_tmd_adiv_ dispatching adiv_tng4_/
 *    adiv_tnf4_ into the recursive subdivide_quad_) keeps per-vertex records
 *    and a recursion stack there instead.
 */

/*
 * Workspace of the fast (non-dividing) renderers.  Quads stage their
 * POLY_GT4 at +0, the triangle pair stages its POLY_GT3 at +0x34; the
 * context fields from +0x5c are shared by all four.  The dispatcher
 * decode_tmd_fast_ fills the constants: farz 0x4a98, fogz 15000, and the
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

/* decode_tmd_fast_ must retain an integer workspace parameter, but its
 * scalar stores can still be derived from the typed context layout. */
#define TMD_FAST_BYTE_OFFSET(member) ((u_long)&((TMD_FAST_WORK *)0)->member)
#define TMD_FAST_WORD(work, member)                                        \
    (*(u_long *)((int)(work) + TMD_FAST_BYTE_OFFSET(member)))

enum
{
    TMD_FAST_FAR_Z = 0x4a98,
    TMD_FAST_FOG_Z = 15000
};

/* The renderers take (typed primitive records, Sony's VERT table, output
 * packet list, record count, work).  The dispatcher deliberately declares the
 * count parameter u_short at its call sites (the retail caller codegen
 * depends on it), while the definitions widen it to int — so the
 * prototypes live with the callers, not here. */

/*
 * One subdivision vertex of the active-subdivision cluster: object-space
 * position, (interpolated) colour with the primitive code byte, projected
 * screen XY/Z, and texture coordinates.  subdivide_quad_ averages two of
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
} ADIV_VERT;       /* 0x18 bytes */

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
} ADIV_FRAME;         /* 0x88 bytes */

/*
 * Workspace of the active-subdivision cluster (decode_tmd_adiv_ dispatching
 * adiv_tng4_/adiv_tnf4_ into the recursive subdivide_quad_).  The entry
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
    long limit;          /* 0x00 recursion depth limit (4) */
    long pad04;          /* 0x04 */
    long pad08;          /* 0x08 */
    long shift;          /* 0x0c OT bucket shift */
    u_long *org;         /* 0x10 ot->org */
    u_long *out;         /* 0x14 output packet cursor (the return value) */
    long zmax;           /* 0x18 max SZ, then the OTZ bucket index */
    long zmin;           /* 0x1c min SZ */
    long adivz;          /* 0x20 subdivide-when-nearer-than threshold (0x96) */
    long pad24;          /* 0x24 */
    long pad28;          /* 0x28 */
    short minx;          /* 0x2c screen extent of the current quad */
    short miny;          /* 0x2e */
    short maxx;          /* 0x30 */
    short maxy;          /* 0x32 */
    short adivw;         /* 0x34 HWD0/2 clip half-width */
    short adivh;         /* 0x36 VWD0/2 clip half-height */
    u_long *otp;         /* 0x38 cached OT slot */
    long pad3c[4];       /* 0x3c */
    POLY_GT4 packet;     /* 0x4c leaf-quad packet template (clut/tpage staged) */
    ADIV_VERT v[4];      /* 0x80 the root quad's vertices */
    ADIV_FRAME frame[1]; /* 0xe0 recursion frames, one per depth level */
} ADIV_WORK;

/* The entry renderers need scalar pointer arithmetic for retail scheduling,
 * but its constants can still be derived from the typed workspace map. */
#define ADIV_BYTE_OFFSET(member) ((u_long)&((ADIV_WORK *)0)->member)
#define ADIV_WORD_OFFSET(member) (ADIV_BYTE_OFFSET(member) / sizeof(u_long))
#define ADIV_BYTE(work, member)                                            \
    (*(u_char *)((int)(work) + ADIV_BYTE_OFFSET(member)))
#define ADIV_SHORT(work, member)                                          \
    (*(short *)((int)(work) + ADIV_BYTE_OFFSET(member)))
#define ADIV_WORD(work, member) ((work)[ADIV_WORD_OFFSET(member)])
#define ADIV_WORD_ADDRESS(work, member) ((work) + ADIV_WORD_OFFSET(member))

enum
{
    GPU_POLY_F3_CODE = 0x20,
    GPU_POLY_GT3_CODE = 0x34,
    GPU_POLY_GT4_CODE = 0x3c,
    GPU_POLY_F3_WORDS = sizeof(POLY_F3) / sizeof(u_long),
    GPU_POLY_GT3_WORDS = sizeof(POLY_GT3) / sizeof(u_long),
    GPU_POLY_GT4_WORDS = sizeof(POLY_GT4) / sizeof(u_long),
    GPU_POLY_F3_LENGTH = GPU_POLY_F3_WORDS - 1,
    GPU_POLY_GT3_LENGTH = GPU_POLY_GT3_WORDS - 1,
    GPU_POLY_GT4_LENGTH = GPU_POLY_GT4_WORDS - 1,
    GPU_DMA_ADDRESS_MASK = 0x00ffffff,
    GPU_DMA_TAG_GT3 = GPU_POLY_GT3_LENGTH << 24,
    GPU_DMA_TAG_GT4 = GPU_POLY_GT4_LENGTH << 24
};

#endif
