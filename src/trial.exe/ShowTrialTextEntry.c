#include "common.h"
#include "text_entry.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/tuning.h"
#include <psxsdk/libgpu.h>

/* The same keyboard edits mission titles, passwords, author names and records.
 * Its work buffer holds native u16 codes; the caller receives Shift-JIS bytes. */
void ShowTrialTextEntry(short mode)
{
    GsSPRITE underline;
    short phase = 1;
    short brightness = 128;
    int i;
    short trimming;

    ClearTrialDisplay(320, 480);
    ClearTrialPages();
    InitTrialTextEntry(mode);
    LoadTrialTextGraphics(mode);
    SetDispMask(1);
    DrawSync(0);
    VSync(0);
    while (phase)
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            brightness -= 16;
            if (brightness <= 0)
            {
                brightness = 0;
                phase++;
                TrialTextButtons = GetRealPad(0);
            }
            DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        case 2:
            if (!UpdateTrialTextEntry())
                phase++;
            break;
        case 3:
            brightness += 16;
            if (brightness >= 128)
            {
                brightness = 128;
                phase++;
            }
            DrawTrialScreenFade(brightness, 320, 240, 0);
            break;
        default:
            phase = 0;
            break;
        }
        DrawBG(TrialTextBackground);
        GsSortSprite(&TrialTextPanels->sprite, OTablePt, 10);
        GsSortSprite(&TrialTextHeading->sprite, OTablePt, 10);
        for (i = 0; i < 5; i++)
        {
            GsSortSprite(&TrialTextCategoryButtons[i]->sprite, OTablePt, 10);
            GsSortSprite(&TrialTextCategorySprites[i]->sprite, OTablePt, 9);
        }
        for (i = 0; i < 4; i++)
            GsSortSprite(&TrialTextShoulderButtons[i]->sprite, OTablePt, 15);
        GsSortPoly(TrialKeyboardQuad, OTablePt, 8);
        GsSortPoly(TrialEditedTextQuad, OTablePt, 8);
        GsSortSprite(&TrialKeyboardCursor->sprite->sprite, OTablePt, 5);
        underline = TrialEditedTextCursor->sprite->sprite;
        underline.x = -110;
        underline.r = underline.g = underline.b = 32;
        underline.attribute &= ~GS_ATTR_HIDDEN;
        for (i = 0; i < TrialEditedTextCursor->capacity; i++)
        {
            if (i == TrialEditedTextCursor->column)
                GsSortSprite(&TrialEditedTextCursor->sprite->sprite, OTablePt, 5);
            else
                GsSortSprite(&underline, OTablePt, 5);
            underline.x += 16;
        }
        for (i = 0; i < 2; i++)
            GsSortSprite(&TrialTextScrollArrows[i]->sprite, OTablePt, 8);
        SkipFrame = 2;
        EndDrawing(-2);
    }

    /* Drop trailing spaces, retaining spaces between entered characters. */
    trimming = 1;
    for (i = 14; i >= 0; i--)
    {
        if (trimming &&
            (TrialEditedTextCursor->characters[i] == TRIAL_SJIS_SPACE ||
             TrialEditedTextCursor->characters[i] == 0))
        {
            TrialTextEntry[i * 2] = TrialTextEntry[i * 2 + 1] = 0;
        }
        else if (trimming && TrialEditedTextCursor->characters[i] != TRIAL_SJIS_SPACE)
        {
            TrialTextEntry[i * 2] = TrialEditedTextCursor->characters[i] >> 8;
            TrialTextEntry[i * 2 + 1] = TrialEditedTextCursor->characters[i];
            trimming = 0;
        }
        else
        {
            TrialTextEntry[i * 2] = TrialEditedTextCursor->characters[i] >> 8;
            TrialTextEntry[i * 2 + 1] = TrialEditedTextCursor->characters[i];
        }
    }
    ClearTrialPages();
    SetDispMask(0);
    DisposeTrialTextEntry();
}
