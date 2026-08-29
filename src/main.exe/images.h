#ifndef TENCHU_IMAGES_H
#define TENCHU_IMAGES_H

/* GsSPRITE.attribute semi-transparency field (bit 30 = ABE enable,
 * bits 28-29 = blend rate): the two blends the game uses. Invented
 * names for the composite values. */
#define SPR_TRANS 0x40000000     /* translucency enable (rate 0: 50%+50%) */
#define SPR_TRANS_ADD 0x50000000 /* rate 1: back + front (additive) */
#define SPR_TRANS_SUB 0x60000000 /* rate 2: back - front (subtractive) */

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
    IMG_TENCHU = 61
};

extern void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply,
                                short x, short y);
extern void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply,
                                short x, short y);

#endif
