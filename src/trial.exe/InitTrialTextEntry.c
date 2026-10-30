#include "common.h"
#include "text_entry.h"
#include "mission_menu.h"
#include <psxsdk/libapi.h>
#include <psxsdk/libgpu.h>

const char TrialTextArchivePath[] = "K:\\WORK\\CDIMAGE\\TRIAL\\IMAGE\\INPJP.ARC";

extern const u8 TrialLatinOffsets[], TrialKatakanaOffsets[], TrialHiraganaOffsets[];
extern const u8 TrialKanjiIndexOffsets[], TrialKanjiIndexRanges[];

typedef struct TrialRangeDefinition
{
    u16 columns, first, last;
} TrialRangeDefinition;

typedef struct TrialRangeDefinitions
{
    TrialRangeDefinition ranges[45];
} TrialRangeDefinitions;

const TrialRangeDefinitions TrialTextRangeDefinitions = {{
    {10, 0x889f, 0x88c7}, {13, 0x88c8, 0x8944}, {7, 0x8945, 0x895f},
    {12, 0x8960, 0x8996}, {8, 0x8997, 0x89b9}, {48, 0x89ba, 0x8ae8},
    {38, 0x8ae9, 0x8be2}, {11, 0x8be3, 0x8c53}, {23, 0x8c54, 0x8cc0},
    {36, 0x8cc1, 0x8db0}, {24, 0x8db1, 0x8e63}, {81, 0x8e64, 0x9078},
    {9, 0x9079, 0x90a1}, {24, 0x90a2, 0x9157}, {21, 0x9158, 0x91bb},
    {23, 0x91bc, 0x926b}, {19, 0x926c, 0x92c2}, {7, 0x92c3, 0x92df},
    {15, 0x92e0, 0x9364}, {25, 0x9365, 0x93dd}, {5, 0x93de, 0x93f0},
    {5, 0x93f1, 0x9446}, {2, 0x9447, 0x9447}, {4, 0x9448, 0x9453},
    {4, 0x9454, 0x9461}, {25, 0x9462, 0x94d8}, {19, 0x94d9, 0x9572},
    {15, 0x9573, 0x95b7}, {8, 0x95b8, 0x95da}, {21, 0x95db, 0x967e},
    {8, 0x9680, 0x96a0}, {5, 0x96a1, 0x96b0}, {3, 0x96b1, 0x96ba},
    {5, 0x96bb, 0x96cb}, {7, 0x96cc, 0x96e6}, {5, 0x96e7, 0x96f8},
    {8, 0x96f9, 0x975b}, {9, 0x975c, 0x9784}, {5, 0x9785, 0x9797},
    {15, 0x9798, 0x97d9}, {2, 0x97da, 0x97de}, {8, 0x97df, 0x9842},
    {7, 0x9843, 0x985f}, {13, 0x9860, 0x9872}, {30, 0x8141, 0x81f7}
}};

typedef struct TrialTextOffsetTables
{
    const u8 *table[4];
} TrialTextOffsetTables;

typedef struct TrialTextCategoryBounds
{
    u16 codes[4][2];
} TrialTextCategoryBounds;

const TrialTextOffsetTables TrialTextOffsets = {{
    TrialLatinOffsets, TrialKatakanaOffsets, TrialHiraganaOffsets, TrialKanjiIndexOffsets
}};
const TrialTextCategoryBounds TrialTextBounds = {{
    {0x824f, 2}, {0x8340, 6}, {0x829f, 6}, {0x829f, 0}
}};

/* Create the text-entry resources and page descriptors. BIOS font ranges
 * contain holes, so cache the first available glyph in each five-row column. */
void InitTrialTextEntry(short mode)
{
    int i, column, row;
    u16 code;
    u16 *starts;

    TrialTextArchive = FileRead(TrialTextArchivePath);
    TrialTextBackground = NULL;
    TrialTextPanels = NULL;
    for (i = 0; i < 5; i++)
    {
        TrialTextCategoryButtons[i] = NULL;
        TrialTextCategorySprites[i] = NULL;
    }
    for (i = 0; i < 4; i++)
        TrialTextShoulderButtons[i] = NULL;
    TrialTextHeading = NULL;
    TrialTextScrollArrows[0] = TrialTextScrollArrows[1] = NULL;
    TrialKeyboardQuad = NULL;
    TrialEditedTextQuad = NULL;

    TrialKeyboardCursor = valloc(sizeof(*TrialKeyboardCursor));
    TrialKeyboardCursor->characters = valloc(13 * 5 * sizeof(u16));
    TrialKeyboardCursor->current = TrialKeyboardCursor->characters;
    TrialKeyboardCursor->column = TrialKeyboardCursor->row = 0;
    TrialKeyboardCursor->category = TRIAL_TEXT_LATIN;
    TrialKeyboardCursor->sprite = NULL;
    TrialEditedTextCursor = valloc(sizeof(*TrialEditedTextCursor));
    TrialEditedTextCursor->characters = valloc(16 * sizeof(u16));
    TrialEditedTextCursor->current = TrialEditedTextCursor->characters;
    TrialEditedTextCursor->column = TrialEditedTextCursor->row = 0;
    TrialEditedTextCursor->sprite = NULL;
    TrialEditedTextCursor->category = mode;
    switch ((u16)mode)
    {
    case TRIAL_TEXT_TITLE:       TrialEditedTextCursor->capacity = 15; break;
    case TRIAL_TEXT_PASSWORD:    TrialEditedTextCursor->capacity = 8; break;
    case TRIAL_TEXT_AUTHOR:      TrialEditedTextCursor->capacity = 8; break;
    case TRIAL_TEXT_RECORD_NAME: TrialEditedTextCursor->capacity = 4; break;
    default:                    TrialEditedTextCursor->capacity = 15; break;
    }
    memset(TrialEditedTextCursor->characters, 0, 16 * sizeof(u16));
    for (i = 0; i < TrialEditedTextCursor->capacity; i++)
        TrialEditedTextCursor->characters[i] = TRIAL_SJIS_SPACE;
    for (i = 0; i < (int)strlen(TrialTextEntry) / 2; i++)
        TrialEditedTextCursor->characters[i] =
            (TrialTextEntry[i * 2] << 8) | TrialTextEntry[i * 2 + 1];
    setRECT(&TrialKeyboardArea, 640, 288, 240, 112);

    {
        /* Columns, first Shift-JIS code, last code. The final range is punctuation. */
        TrialRangeDefinitions definitions = TrialTextRangeDefinitions;

        TrialBlankGlyph = (u16 *)Krom2RawAdd2(TRIAL_SJIS_SPACE);
        for (i = 0; i < 45; i++)
        {
            column = 0;
            TrialKanjiRanges[i].column_count = definitions.ranges[i].columns;
            code = TrialKanjiRanges[i].first_code = definitions.ranges[i].first;
            TrialKanjiRanges[i].last_code = definitions.ranges[i].last;
            starts = TrialKanjiRanges[i].column_starts =
                valloc(TrialKanjiRanges[i].column_count * sizeof(*starts));
            for (; column < TrialKanjiRanges[i].column_count; column++)
            {
                for (row = 0; row < 5; row++)
                {
                    if (code <= 0x9872)
                    {
                        while ((u16 *)Krom2RawAdd2(code) == TrialBlankGlyph)
                            code++;
                        if (row == 0)
                            *starts = code;
                    }
                    else if (row == 0)
                        *starts = TRIAL_SJIS_SPACE;
                    code++;
                }
                starts++;
            }
            if (i != 44)
            {
                if (i == 0)
                {
                    TrialKanjiRanges[i].previous = NULL;
                    TrialKanjiRanges[i].next = &TrialKanjiRanges[i + 1];
                }
                else if (i == 43)
                {
                    TrialKanjiRanges[i].previous = &TrialKanjiRanges[i - 1];
                    TrialKanjiRanges[i].next = NULL;
                }
                else
                {
                    TrialKanjiRanges[i].previous = &TrialKanjiRanges[i - 1];
                    TrialKanjiRanges[i].next = &TrialKanjiRanges[i + 1];
                }
            }
        }
    }
    i = 0;
    {
        TrialTextOffsetTables offsets = TrialTextOffsets;
        TrialTextCategoryBounds bounds = TrialTextBounds;
        for (; i < 4; i++)
        {
            memset(&TrialTextPages[i], 0, sizeof(TrialTextPages[i]));
            TrialTextPages[i].code_base = bounds.codes[i][0];
            TrialTextPages[i].offsets = TrialTextPages[i].visible_offsets = offsets.table[i];
            TrialTextPages[i].range_indices = TrialKanjiIndexRanges;
            TrialTextPages[i].first_visible_code =
                TrialTextPages[i].code_base + *TrialTextPages[i].visible_offsets;
            TrialTextPages[i].first_column_code =
                TrialTextPages[i].code_base + *TrialTextPages[i].offsets;
            TrialTextPages[i].last_column_code =
                TrialTextPages[i].code_base + TrialTextPages[i].offsets[bounds.codes[i][1] * 5];
        }
    }
    memset(&TrialTextPages[TRIAL_TEXT_KANJI], 0, sizeof(TrialTextPages[TRIAL_TEXT_KANJI]));
    TrialTextPages[TRIAL_TEXT_KANJI].range = &TrialKanjiRanges[0];
    TrialTextPages[TRIAL_TEXT_KANJI].first_column_code = 0x889f;
    TrialTextPages[TRIAL_TEXT_KANJI].last_column_code = 0x9860;
    TrialTextPages[TRIAL_TEXT_KANJI].column_index = 0;
    TrialTextPages[TRIAL_TEXT_SYMBOLS].range = &TrialKanjiRanges[44];
    TrialTextPages[TRIAL_TEXT_SYMBOLS].column_index = 0;
    TrialTextPages[TRIAL_TEXT_SYMBOLS].first_column_code = 0x8141;
    TrialTextPages[TRIAL_TEXT_SYMBOLS].last_column_code = 0x8197;
    TrialKanjiIndexSelection.active = 0;
    TrialTextPages[TRIAL_TEXT_KANJI].code_base = TrialTextPages[TRIAL_TEXT_KANJI].first_visible_code =
        TrialTextPages[TRIAL_TEXT_KANJI].range->first_code;
    TrialTextPages[TRIAL_TEXT_SYMBOLS].code_base = TrialTextPages[TRIAL_TEXT_SYMBOLS].first_visible_code =
        TrialTextPages[TRIAL_TEXT_SYMBOLS].range->first_code;
}
