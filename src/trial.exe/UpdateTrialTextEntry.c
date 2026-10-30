#include "common.h"
#include "text_entry.h"
#include "text_glyphs.h"
#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/padcmd.h"

extern u_long TrialTextNewButtons, TrialTextRepeatedButtons;
extern short TrialTextRepeatTicks, TrialTextCursorVisible;

enum
{
    TEXT_SOUND_DELETE = 12,
    TEXT_SOUND_INSERT = 13,
    TEXT_SOUND_MOVE = 18
};

static inline void RestoreTrialKanjiIndex(GsSPRITE *cursor)
{
    TrialKeyboardCursor->category = TrialKanjiIndexSelection.category;
    PopulateTrialKeyboard(&TrialKeyboardArea, 13, 5);
    cursor->x = TrialKanjiIndexSelection.cursor_x;
    cursor->y = TrialKanjiIndexSelection.cursor_y;
    TrialKeyboardCursor->column = TrialKanjiIndexSelection.column;
    TrialKeyboardCursor->row = TrialKanjiIndexSelection.row;
    TrialKeyboardCursor->current = TrialKeyboardCursor->characters +
        (TrialKeyboardCursor->row * 13 + TrialKeyboardCursor->column);
    TrialKanjiIndexSelection.active = 0;
}

short UpdateTrialTextEntry(void)
{
    RECT area;
    GsSPRITE *cursor;
    TrialGlyphPage *page;
    int category;
    u16 last_column;
    u8 range_index;
    short position;
    int i;

    TrialTextNewButtons = TrialTextButtons;
    TrialTextButtons = GetRealPad(0);
    TrialTextRepeatedButtons = TrialTextNewButtons & TrialTextButtons;
    TrialTextNewButtons = (TrialTextNewButtons ^ TrialTextButtons) & TrialTextButtons;
    cursor = &TrialKeyboardCursor->sprite->sprite;
    /* The original keeps this page for the whole update, including the frame
     * in which a shoulder button changes category. */
    page = &TrialTextPages[TrialKeyboardCursor->category];
    category = TrialKeyboardCursor->category;

    if (TrialTextNewButtons & PADL2)
    {
        if (TrialKeyboardCursor->category != TRIAL_TEXT_LATIN)
        {
            if (TrialKeyboardCursor->category == TRIAL_TEXT_KANJI)
            {
                TrialKeyboardCursor->category = TRIAL_TEXT_HIRAGANA;
                HighlightTrialTextCategory(TRIAL_TEXT_KANJI_INDEX, TRIAL_TEXT_HIRAGANA);
            }
            else
            {
                TrialKeyboardCursor->category = category - 1;
                HighlightTrialTextCategory(category, category - 1);
            }
            PopulateTrialKeyboard(&TrialKeyboardArea, 13, 5);
            TrialKanjiIndexSelection.active = 0;
            SoundEx(NULL, TRIAL_SOUND_CURSOR);
        }
    }
    else if (TrialTextNewButtons & PADR2)
    {
        if (TrialKeyboardCursor->category < TRIAL_TEXT_SYMBOLS)
        {
            TrialKeyboardCursor->category = category + 1;
            PopulateTrialKeyboard(&TrialKeyboardArea, 13, 5);
            HighlightTrialTextCategory(TrialKeyboardCursor->category - 1,
                                      TrialKeyboardCursor->category);
            TrialKanjiIndexSelection.active = 0;
            SoundEx(NULL, TRIAL_SOUND_CURSOR);
        }
        else if (TrialKeyboardCursor->category == TRIAL_TEXT_KANJI)
        {
            TrialKeyboardCursor->category = TRIAL_TEXT_SYMBOLS;
            PopulateTrialKeyboard(&TrialKeyboardArea, 13, 5);
            HighlightTrialTextCategory(TRIAL_TEXT_KANJI_INDEX, TRIAL_TEXT_SYMBOLS);
            TrialKanjiIndexSelection.active = 0;
            SoundEx(NULL, TRIAL_SOUND_CURSOR);
        }
    }

    if (TrialTextButtons & PADLup)
    {
        if ((TrialTextNewButtons & PADLup) ||
            (TrialTextRepeatedButtons == PADLup && TrialTextRepeatTicks > 5 && !(GameClock & 1)))
        {
            position = cursor->y - 19;
            if (position >= -30)
            {
                cursor->y = position;
                TrialKeyboardCursor->row--;
                TrialKeyboardCursor->current -= 13;
                SoundEx(NULL, TEXT_SOUND_MOVE);
            }
        }
        if (TrialTextRepeatedButtons == PADLup)
            TrialTextRepeatTicks++;
    }
    else if (TrialTextButtons & PADLdown)
    {
        if ((TrialTextNewButtons & PADLdown) ||
            (TrialTextRepeatedButtons == PADLdown && TrialTextRepeatTicks > 5 && !(GameClock & 1)))
        {
            position = cursor->y + 19;
            if (position < 47)
            {
                cursor->y = position;
                TrialKeyboardCursor->row++;
                TrialKeyboardCursor->current += 13;
                SoundEx(NULL, TEXT_SOUND_MOVE);
            }
        }
        if (TrialTextRepeatedButtons == PADLdown)
            TrialTextRepeatTicks++;
    }
    if (TrialTextButtons & PADLleft)
    {
        if ((TrialTextNewButtons & PADLleft) ||
            (TrialTextRepeatedButtons == PADLleft && TrialTextRepeatTicks > 5 && !(GameClock & 1)))
        {
            position = cursor->x - 18;
            if (position < -108)
            {
                if (page->first_visible_code == page->last_column_code)
                    goto repeat_left;
                if (TrialKeyboardCursor->category < TRIAL_TEXT_SYMBOLS)
                {
                    page->visible_offsets += 5;
                    page->range_indices += 5;
                    page->first_visible_code = page->code_base + *page->visible_offsets;
                }
                else if (TrialKeyboardCursor->category == TRIAL_TEXT_KANJI)
                {
                    if (page->column_index == page->range->column_count - 1)
                        SelectTrialKanjiRange(page->range->next, 1);
                    else
                    {
                        page->column_index++;
                        page->first_visible_code = page->range->column_starts[page->column_index];
                    }
                }
                else
                {
                    page->column_index++;
                    page->first_visible_code = page->range->column_starts[page->column_index];
                }
                PopulateTrialKeyboard(&TrialKeyboardArea, 13, 5);
                SoundEx(NULL, TEXT_SOUND_MOVE);
            }
            else
            {
                cursor->x = position;
                TrialKeyboardCursor->column++;
                TrialKeyboardCursor->current++;
                SoundEx(NULL, TEXT_SOUND_MOVE);
            }
        }
repeat_left:
        if (TrialTextRepeatedButtons == PADLleft)
            TrialTextRepeatTicks++;
    }
    if (TrialTextButtons & PADLright)
    {
        /* Right accepts other held buttons; the other directions require
         * their direction to be the only repeated button. */
        if ((TrialTextNewButtons & PADLright) ||
            ((TrialTextRepeatedButtons & PADLright) && TrialTextRepeatTicks > 5 && !(GameClock & 1)))
        {
            position = cursor->x + 18;
            if (position > 108)
            {
                if (page->first_visible_code == page->first_column_code)
                    goto repeat_right;
                if (TrialKeyboardCursor->category < TRIAL_TEXT_SYMBOLS)
                {
                    page->visible_offsets -= 5;
                    page->range_indices -= 5;
                    page->first_visible_code = page->code_base + *page->visible_offsets;
                }
                else if (TrialKeyboardCursor->category == TRIAL_TEXT_KANJI)
                {
                    if (page->column_index == 0)
                        SelectTrialKanjiRange(page->range->previous, 0);
                    else
                    {
                        page->column_index--;
                        page->first_visible_code = page->range->column_starts[page->column_index];
                    }
                }
                else
                {
                    page->column_index--;
                    page->first_visible_code = page->range->column_starts[page->column_index];
                }
                PopulateTrialKeyboard(&TrialKeyboardArea, 13, 5);
                SoundEx(NULL, TEXT_SOUND_MOVE);
            }
            else
            {
                cursor->x = position;
                TrialKeyboardCursor->column--;
                TrialKeyboardCursor->current--;
                SoundEx(NULL, TEXT_SOUND_MOVE);
            }
        }
repeat_right:
        if (TrialTextRepeatedButtons & PADLright)
            TrialTextRepeatTicks++;
    }

    if (TrialTextNewButtons & PADRright)
    {
        if (TrialKeyboardCursor->category == TRIAL_TEXT_KANJI_INDEX)
        {
            range_index = TrialTextPages[TrialKeyboardCursor->category].range_indices[
                TrialKeyboardCursor->column * 5 + TrialKeyboardCursor->row];
            if (range_index != 0xff)
            {
                TrialKanjiIndexSelection.category = TrialKeyboardCursor->category;
                TrialKanjiIndexSelection.column = TrialKeyboardCursor->column;
                TrialKanjiIndexSelection.row = TrialKeyboardCursor->row;
                TrialKanjiIndexSelection.cursor_x = cursor->x;
                TrialKanjiIndexSelection.cursor_y = cursor->y;
                TrialKanjiIndexSelection.active = 1;
                TrialKeyboardCursor->category = TRIAL_TEXT_KANJI;
                SelectTrialKanjiRange(&TrialKanjiRanges[range_index], 1);
                PopulateTrialKeyboard(&TrialKeyboardArea, 13, 5);
                TrialKeyboardCursor->column = TrialKeyboardCursor->row = 0;
                TrialKeyboardCursor->current = TrialKeyboardCursor->characters;
                cursor->x = 108;
                cursor->y = -30;
                last_column = TrialTextPages[TRIAL_TEXT_KANJI].range->column_count - 13 < 0 ?
                    0 : TrialTextPages[TRIAL_TEXT_KANJI].range->column_count - 13;
                TrialTextPages[TrialKeyboardCursor->category].first_column_code =
                    TrialTextPages[TrialKeyboardCursor->category].range->first_code;
                TrialTextPages[TrialKeyboardCursor->category].last_column_code =
                    TrialTextPages[TrialKeyboardCursor->category].range->column_starts[last_column];
            }
        }
        else
        {
            setRECT(&area, 640 + TrialEditedTextCursor->column * 16, 256, 16, 16);
            *TrialEditedTextCursor->current = *TrialKeyboardCursor->current;
            LoadTrialGlyphGrid(*TrialKeyboardCursor->current, &area, 1, 1);
            if (TrialEditedTextCursor->column < TrialEditedTextCursor->capacity - 1)
            {
                TrialEditedTextCursor->column++;
                TrialEditedTextCursor->current++;
                TrialEditedTextCursor->sprite->sprite.x += 16;
            }
            if (TrialKeyboardCursor->category == TRIAL_TEXT_KANJI && TrialKanjiIndexSelection.active)
                RestoreTrialKanjiIndex(cursor);
        }
        SoundEx(NULL, TEXT_SOUND_INSERT);
    }
    if (TrialTextNewButtons & PADRdown)
    {
        if (TrialEditedTextCursor->column == TrialEditedTextCursor->capacity - 1 &&
            TrialEditedTextCursor->characters[TrialEditedTextCursor->column] != TRIAL_SJIS_SPACE)
        {
            *TrialEditedTextCursor->current = TRIAL_SJIS_SPACE;
            setRECT(&area, 640 + TrialEditedTextCursor->column * 16, 256, 16, 16);
            LoadTrialGlyphGrid(TRIAL_SJIS_SPACE, &area, 1, 1);
        }
        else
        {
            char buffer[64];

            if (TrialEditedTextCursor->column == 0)
                goto after_delete;
            /* Preserve the original byte-string deletion of the u16 buffer. */
            memset(buffer, 0, sizeof(buffer));
            strncpy(buffer, (char *)TrialEditedTextCursor->characters,
                    (TrialEditedTextCursor->column - 1) * 2);
            strncat(buffer, (char *)TrialEditedTextCursor->current,
                    32 - TrialEditedTextCursor->column * 2);
            buffer[TrialEditedTextCursor->capacity * 2 - 2] = 0x40;
            buffer[TrialEditedTextCursor->capacity * 2 - 1] = 0x81;
            __builtin_memcpy(TrialEditedTextCursor->characters, buffer, 15 * sizeof(u16));
            TrialEditedTextCursor->column--;
            TrialEditedTextCursor->current--;
            TrialEditedTextCursor->sprite->sprite.x -= 16;
            for (i = 0; i < 15; i++)
            {
                setRECT(&area, 640 + i * 16, 256, 16, 16);
                LoadTrialGlyphGrid(TrialEditedTextCursor->characters[i], &area, 1, 1);
            }
        }
        SoundEx(NULL, TEXT_SOUND_DELETE);
    }
after_delete:
    if ((TrialTextNewButtons & PADRup) && TrialKeyboardCursor->category == TRIAL_TEXT_KANJI &&
        TrialKanjiIndexSelection.active)
        RestoreTrialKanjiIndex(cursor);

    if (page->first_visible_code != page->first_column_code)
        TrialTextScrollArrows[1]->sprite.attribute &= ~GS_ATTR_HIDDEN;
    else
        TrialTextScrollArrows[1]->sprite.attribute |= GS_ATTR_HIDDEN;
    if (page->first_visible_code != page->last_column_code)
        TrialTextScrollArrows[0]->sprite.attribute &= ~GS_ATTR_HIDDEN;
    else
        TrialTextScrollArrows[0]->sprite.attribute |= GS_ATTR_HIDDEN;
    if (!(TrialTextRepeatedButtons & (PADLup | PADLdown | PADLleft | PADLright)))
        TrialTextRepeatTicks = 0;
    if (TrialTextNewButtons & PADL1)
    {
        TrialEditedTextCursor->column--;
        if (TrialEditedTextCursor->column < 0)
            TrialEditedTextCursor->column = 0;
        else
        {
            TrialEditedTextCursor->current--;
            TrialEditedTextCursor->sprite->sprite.x -= 16;
        }
    }
    if ((TrialTextNewButtons & PADR1) && *TrialEditedTextCursor->current != 0)
    {
        TrialEditedTextCursor->column++;
        if (TrialEditedTextCursor->column > TrialEditedTextCursor->capacity - 1)
            TrialEditedTextCursor->column = TrialEditedTextCursor->capacity - 1;
        else
        {
            TrialEditedTextCursor->current++;
            TrialEditedTextCursor->sprite->sprite.x += 16;
        }
    }
    if (TrialTextButtons & (PADL1 | PADR1 | PADRright | PADRdown | PADRleft))
        TrialTextCursorVisible = 1;
    else if (GameClock % 15 == 0)
        TrialTextCursorVisible = !TrialTextCursorVisible;
    if (TrialTextCursorVisible)
        TrialEditedTextCursor->sprite->sprite.attribute &= ~GS_ATTR_HIDDEN;
    else
        TrialEditedTextCursor->sprite->sprite.attribute |= GS_ATTR_HIDDEN;
    if (TrialTextNewButtons & PADstart)
    {
        SoundEx(NULL, TRIAL_SOUND_CANCEL);
        return 0;
    }
    return 1;
}
