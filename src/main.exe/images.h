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

/* GetImage slots the code pins, named by what each becomes (invented
 * names): the afterimage texture, the water-splash sprite, the item
 * icon sheet, the title kanji poly, and the snowflake sprite. */
#define N_IMAGES 62

enum
{
    IMG_AFTERIMAGE = 0xA,
    IMG_SPLASH = 0xE,
    IMG_ITEM_ICONS = 0xF,
    IMG_TEN_LOGO = 0x2D,
    IMG_SNOW = 0x37
};

/* IMAGES.C's original CD location lead-in. */
enum
{
    OFFSET = 150
};

/* IMAGES.C's original archive identifiers. Retail reordered the archive;
 * these values are recovered where aligned demo/retail callers preserve the
 * asset's role. Keep unknown or retail-only entries numeric until their names
 * have comparable evidence. */
enum
{
    IMG_SMOKE = 6,
    IMG_BOMB0 = 7,
    IMG_GOSHIKIMAI = 13,
    IMG_LOADING = 44,
    IMG_KEHAI_GREEN = 46,
    IMG_KEHAI_YELLOW = 47,
    IMG_KEHAI_RED = 48,
    IMG_CURSOR = 50,
    IMG_FONT_NUMBER = 51,
    IMG_SIGHT = 52,
    IMG_SMOKE_ALT = 58,
    IMG_TENCHU = 61
};

extern void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply,
                                short x, short y);
extern void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply,
                                short x, short y);

#endif
