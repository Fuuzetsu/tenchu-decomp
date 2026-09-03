#ifndef TENCHU_FONT_H
#define TENCHU_FONT_H

enum font_character_code
{
    FONT_PRINTABLE_FIRST = 0x20,
    FONT_REMAP_TARGET = 0x27,
    FONT_REMAP_CODE = 0x92,
    FONT_UPPER_BLOCK_FIRST = 0xC0,
    FONT_NUDGE_EXEMPT = 0xC7,
    FONT_EXTENDED_BLOCK_FIRST = 0xE0,
    FONT_RAISED_CODE = 0xE7
};

enum font_layout_constant
{
    FONT_CODE_BLOCK_SIZE = 0x20,
    FONT_UPPER_BLOCK_OFFSET = 0x40,
    FONT_ATLAS_COLUMNS = 16,
    FONT_GLYPH_WIDTH = 3,
    FONT_GLYPH_HEIGHT = 16
};

enum font_vertical_nudge
{
    FONT_NUDGE_UPPER = -4,
    FONT_NUDGE_EXTENDED = -2,
    FONT_NUDGE_NONE = 0,
    FONT_NUDGE_RAISED = 3
};

/* SetupTelop rasterizes double-byte Shift-JIS glyphs into a private strip at
 * the bottom-right of VRAM. Each 16-pixel-wide cell stores fifteen glyph rows
 * in a 16x16 scratch tile, then adds a one-pixel dark outline before upload. */
enum telop_raster_layout
{
    TELOP_BITMAP_SIZE = 16,
    TELOP_GLYPH_ROWS = 15,
    TELOP_BYTES_PER_SJIS_GLYPH = 2,
    TELOP_VRAM_X = 0x300,
    TELOP_VRAM_BASE_Y = 0x1F0,
    TELOP_VRAM_STRIP_WIDTH = 0x100,
    TELOP_MAX_SJIS_GLYPHS = TELOP_VRAM_STRIP_WIDTH / TELOP_BITMAP_SIZE,
    TELOP_MAX_SJIS_BYTES =
        TELOP_MAX_SJIS_GLYPHS * TELOP_BYTES_PER_SJIS_GLYPH,
    TELOP_TEXTURE_U_ORIGIN = TELOP_VRAM_X + 1,
    TELOP_OUTLINE_FIRST_PIXEL = 1,
    TELOP_OUTLINE_X_LIMIT = TELOP_BITMAP_SIZE - 1,
    TELOP_OUTLINE_Y_LIMIT = TELOP_GLYPH_ROWS - 1
};

/* 0x8199 is Shift-JIS's white-star character. The game supplies its own
 * fifteen-row bitmap instead of asking the PlayStation KROM for it. */
enum telop_character_code
{
    TELOP_CUSTOM_STAR_SJIS = 0x8199,
    TELOP_CUSTOM_STAR_LEAD = TELOP_CUSTOM_STAR_SJIS >> 8,
    TELOP_CUSTOM_STAR_TRAIL = TELOP_CUSTOM_STAR_SJIS & 0xFF
};

/* 15-bit BGR pixels written directly into the VRAM staging strip. */
enum telop_pixel_color
{
    TELOP_PIXEL_TRANSPARENT = 0,
    TELOP_PIXEL_OUTLINE = 0x1CE7,
    TELOP_PIXEL_WHITE = 0x7FFF
};

extern u8 FontWidth[];
extern GsIMAGE FONT_IMAGE_;

extern void SetupTelop(u8 *telop, short line);
extern s32 telop_text_width_(u8 *text);
extern void draw_glyph_(GsOT_TAG *ot, s32 x, s32 y, u32 code);
extern void draw_telop_line_(GsOT_TAG *ot, s32 x, s32 y, u8 *text);
extern void load_font_image_into_global(GsIMAGE *image);
extern void initialise_font(void);

#endif
