#ifndef TENCHU_TRIAL_TEXT_ENTRY_H
#define TENCHU_TRIAL_TEXT_ENTRY_H

#include "trial.exe.h"

/* The keyboard and the edited text share a cursor/buffer layout. */
typedef struct TrialTextCursor
{
    s16 column, row;
    u16 category;
    s16 unknown6;
    u16 *characters;
    u16 *current;
    Sprite3D *sprite;
    u16 capacity;
    s16 unknown16;
} TrialTextCursor; /* 0x18 */

typedef struct TrialKanjiRange TrialKanjiRange;
struct TrialKanjiRange
{
    u16 column_count;
    u16 first_code, last_code;
    u16 padding;
    u16 *column_starts; /* each column contains five valid BIOS glyphs */
    TrialKanjiRange *previous, *next;
}; /* 0x14 */

typedef struct TrialGlyphPage
{
    u16 code_base, first_visible_code;
    u16 first_column_code, last_column_code;
    s16 column_index;
    s16 unknown0a;
    const u8 *offsets;
    const u8 *visible_offsets;
    const u8 *range_indices;
    TrialKanjiRange *range;
} TrialGlyphPage; /* 0x1c */

typedef struct TrialSavedKanjiIndex
{
    s16 cursor_x, cursor_y;
    s16 category, column, row;
    s16 active;
} TrialSavedKanjiIndex;

enum
{
    TRIAL_TEXT_LATIN = 0,
    TRIAL_TEXT_KATAKANA,
    TRIAL_TEXT_HIRAGANA,
    TRIAL_TEXT_KANJI_INDEX,
    TRIAL_TEXT_SYMBOLS,
    TRIAL_TEXT_KANJI,
    TRIAL_TEXT_CATEGORY_COUNT,

    TRIAL_SJIS_SPACE = 0x8140,
    TRIAL_GLYPH_INK = 0x0421,
    TRIAL_GLYPH_EDGE = 0x4210
};

extern TrialTextCursor *TrialKeyboardCursor;
extern TrialTextCursor *TrialEditedTextCursor;
extern u16 *TrialBlankGlyph;
extern TrialGlyphPage TrialKanjiPage;
extern TrialGlyphPage TrialTextPages[TRIAL_TEXT_CATEGORY_COUNT];
extern TrialKanjiRange TrialKanjiRanges[45];
extern BackGround *TrialTextBackground;
extern Sprite3D *TrialTextPanels;
extern Sprite3D *TrialTextHeading;
extern Sprite3D *TrialTextScrollArrows[2];
extern Sprite3D *TrialTextCategoryButtons[5];
extern Sprite3D *TrialTextCategorySprites[5];
extern Sprite3D *TrialTextShoulderButtons[4];
extern POLY_FT4 *TrialKeyboardQuad, *TrialEditedTextQuad;
extern ArcFile *TrialTextArchive;
extern u_long TrialTextButtons;
extern RECT TrialKeyboardArea, TrialEditedTextArea;
extern TrialSavedKanjiIndex TrialKanjiIndexSelection;

short UploadTrialGlyph(u16 code, RECT *area);
void LoadTrialGlyphGrid(u16 first_code, RECT *area, short columns, short rows);
void SelectTrialKanjiRange(TrialKanjiRange *range, short first_column);
void HighlightTrialTextCategory(short old_index, short new_index);
void DisposeTrialTextEntry(void);
void InitTrialTextEntry(short mode);
void LoadTrialTextGraphics(short mode);
void PopulateTrialKeyboard(RECT *area, short columns, short rows);
short UpdateTrialTextEntry(void);

#endif
