#ifndef TENCHU_IMAGES_H
#define TENCHU_IMAGES_H

/* LIBGS attributes encode a TIM pixel mode in bits 24-25. For sprites,
 * bits 28-29 carry gpu_blend_mode and bit 30 enables semi-transparency.
 * GsSortSprite translates those fields into the GPU packet encoding. */
enum
{
    GS_ATTR_TEXTURE_MODE_SHIFT = 24,
    GS_ATTR_BLEND_MODE_SHIFT = 28
};

#define GS_ATTR_TEXTURE_MODE(mode) ((mode) << GS_ATTR_TEXTURE_MODE_SHIFT)
#define GS_ATTR_BLEND_MODE(mode) ((mode) << GS_ATTR_BLEND_MODE_SHIFT)
#define GS_ATTR_BLEND_MODE_MASK GS_ATTR_BLEND_MODE(GPU_BLEND_MODE_MASK)
#define GS_ATTR_SEMITRANS_ENABLE 0x40000000
#define GS_ATTR_SEMITRANS_MASK \
    (GS_ATTR_SEMITRANS_ENABLE | GS_ATTR_BLEND_MODE_MASK)
#define GS_ATTR_SEMITRANS(mode) \
    (GS_ATTR_SEMITRANS_ENABLE | GS_ATTR_BLEND_MODE(mode))

#define GS_ATTR_SEMITRANS_AVERAGE GS_ATTR_SEMITRANS(GPU_BLEND_AVERAGE)
#define GS_ATTR_SEMITRANS_ADD GS_ATTR_SEMITRANS(GPU_BLEND_ADD)
#define GS_ATTR_SEMITRANS_SUBTRACT GS_ATTR_SEMITRANS(GPU_BLEND_SUBTRACT)

/* IMAGES.C's original CD location lead-in. */
enum
{
    OFFSET = 150
};

extern GsIMAGE *GetImage(ImageArchiveId id);
extern void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply,
                                short x, short y);
extern void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply,
                                short x, short y);

#endif
