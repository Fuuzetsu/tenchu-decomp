#include "common.h"
#include "text_entry.h"
#include <psxsdk/libgpu.h>

extern const u16 TrialSpecialGlyphCodes[];

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/trial.exe/asm/nonmatchings/PopulateTrialKeyboard", PopulateTrialKeyboard);
#else
/* The 1,044-byte draft still differs from the 1,040-byte original in its
 * kanji-loop setup. The fixed-page path and the loop bodies are recovered. */

/* The four fixed pages use byte offsets; symbols and kanji use the BIOS
 * font ranges. Texture columns run right to left, while the cursor buffer
 * stores a conventional row-major grid. */
void PopulateTrialKeyboard(RECT *area, short columns, short rows)
{
    RECT rect = *area;
    int x, y, column;
    u16 category, code;
    u8 offset;
    const u8 *offsets;
    TrialKanjiRange *range;
    TrialGlyphPage *page;
    u16 *starts;

    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    rect.w = 16;
    rect.h = 15;
    rect.x += (columns - 1) * 18;
    category = TrialKeyboardCursor->category;
    switch (category)
    {
    case TRIAL_TEXT_LATIN:
    case TRIAL_TEXT_KATAKANA:
    case TRIAL_TEXT_HIRAGANA:
    case TRIAL_TEXT_KANJI_INDEX:
        offsets = TrialTextPages[category].visible_offsets;
        for (x = 0; x < columns; x++)
        {
            for (y = 0; y < rows; y++)
            {
                offset = offsets[rows * x + y];
                if (offset != 0xff)
                {
                    if (offset >= 100)
                        code = TrialSpecialGlyphCodes[offset - 100];
                    else
                        code = TrialTextPages[category].code_base + offset;
                }
                else
                    code = TRIAL_SJIS_SPACE;
                if (UploadTrialGlyph(code, &rect))
                {
                    if (columns != 1)
                        TrialKeyboardCursor->characters[x + y * columns] = code;
                    rect.y += 19;
                }
            }
            rect.x = (short)(rect.x - 2) - rect.w;
            rect.y = area->y;
        }
        break;
    case TRIAL_TEXT_SYMBOLS:
    case TRIAL_TEXT_KANJI:
        page = &TrialTextPages[category];
        range = page->range;
        column = page->column_index;
        starts = range->column_starts;
        for (x = 0; x < columns; x++)
        {
            if (column < range->column_count)
                code = starts[column];
            else
                code = TRIAL_SJIS_SPACE;
            for (y = 0; y < rows; y++)
            {
                if (code > range->last_code || code == TRIAL_SJIS_SPACE)
                {
                    UploadTrialGlyph(TRIAL_SJIS_SPACE, &rect);
                    if (columns != 1)
                        TrialKeyboardCursor->characters[x + y * columns] = TRIAL_SJIS_SPACE;
                }
                else
                {
                    while (!UploadTrialGlyph(code, &rect))
                        code++;
                    if (columns != 1)
                        TrialKeyboardCursor->characters[x + y * columns] = code;
                    code++;
                }
                rect.y += 19;
            }
            rect.x = (short)(rect.x - 2) - rect.w;
            rect.y = area->y;
            column++;
        }
        break;
    }
}

#endif
