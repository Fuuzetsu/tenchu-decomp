#ifndef TENCHU_IMAGES_H
#define TENCHU_IMAGES_H

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
    IMG_TENCHU = 61
};

extern void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply,
                                short x, short y);
extern void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply,
                                short x, short y);

#endif
