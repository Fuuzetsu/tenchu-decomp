#include "common.h"
#include "options.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/font.h"
#include "../main.exe/memcard.h"
#include "../main.exe/music.h"
#include "../main.exe/padcmd.h"
#include "../main.exe/score.h"
#include "../main.exe/tim.h"
#include "../main.exe/vmemory.h"

extern char path_options_archive[];

void RunOptionsMenu(void)
{
    short fade = 128;
    short phase;
    short screen;
    short result;

    SetupOptionsImages(1);
    phase = 0;
    ClearMenuPages();
    SetDispMask(1);
    screen = 0;
    OptionsNextScreen = 0;
    OptionsSelection = 0;
    OptionHandlers[0](-1);
    do
    {
        StartDrawing();
        DrawBG(OptionsBackground);
        if (phase != 1)
        {
            OptionsButtons = -1;
            OptionsPreviousButtons = -1;
        }
        else
        {
            OptionsPreviousButtons = OptionsButtons;
            OptionsButtons = GetRealPad(0);
        }
        result = OptionHandlers[screen](0);
        if (result != 0)
        {
            phase = 4;
            if (result < 0)
                phase = 2;
        }
        if (phase == 0)
        {
            fade -= 8;
            if (fade <= 0)
            {
                phase = 1;
                fade = 0;
            }
            DrawMenuBackground(fade);
        }
        else if (phase == 2)
        {
            fade += 8;
            if (fade >= 128)
            {
                phase = 3;
                fade = 128;
            }
            DrawMenuBackground(fade);
        }
        SkipFrame = 2;
        EndDrawing(0);
        if (phase >= 3)
        {
            OptionHandlers[screen](1);
            screen = OptionsNextScreen;
            if (screen >= 0)
                OptionHandlers[screen](-1);
            phase -= 3;
        }
    } while (screen >= 0);
    ClearMenuPages();
    SetDispMask(0);
    SetupOptionsImages(0);
}

void SetupOptionsImages(short load)
{
    GsIMAGE image;
    short i;
    u_long *data;

    if (!load)
    {
        DisposeBG(OptionsBackground);
        vfree(OptionsHeaders[0]);
        vfree(OptionsHeaders[1]);
        vfree(OptionsHelp[0]);
        vfree(OptionsHelp[1]);
        vfree(OptionsSmallPanel);
        vfree(OptionsCheckmark);
        vfree(OptionsImageArchive);
        return;
    }
    OptionsImageArchive = FileRead(path_options_archive);
    data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_BACKGROUND);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    OptionsBackground = SetupBG(&image, SCREEN_W, SCREEN_H);
    OptionsBackground->sz = 10;
    for (i = OptionsBackground->map.ncellw * OptionsBackground->map.ncellh - 1;
         i >= 0; i--)
    {
        OptionsBackground->index[i] = i;
    }
    data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_HEADERS_1);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    OptionsHeaders[0] = SetupSprite(0, &image);
    OptionsHeaders[0]->sprite.h /= 6;
    OptionsHeaders[0]->sprite.my = image.ph / 12;
    OptionsHeaders[0]->id = OptionsHeaders[0]->sprite.v;
    OptionsHeaders[0]->sprite.y = -96;
    data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_HEADERS_2);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    OptionsHeaders[1] = SetupSprite(0, &image);
    OptionsHeaders[1]->sprite.h /= 5;
    OptionsHeaders[1]->sprite.my = image.ph / 10;
    OptionsHeaders[1]->id = OptionsHeaders[1]->sprite.v;
    OptionsHeaders[1]->sprite.y = -96;
    data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_CONFIRM_EXIT);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    OptionsHelp[0] = SetupSprite(0, &image);
    OptionsHelp[0]->sprite.y = 96;
    data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_CONFIRM_CANCEL);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    OptionsHelp[1] = SetupSprite(0, &image);
    OptionsHelp[1]->sprite.y = 96;
    data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_SMALL_PANEL);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    OptionsSmallPanel = SetupSprite(0, &image);
    data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_CHECKMARK);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    OptionsCheckmark = SetupSprite(0, &image);
    OptionsCheckmark->sprite.x = -38;
}

short UpdateOptionsList(short mode)
{
    GsIMAGE image;
    u_long *data;
    short i;
    int position;
    short coordinate;

    switch (mode)
    {
    case -1:
        OptionsHeaders[0]->sprite.v = OptionsHeaders[0]->id;
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_SWORD);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 10; i++)
        {
            if (i == 0)
                OptionSwords[0] = SetupSprite(0, &image);
            else
            {
                OptionSwords[i] = SetupSprite(OptionSwords[0], 0);
                if (i >= 5)
                    OptionSwords[i]->sprite.attribute |= 0x00C00000;
            }
            position = OptionSwords[i]->sprite.mx - 164;
            if (i >= 5)
                position = -position;
            OptionSwords[i]->clip.vx = position;
            OptionSwords[i]->sprite.y = (i % 5) * 34 - 68;
        }
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_CURSOR);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        OptionsCursor = SetupSprite(0, &image);
        OptionsCursor->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_MENU_LABELS);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 10; i++)
        {
            if (i == 0)
            {
                OptionLabels[0] = SetupSprite(0, &image);
                OptionLabels[0]->sprite.h /= 10;
                OptionLabels[0]->sprite.my = image.ph / 20;
            }
            else
                OptionLabels[i] = SetupSprite(OptionLabels[0], 0);
            OptionLabels[i]->sprite.v += (u_short)(image.ph / 10) * i;
            OptionLabels[i]->sprite.y = OptionSwords[i]->sprite.y + (i < 5 ? 2 : -2);
        }
        OptionsListSlide = 250;
        return 0;
    case 1:
        for (i = 0; i < 10; i++)
        {
            vfree(OptionSwords[i]);
            vfree(OptionLabels[i]);
        }
        vfree(OptionsCursor);
        return 0;
    default:

        if (OptionsListSlide != 0)
        {
            if (OptionsListSlide > 0)
            {
                OptionsListSlide -= 20;
                if (OptionsListSlide <= 0)
                    OptionsListSlide = 0;
                for (i = 0; i < 10; i++)
                {
                    if (i < 5)
                        OptionSwords[i]->sprite.x = OptionSwords[i]->clip.vx - OptionsListSlide;
                    else
                        OptionSwords[i]->sprite.x = OptionSwords[i]->clip.vx + OptionsListSlide;
                    OptionLabels[i]->sprite.x = OptionSwords[i]->sprite.x;
                }
            }
            else
            {
                OptionsListSlide += 20;
                if (OptionsListSlide >= 0)
                {
                    if (OptionsNextScreen == OPTIONS_RECORDS)
                    {
                        OptionsListSlide = 0;
                        return 0;
                    }
                    return 1;
                }
                for (i = 0; i < 10; i++)
                {
                    if (i < 5)
                    {
                        coordinate = OptionSwords[i]->clip.vx - 250;
                        OptionSwords[i]->sprite.x = coordinate - OptionsListSlide;
                    }
                    else
                    {
                        coordinate = OptionSwords[i]->clip.vx + 250;
                        OptionSwords[i]->sprite.x = OptionsListSlide + coordinate;
                    }
                    OptionLabels[i]->sprite.x = OptionSwords[i]->sprite.x;
                }
            }
        }
        if (OptionsListSlide == 0)
        {
            GsSortSprite(&OptionsHeaders[0]->sprite, OTablePt, 9);
            GsSortSprite(&OptionsHelp[0]->sprite, OTablePt, 9);
        }
        for (i = 0; i < 10; i++)
        {
            OptionLabels[i]->sprite.r = OptionLabels[i]->sprite.g =
                OptionLabels[i]->sprite.b = 63;
            OptionLabels[i]->sprite.attribute =
                (OptionLabels[i]->sprite.attribute & ~GS_ATTR_SEMITRANS_MASK) |
                GS_ATTR_SEMITRANS_SUBTRACT;
            GsSortSprite(&OptionLabels[i]->sprite, OTablePt, 7);
            if (i == OptionsSelection)
            {
                GsSortSprite(&OptionLabels[i]->sprite, OTablePt, 7);
                OptionSwords[i]->sprite.attribute &= ~GS_ATTR_SEMITRANS_ENABLE;
                OptionsCursor->sprite.x = OptionSwords[i]->sprite.x;
                OptionsCursor->sprite.y = OptionSwords[i]->sprite.y;
                OptionLabels[i]->sprite.r = 227;
                OptionLabels[i]->sprite.g = 0;
                OptionLabels[i]->sprite.b = 16;
            }
            else
            {
                OptionSwords[i]->sprite.attribute |= GS_ATTR_SEMITRANS_ENABLE;
                OptionLabels[i]->sprite.r = 34;
                OptionLabels[i]->sprite.g = 0;
                OptionLabels[i]->sprite.b = 87;
            }
            OptionLabels[i]->sprite.attribute =
                (OptionLabels[i]->sprite.attribute & ~GS_ATTR_SEMITRANS_MASK) |
                GS_ATTR_SEMITRANS_ADD;
            GsSortSprite(&OptionLabels[i]->sprite, OTablePt, 6);
            GsSortSprite(&OptionSwords[i]->sprite, OTablePt, 9);
        }
        GsSortSprite(&OptionsCursor->sprite, OTablePt, 8);
        if (OptionsPreviousButtons == 0 && OptionsListSlide == 0)
        {
            if (OptionsButtons & (PADLup | PADLdown | PADLleft | PADLright))
            {
                i = OptionsSelection;
                if (OptionsButtons == PADLup && OptionsSelection % 5 > 0)
                    OptionsSelection--;
                else if (OptionsButtons == PADLdown && OptionsSelection % 5 < 4)
                    OptionsSelection++;
                else if (OptionsButtons == PADLleft && OptionsSelection >= 5)
                    OptionsSelection -= 5;
                else if (OptionsButtons == PADLright && OptionsSelection < 5)
                    OptionsSelection += 5;
                if (i != OptionsSelection)
                    Sound(0, 3);
            }
            else if (OptionsButtons & (PADRright | PADRdown | PADstart))
            {
                Sound(0, 4);
                OptionsListSlide = -250;
                if (OptionsButtons == PADRright)
                {
                    OptionsNextScreen = OptionsSelection + 1;
                    if (OptionsNextScreen == OPTIONS_RECORDS)
                        return -1;
                }
                else if (OptionsButtons == PADRdown || OptionsButtons == PADstart)
                {
                    OptionsNextScreen = -1;
                    return -1;
                }
            }
        }
        return 0;
    }
}

short UpdateControlOptions(short mode)
{
    GsIMAGE image;
    u_long *data;
    short i;
    int language = LANG_JAPANESE;
    short y;

    switch (mode)
    {
    case -1:
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_CONTROLLER_PANEL);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        image.pw /= 2;
        ControlOptionPanels[0] = SetupSprite(0, &image);
        ControlOptionPanels[0]->sprite.x = -ControlOptionPanels[0]->sprite.mx;
        image.px += image.pw;
        ControlOptionPanels[1] = SetupSprite(0, &image);
        ControlOptionPanels[1]->sprite.x = ControlOptionPanels[1]->sprite.mx;
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_CONTROL_SCHEMES);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 4; i++)
        {
            if (i == 0)
            {
                ControlSchemeLabels[0] = SetupSprite(0, &image);
                ControlSchemeLabels[0]->sprite.h /= 4;
                ControlSchemeLabels[0]->sprite.my = image.ph / 8;
                ControlSchemeLabels[0]->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
            }
            else
                ControlSchemeLabels[i] = SetupSprite(ControlSchemeLabels[0], 0);
            ControlSchemeLabels[i]->sprite.v += (u_short)(image.ph / 4) * i;
            ControlSchemeLabels[i]->sprite.y = ControlSchemeLabels[i]->clip.vy = -62;
        }
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_CONTROL_ACTIONS);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 6; i++)
        {
            if (i == 0)
            {
                ControlActionLabels[0] = SetupSprite(0, &image);
                ControlActionLabels[0]->sprite.h /= 6;
                ControlActionLabels[0]->sprite.my = image.ph / 12;
                ControlActionLabels[0]->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
            }
            else
                ControlActionLabels[i] = SetupSprite(ControlActionLabels[0], 0);
            ControlActionLabels[i]->sprite.v += (u_short)(image.ph / 6) * i;
        }
        ControlOptionSlide = 250;
        ControlOptionScheme = MENU_STATE->control_scheme;
        break;
    case 1:
        vfree(ControlOptionPanels[0]);
        vfree(ControlOptionPanels[1]);
        i = 0;
        while (i < 4)
            vfree(ControlSchemeLabels[i++]);
        i = 0;
        while (i < 6)
            vfree(ControlActionLabels[i++]);
        break;
    default:
        if (ControlOptionSlide != 0)
        {
            if (ControlOptionSlide > 0)
            {
                ControlOptionSlide -= 20;
                if (ControlOptionSlide <= 0)
                    ControlOptionSlide = 0;
                ControlOptionPanels[0]->sprite.y = -ControlOptionSlide;
                ControlOptionPanels[1]->sprite.y = -ControlOptionSlide;
                ControlSchemeLabels[ControlOptionScheme]->sprite.y =
                    ControlSchemeLabels[ControlOptionScheme]->clip.vy - ControlOptionSlide;
                for (i = 0; i < 8; i++)
                {
                    GsSPRITE *sprite = &ControlActionLabels[ControlLabelActions[ControlOptionScheme][i]]->sprite;
                    sprite->x = ControlLabelPositions[i][language].x;
                    sprite->y = ControlLabelPositions[i][language].y - ControlOptionSlide;
                    GsSortSprite(sprite, OTablePt, 8);
                }
            }
            else
            {
                ControlOptionSlide += 20;
                if (ControlOptionSlide >= 0)
                    return 1;
                ControlOptionPanels[0]->sprite.y = -(ControlOptionSlide + 250);
                ControlOptionPanels[1]->sprite.y = -(ControlOptionSlide + 250);
                y = ControlSchemeLabels[ControlOptionScheme]->clip.vy - 250;
                ControlSchemeLabels[ControlOptionScheme]->sprite.y = y - ControlOptionSlide;
                for (i = 0; i < 8; i++)
                {
                    GsSPRITE *sprite = &ControlActionLabels[ControlLabelActions[ControlOptionScheme][i]]->sprite;
                    sprite->x = ControlLabelPositions[i][language].x;
                    y = ControlLabelPositions[i][language].y - 250;
                    sprite->y = y - ControlOptionSlide;
                    GsSortSprite(sprite, OTablePt, 8);
                }
            }
        }
        GsSortSprite(&ControlOptionPanels[0]->sprite, OTablePt, 9);
        GsSortSprite(&ControlOptionPanels[1]->sprite, OTablePt, 9);
        GsSortSprite(&ControlSchemeLabels[ControlOptionScheme]->sprite, OTablePt, 8);
        for (i = 0; ControlOptionSlide == 0 && i < 8; i++)
        {
            GsSPRITE *sprite = &ControlActionLabels[ControlLabelActions[ControlOptionScheme][i]]->sprite;
            sprite->x = ControlLabelPositions[i][language].x;
            sprite->y = ControlLabelPositions[i][language].y;
            GsSortSprite(sprite, OTablePt, 8);
        }
        if (OptionsPreviousButtons == 0 && ControlOptionSlide == 0)
        {
            if (OptionsButtons & (PADLleft | PADLright))
            {
                if (OptionsButtons == PADLleft)
                    ControlOptionScheme = (ControlOptionScheme + 3) % 4;
                else
                    ControlOptionScheme = (ControlOptionScheme + 1) % 4;
                Sound(0, 3);
            }
            else if (OptionsButtons & (PADRright | PADRdown))
            {
                Sound(0, 4);
                ControlOptionSlide = -250;
                OptionsNextScreen = OPTIONS_MAIN;
                if (OptionsButtons == PADRright)
                    MENU_STATE->control_scheme = ControlOptionScheme;
            }
        }
        break;
    }
    return 0;
}

/* Draft: the screen logic is recovered; label layout and checkmark drawing
 * still have compiler scheduling differences (3,408 versus 3,416 bytes). */
#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/menu.exe/asm/nonmatchings/OPTIONS", UpdateChoiceOptions);
#else
short UpdateChoiceOptions(short mode)
{
    GsIMAGE image;
    u_long *data;
    int i;
    short y;

    switch (mode)
    {
    case -1:
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_CHOICES);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 7; i++)
        {
            if (i == 0)
            {
                ChoiceOptionLabels[0] = SetupSprite(0, &image);
                ChoiceOptionLabels[0]->sprite.h /= 7;
                ChoiceOptionLabels[0]->sprite.my = image.ph / 14;
                ChoiceOptionLabels[0]->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
                ChoiceOptionLabels[0]->sprite.x = 15;
            }
            else
                ChoiceOptionLabels[i] = SetupSprite(ChoiceOptionLabels[0], 0);
            ChoiceOptionLabels[i]->sprite.v += (u_short)(image.ph / 7) * i;
        }
        ChoiceOptionScreen = OptionsNextScreen;
        switch (OptionsNextScreen)
        {
        case OPTIONS_DIFFICULTY:
            ChoiceOptionHeader = OptionsHeaders[0];
            ChoiceOptionHeader->sprite.v = ChoiceOptionHeader->id + ChoiceOptionHeader->sprite.h * 2;
            ChoiceOptionFirst = 0;
            ChoiceOptionCount = 3;
            ChoiceOptionSelection = MENU_STATE->Nannido;
            break;
        case OPTIONS_SOUND_MODE:
            ChoiceOptionHeader = OptionsHeaders[0];
            ChoiceOptionHeader->sprite.v = ChoiceOptionHeader->id + ChoiceOptionHeader->sprite.h * 3;
            ChoiceOptionFirst = 3;
            ChoiceOptionCount = 2;
            ChoiceOptionSelection = !MENU_STATE->Stereo;
            break;
        case OPTIONS_RUMBLE:
            ChoiceOptionHeader = OptionsHeaders[1];
            ChoiceOptionHeader->sprite.v = ChoiceOptionHeader->id + ChoiceOptionHeader->sprite.h;
            ChoiceOptionFirst = 5;
            ChoiceOptionCount = 2;
            ChoiceOptionSelection = !MENU_STATE->Anakon;
            break;
        case OPTIONS_AUTO_SAVE:
            ChoiceOptionHeader = OptionsHeaders[1];
            ChoiceOptionHeader->sprite.v = ChoiceOptionHeader->id + ChoiceOptionHeader->sprite.h * 3;
            ChoiceOptionFirst = 5;
            ChoiceOptionCount = 2;
            ChoiceOptionSelection = !MENU_STATE->fMemory;
            break;
        case OPTIONS_MEMORY_CARD:
            ChoiceOptionHeader = OptionsHeaders[1];
            ChoiceOptionHeader->sprite.v = ChoiceOptionHeader->id + ChoiceOptionHeader->sprite.h * 2;
            data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_LOAD_SAVE);
            GetTIMInfo(data, &image);
            LoadTIM(data);
            for (i = 0; i < 2; i++)
            {
                vfree(ChoiceOptionLabels[i]);
                if (i == 0)
                {
                    ChoiceOptionLabels[0] = SetupSprite(0, &image);
                    ChoiceOptionLabels[0]->sprite.h /= 2;
                    ChoiceOptionLabels[0]->sprite.my = image.ph / 4;
                    ChoiceOptionLabels[0]->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
                }
                else
                    ChoiceOptionLabels[i] = SetupSprite(ChoiceOptionLabels[0], 0);
                ChoiceOptionLabels[i]->sprite.v += (image.ph / 2) * i;
            }
            initialise_font();
            ChoiceOptionFirst = 0;
            ChoiceOptionCount = 2;
            ChoiceOptionSelection = 0;
            break;
        case OPTIONS_LANGUAGE:
            ChoiceOptionHeader = OptionsHeaders[1];
            ChoiceOptionHeader->sprite.v = ChoiceOptionHeader->id + ChoiceOptionHeader->sprite.h * 4;
            data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_LANGUAGES);
            GetTIMInfo(data, &image);
            LoadTIM(data);
            for (i = 0; i < 5; i++)
            {
                vfree(ChoiceOptionLabels[i]);
                if (i == 0)
                {
                    ChoiceOptionLabels[0] = SetupSprite(0, &image);
                    ChoiceOptionLabels[0]->sprite.h /= 5;
                    ChoiceOptionLabels[0]->sprite.my = image.ph / 10;
                    ChoiceOptionLabels[0]->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
                    ChoiceOptionLabels[0]->sprite.x = 15;
                }
                else
                    ChoiceOptionLabels[i] = SetupSprite(ChoiceOptionLabels[0], 0);
                ChoiceOptionLabels[i]->sprite.v += (u_short)(image.ph / 5) * i;
            }
            ChoiceOptionFirst = 0;
            ChoiceOptionCount = N_LANGUAGES;
            ChoiceOptionSelection = ChoicePreviousLanguage = MENU_STATE->language;
            if (ChoiceOptionSelection >= N_LANGUAGES)
                ChoiceOptionSelection = LANG_JAPANESE;
            ChoiceOptionSelection =
                ChoiceOptionSelection == LANG_JAPANESE ? 0 : ChoiceOptionSelection + 1;
            if (!(MENU_STATE->GameRetry & MENU_EXTRA_LANGUAGES))
            {
                ScoreStats *finale[2] = {
                    MENU_STATE->stage_stats[RIKIMARU_0][STAGE_ID_FREE_PRINCESS],
                    MENU_STATE->stage_stats[AYAME_0][STAGE_ID_FREE_PRINCESS]
                };

                for (i = 0; i < N_STAGE_LAYOUTS; i++)
                {
                    if (finale[0][i].stageBosses + finale[0][i].stageEnemies != 0 ||
                        finale[1][i].stageBosses + finale[1][i].stageEnemies != 0)
                        break;
                }
                if (i != N_STAGE_LAYOUTS)
                    MENU_STATE->GameRetry |= MENU_EXTRA_LANGUAGES;
                else
                {
                    if (ChoiceOptionSelection >= 2)
                        ChoiceOptionSelection = 0;
                    /* The fifth row is the locked-language label. */
                    vfree(ChoiceOptionLabels[2]);
                    ChoiceOptionLabels[2] = SetupSprite(ChoiceOptionLabels[4], 0);
                    vfree(ChoiceOptionLabels[3]);
                    ChoiceOptionLabels[3] = SetupSprite(ChoiceOptionLabels[4], 0);
                }
            }
            break;
        }
        for (i = 0; i < ChoiceOptionCount; i++)
        {
            if (ChoiceOptionCount == 2)
                ChoiceOptionLabels[ChoiceOptionFirst + i]->clip.vy = i != 0 ? 20 : -20;
            else if (ChoiceOptionCount == 4)
                ChoiceOptionLabels[ChoiceOptionFirst + i]->clip.vy = i * 20 - 30;
            else
                ChoiceOptionLabels[ChoiceOptionFirst + i]->clip.vy = i * 28 - 28;
        }
        ChoiceCardOperation = 0;
        ChoiceOptionSlide = 250;
        break;
    case 1:
        i = 0;
        while (i < 7)
            vfree(ChoiceOptionLabels[i++]);
        break;
    default:
        if (ChoiceOptionSlide != 0)
        {
            if (ChoiceOptionSlide > 0)
            {
                ChoiceOptionSlide -= 20;
                if (ChoiceOptionSlide <= 0)
                    ChoiceOptionSlide = 0;
                OptionsSmallPanel->sprite.y = -ChoiceOptionSlide;
                for (i = 0; i < ChoiceOptionCount; i++)
                    ChoiceOptionLabels[ChoiceOptionFirst + i]->sprite.y =
                        ChoiceOptionLabels[ChoiceOptionFirst + i]->clip.vy - ChoiceOptionSlide;
            }
            else
            {
                ChoiceOptionSlide += 20;
                if (ChoiceOptionSlide >= 0)
                {
                    if (ChoiceCardOperation != 0)
                        ChoiceOptionSlide = 0;
                    else
                        return 1;
                }
                OptionsSmallPanel->sprite.y = -(ChoiceOptionSlide + 250);
                for (i = 0; i < ChoiceOptionCount; i++)
                {
                    y = ChoiceOptionLabels[ChoiceOptionFirst + i]->clip.vy - 250;
                    ChoiceOptionLabels[ChoiceOptionFirst + i]->sprite.y = y - ChoiceOptionSlide;
                }
            }
            OptionsCheckmark->sprite.y =
                ChoiceOptionLabels[ChoiceOptionFirst + ChoiceOptionSelection]->sprite.y;
        }
        if (ChoiceOptionSlide == 0)
        {
            if (ChoiceCardOperation != 0)
            {
                i = 0;
                if (OptionsPreviousButtons == 0 && OptionsButtons != 0 && ChoiceOptionSlide == 0)
                    i = OptionsButtons;
                if (ChoiceCardOperation == 1)
                    i = UpdateLoadCardScreen(i);
                else
                    i = update_card_screen_(i);
                if (i != 0)
                {
                    ChoiceCardOperation = 0;
                    ChoiceOptionSlide = 250;
                }
                return 0;
            }
            GsSortSprite(&ChoiceOptionHeader->sprite, OTablePt, 9);
            GsSortSprite(&OptionsHelp[ChoiceOptionScreen != OPTIONS_MEMORY_CARD]->sprite, OTablePt, 9);
        }
        GsSortSprite(&OptionsSmallPanel->sprite, OTablePt, 9);
        if (ChoiceOptionScreen != OPTIONS_MEMORY_CARD)
        {
            OptionsCheckmark->sprite.r = OptionsCheckmark->sprite.g =
                OptionsCheckmark->sprite.b = 255;
            OptionsCheckmark->sprite.attribute =
                (OptionsCheckmark->sprite.attribute & ~GS_ATTR_SEMITRANS_MASK) |
                GS_ATTR_SEMITRANS_SUBTRACT;
            GsSortSprite(&OptionsCheckmark->sprite, OTablePt, 8);
            OptionsCheckmark->sprite.r = 143;
            OptionsCheckmark->sprite.attribute =
                (OptionsCheckmark->sprite.attribute & ~GS_ATTR_SEMITRANS_MASK) |
                GS_ATTR_SEMITRANS_ADD;
            OptionsCheckmark->sprite.g = 8;
            OptionsCheckmark->sprite.b = 10;
            GsSortSprite(&OptionsCheckmark->sprite, OTablePt, 7);
        }
        for (i = 0; i < ChoiceOptionCount; i++)
        {
            GsSPRITE *sprite = &ChoiceOptionLabels[ChoiceOptionFirst + i]->sprite;
            if (i == ChoiceOptionSelection)
                sprite->r = sprite->g = sprite->b = 128;
            else
                sprite->r = sprite->g = sprite->b = 32;
            GsSortSprite(sprite, OTablePt, 8);
        }
        if ((short)check_cheat_command_(OptionsButtons,
                OptionsButtons & (OptionsPreviousButtons ^ OptionsButtons)) == MENU_CHEAT_LANGUAGES)
        {
            MENU_STATE->GameRetry |= MENU_EXTRA_LANGUAGES;
            Sound(0, 4);
            ChoiceOptionSlide = -250;
            OptionsNextScreen = OPTIONS_MAIN;
            return 0;
        }
        if (OptionsPreviousButtons == 0 && ChoiceOptionSlide == 0)
        {
            if (OptionsButtons & (PADLup | PADLdown))
            {
                i = ChoiceOptionSelection;
                if (OptionsButtons == PADLup && ChoiceOptionSelection > 0)
                    ChoiceOptionSelection--;
                else if (OptionsButtons == PADLdown && ChoiceOptionSelection < ChoiceOptionCount - 1)
                    ChoiceOptionSelection++;
                if (!(MENU_STATE->GameRetry & MENU_EXTRA_LANGUAGES) &&
                    ChoiceOptionScreen == OPTIONS_LANGUAGE && ChoiceOptionSelection >= 2)
                    ChoiceOptionSelection = 1;
                if (i != ChoiceOptionSelection)
                {
                    Sound(0, 3);
                    OptionsCheckmark->sprite.y =
                        ChoiceOptionLabels[ChoiceOptionFirst + ChoiceOptionSelection]->sprite.y;
                }
            }
            else if (OptionsButtons & (PADRright | PADRdown))
            {
                if (ChoiceOptionScreen != OPTIONS_LANGUAGE || OptionsButtons == PADRdown)
                    Sound(0, 4);
                ChoiceOptionSlide = -250;
                OptionsNextScreen = OPTIONS_MAIN;
                if (OptionsButtons == PADRdown)
                    return 0;
                switch (ChoiceOptionScreen)
                {
                case OPTIONS_DIFFICULTY:
                    MENU_STATE->Nannido = ChoiceOptionSelection;
                    break;
                case OPTIONS_SOUND_MODE:
                    MENU_STATE->Stereo = 1 - ChoiceOptionSelection;
                    if (MENU_STATE->Stereo)
                        SsSetStereo();
                    else
                        SsSetMono();
                    break;
                case OPTIONS_RUMBLE:
                    MENU_STATE->Anakon = 1 - ChoiceOptionSelection;
                    break;
                case OPTIONS_AUTO_SAVE:
                    MENU_STATE->fMemory = 1 - ChoiceOptionSelection;
                    break;
                case OPTIONS_MEMORY_CARD:
                    ChoiceCardOperation = ChoiceOptionSelection + 1;
                    break;
                case OPTIONS_LANGUAGE:
                    MENU_STATE->language =
                        ChoiceOptionSelection != 0 ? ChoiceOptionSelection - 1 : LANG_JAPANESE;
                    if (MENU_STATE->language != ChoicePreviousLanguage)
                        SetupSoundEffect(0, 9);
                    Sound(0, 4);
                    break;
                }
            }
        }
        break;
    }
    return 0;
}
#endif

short UpdateVolumeOptions(short mode)
{
    GsIMAGE image;
    u_long *data;
    short i;
    short y;

    switch (mode)
    {
    case -1:
        OptionsHeaders[0]->sprite.v = OptionsHeaders[0]->id + OptionsHeaders[0]->sprite.h * 4;
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_VOLUME_TRACK);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        VolumeSliders[VOLUME_SLIDER_TRACK] = SetupSprite(0, &image);
        VolumeSliders[VOLUME_SLIDER_TRACK]->sprite.x = 50;
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_VOLUME_KNOB);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        VolumeSliders[VOLUME_SLIDER_KNOB] = SetupSprite(0, &image);
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_LARGE_PANEL);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        VolumePanel = SetupSprite(0, &image);
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_VOLUME_LABELS);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 2; i++)
        {
            if (i == 0)
            {
                VolumeLabels[0] = SetupSprite(0, &image);
                VolumeLabels[0]->sprite.h /= 2;
                VolumeLabels[0]->sprite.my = image.ph / 4;
                VolumeLabels[0]->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
                VolumeLabels[0]->sprite.x = -50;
            }
            else
                VolumeLabels[i] = SetupSprite(VolumeLabels[0], 0);
            VolumeLabels[i]->sprite.v += (image.ph / 2) * i;
            VolumeLabels[i]->clip.vy = i != 0 ? 24 : -24;
        }
        _PlayMusic(OptionsMusicTrack, 1);
        VolumeOptionSelection = 0;
        VolumeSavedLevels[0] = MENU_STATE->SoundLevel;
        VolumeSavedLevels[1] = MENU_STATE->SELevel;
        VolumePositions[0] = VolumeSavedLevels[0] * 80 / 127;
        VolumePositions[1] = VolumeSavedLevels[1] * 80 / 127;
        if (__builtin_abs(VolumePositions[0]) > 80 || __builtin_abs(VolumePositions[1]) > 80)
            VolumePositions[0] = VolumePositions[1] = 80;
        VolumeOptionSlide = 250;
        return 0;
    case 1:
        vfree(VolumeSliders[VOLUME_SLIDER_TRACK]);
        vfree(VolumeSliders[VOLUME_SLIDER_KNOB]);
        vfree(VolumeLabels[0]);
        vfree(VolumeLabels[1]);
        vfree(VolumePanel);
        return 0;
    default:
        if (VolumeOptionSlide != 0)
        {
            if (VolumeOptionSlide > 0)
            {
                VolumeOptionSlide -= 20;
                if (VolumeOptionSlide <= 0)
                    VolumeOptionSlide = 0;
                VolumePanel->sprite.y = -VolumeOptionSlide;
                for (i = 0; i < 2; i++)
                    VolumeLabels[i]->sprite.y = VolumeLabels[i]->clip.vy - VolumeOptionSlide;
            }
            else
            {
                VolumeOptionSlide += 20;
                if (VolumeOptionSlide >= 0)
                    return 1;
                VolumePanel->sprite.y = -(VolumeOptionSlide + 250);
                for (i = 0; i < 2; i++)
                {
                    y = VolumeLabels[i]->clip.vy - 250;
                    VolumeLabels[i]->sprite.y = y - VolumeOptionSlide;
                }
            }
        }
        if (VolumeOptionSlide == 0)
        {
            GsSortSprite(&OptionsHeaders[0]->sprite, OTablePt, 9);
            GsSortSprite(&OptionsHelp[1]->sprite, OTablePt, 9);
        }
        GsSortSprite(&VolumePanel->sprite, OTablePt, 9);
        for (i = 0; i < 2; i++)
        {
            GsSPRITE *sprite = &VolumeLabels[i]->sprite;
            if (i == VolumeOptionSelection)
                sprite->r = sprite->g = sprite->b = 128;
            else
                sprite->r = sprite->g = sprite->b = 32;
            GsSortSprite(sprite, OTablePt, 8);
            VolumeSliders[VOLUME_SLIDER_TRACK]->sprite.y = VolumeSliders[VOLUME_SLIDER_KNOB]->sprite.y = VolumeLabels[i]->sprite.y;
            VolumeSliders[VOLUME_SLIDER_KNOB]->sprite.x = VolumePositions[i] + 10;
            GsSortSprite(&VolumeSliders[VOLUME_SLIDER_TRACK]->sprite, OTablePt, 8);
            GsSortSprite(&VolumeSliders[VOLUME_SLIDER_KNOB]->sprite, OTablePt, 7);
        }
        if (VolumeOptionSlide == 0)
        {
            if (OptionsButtons & (PADLleft | PADLright))
            {
                i = VolumePositions[VolumeOptionSelection];
                if (OptionsButtons == PADLleft && VolumePositions[VolumeOptionSelection] > 0)
                    VolumePositions[VolumeOptionSelection]--;
                else if (OptionsButtons == PADLright && VolumePositions[VolumeOptionSelection] < 80)
                    VolumePositions[VolumeOptionSelection]++;
                if (i != VolumePositions[VolumeOptionSelection] &&
                    (GameClock % 15 == 0 || OptionsPreviousButtons == 0))
                {
                    if (VolumeOptionSelection != 0)
                    {
                        MENU_STATE->SELevel = VolumePositions[1] * 127 / 80;
                        Sound(0, rand() % 11 + 181);
                    }
                    else
                    {
                        MENU_STATE->SoundLevel = VolumePositions[0] * 127 / 80;
                        SsSetSerialVol(0, MENU_STATE->SoundLevel, MENU_STATE->SoundLevel);
                    }
                }
            }
            else if (OptionsPreviousButtons == 0)
            {
                if (OptionsButtons & (PADLup | PADLdown))
                {
                    i = VolumeOptionSelection;
                    if (OptionsButtons == PADLup && VolumeOptionSelection == 1)
                        VolumeOptionSelection--;
                    else if (OptionsButtons == PADLdown && VolumeOptionSelection == 0)
                        VolumeOptionSelection++;
                    if (i != VolumeOptionSelection)
                        Sound(0, 3);
                }
                else if (OptionsButtons & (PADRright | PADRdown))
                {
                    CdaStop();
                    Sound(0, 4);
                    VolumeOptionSlide = -250;
                    OptionsNextScreen = OPTIONS_MAIN;
                    if (OptionsButtons == PADRdown)
                    {
                        MENU_STATE->SoundLevel = VolumeSavedLevels[0];
                        MENU_STATE->SELevel = VolumeSavedLevels[1];
                    }
                    else
                    {
                        MENU_STATE->SoundLevel = VolumePositions[0] * 127 / 80;
                        MENU_STATE->SELevel = VolumePositions[1] * 127 / 80;
                    }
                }
            }
        }
        return 0;
    }
}

short UpdateSoundTest(short mode)
{
    GsIMAGE image;
    u_long *data;
    short i;
    short tone;
    short y;

    switch (mode)
    {
    case -1:
        OptionsHeaders[0]->sprite.v = OptionsHeaders[0]->id + OptionsHeaders[0]->sprite.h * 5;
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_SOUND_TEST_LABELS);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 2; i++)
        {
            if (i == 0)
            {
                SoundTestLabels[0] = SetupSprite(0, &image);
                SoundTestLabels[0]->sprite.h /= 2;
                SoundTestLabels[0]->sprite.my = image.ph / 4;
                SoundTestLabels[0]->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
                SoundTestLabels[0]->sprite.x = -25;
            }
            else
                SoundTestLabels[i] = SetupSprite(SoundTestLabels[0], 0);
            SoundTestLabels[i]->sprite.v += (image.ph / 2) * i;
            SoundTestLabels[i]->clip.vy = i != 0 ? 24 : -24;
        }
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_LARGE_PANEL);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        SoundTestPanel = SetupSprite(0, &image);
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_LARGE_DIGITS);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 10; i++)
        {
            if (i == 0)
            {
                OptionDigitSprites[0] = SetupSprite(0, &image);
                OptionDigitSprites[0]->sprite.w = 14;
                OptionDigitSprites[0]->sprite.mx = 7;
                OptionDigitSprites[0]->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
            }
            else
            {
                OptionDigitSprites[i] = SetupSprite(OptionDigitSprites[0], 0);
                OptionDigitSprites[i]->sprite.u += i * 14;
            }
        }
        tone = 0;
        i = 0;
        do
        {
            while (FindSETone(StageSE, tone) == -1)
                tone++;
            SoundTestEffects[i++] = tone++;
        } while (i < 100);
        SoundTestSelection = SoundTestChoices[0] = SoundTestChoices[1] = 0;
        SoundTestSlide = 250;
        break;
    case 1:
        vfree(SoundTestLabels[0]);
        vfree(SoundTestLabels[1]);
        i = 0;
        while (i < 10)
            vfree(OptionDigitSprites[i++]);
        vfree(SoundTestPanel);
        break;
    default:
        if (SoundTestSlide != 0)
        {
            if (SoundTestSlide > 0)
            {
                SoundTestSlide -= 20;
                if (SoundTestSlide <= 0)
                    SoundTestSlide = 0;
                SoundTestPanel->sprite.y = -SoundTestSlide;
                for (i = 0; i < 2; i++)
                    SoundTestLabels[i]->sprite.y = SoundTestLabels[i]->clip.vy - SoundTestSlide;
            }
            else
            {
                SoundTestSlide += 20;
                if (SoundTestSlide >= 0)
                    return 1;
                SoundTestPanel->sprite.y = -(SoundTestSlide + 250);
                for (i = 0; i < 2; i++)
                {
                    y = SoundTestLabels[i]->clip.vy - 250;
                    SoundTestLabels[i]->sprite.y = y - SoundTestSlide;
                }
            }
        }
        if (SoundTestSlide == 0)
        {
            GsSortSprite(&OptionsHeaders[0]->sprite, OTablePt, 9);
            GsSortSprite(&OptionsHelp[0]->sprite, OTablePt, 9);
        }
        GsSortSprite(&SoundTestPanel->sprite, OTablePt, 9);
        for (i = 0; i < 2; i++)
        {
            GsSPRITE *sprite = &SoundTestLabels[i]->sprite;
            if (i == SoundTestSelection)
                sprite->r = sprite->g = sprite->b = 128;
            else
                sprite->r = sprite->g = sprite->b = 32;
            GsSortSprite(sprite, OTablePt, 8);
        }
        DrawOptionTwoDigits(SoundTestChoices[0], 65, SoundTestLabels[0]->sprite.y);
        DrawOptionTwoDigits(SoundTestChoices[1], 65, SoundTestLabels[1]->sprite.y);
        if (OptionsPreviousButtons == 0 && SoundTestSlide == 0)
        {
            if (OptionsButtons & (PADLleft | PADLright))
            {
                SoundTestChoices[SoundTestSelection] += OptionsButtons == PADLright ? 1 : -1;
                if (SoundTestSelection != 0)
                    SoundTestChoices[1] = (SoundTestChoices[1] + 100) % 100;
                else
                    SoundTestChoices[0] = (SoundTestChoices[0] + 20) % 20;
            }
            else if (OptionsButtons & (PADLup | PADLdown))
            {
                if (OptionsButtons == PADLup && SoundTestSelection == 1)
                {
                    Sound(0, 3);
                    SoundTestSelection = 0;
                }
                else if (OptionsButtons == PADLdown && SoundTestSelection == 0)
                {
                    Sound(0, 3);
                    SoundTestSelection = 1;
                }
            }
            else if (OptionsButtons == PADRdown)
            {
                CdaStop();
                Sound(0, 4);
                SoundTestSlide = -250;
                OptionsNextScreen = OPTIONS_MAIN;
                if (SoundTestChoices[0] != 0)
                    OptionsMusicTrack = SoundTestMusic[SoundTestChoices[0] - 1];
                else
                    OptionsMusicTrack = MUSIC_TRACK_CHARA;
            }
            else if (OptionsButtons == PADRright)
            {
                if (SoundTestSelection != 0)
                    Sound(0, SoundTestEffects[SoundTestChoices[1]]);
                else if (SoundTestChoices[0] != 0)
                    _PlayMusic(SoundTestMusic[SoundTestChoices[0] - 1], 1);
                else
                    CdaStop();
            }
        }
        break;
    }
    return 0;
}


short UpdateRecordOptions(short mode)
{
    GsIMAGE image;
    char text[16];
    u_long *data;
    short i, mission, character, stage;
    short row, j;
    ScoreStats *stats;
    ScoreResult *result;
    GsSPRITE *sprite;

    switch (mode)
    {
    case -1:
        memset(OptionRecords, -1, sizeof(OptionRecords));
        memset(RecordRanks, -1, sizeof(RecordRanks));
        for (character = 0; character < N_PLAYABLE_CHARACTERS; character++)
        {
            for (i = 0; i < RECORD_STAGE_COUNT; i++)
            {
                stage = RecordStageOrder[i];
                for (j = 0; j < N_STAGE_LAYOUTS; j++)
                {
                    stats = &MENU_STATE->stage_stats[character][stage][j];
                    result = calculate_score(stats, stage);
                    if (stats->stageBosses + stats->stageEnemies != 0)
                    {
                        RecordRanks[character][i][j] = RANK_GRAND_MASTER - result->grade;
                        if (OptionRecords[character][i].score < result->score)
                            OptionRecords[character][i].score = result->score;
                        if (OptionRecords[character][i].time == -1 ||
                            stats->clock < OptionRecords[character][i].time)
                            OptionRecords[character][i].time = stats->clock;
                        /* Training has just one recorded layout. */
                        if (i == 0)
                            break;
                    }
                }
                if (OptionRecords[character][i].time < -1)
                    OptionRecords[character][i].time = 0;
                OptionRecords[character][i].time /= 30;
            }
        }

        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_RIKIMARU_NAME);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        RecordNameplates[0] = SetupSprite(0, &image);
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_AYAME_NAME);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        RecordNameplates[1] = SetupSprite(0, &image);
        RecordNameplates[0]->sprite.y = RecordNameplates[1]->sprite.y = -96;
        RecordNameplates[0]->sprite.x = 40;
        RecordNameplates[1]->sprite.x = 120;

        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_RANKS);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < N_STAGE_RANKS; i++)
        {
            if (i == 0)
            {
                RecordRankSprites[0] = SetupSprite(0, &image);
                RecordRankSprites[0]->sprite.h /= N_STAGE_RANKS;
                RecordRankSprites[0]->sprite.my = image.ph / (N_STAGE_RANKS * 2);
            }
            else
            {
                RecordRankSprites[i] = SetupSprite(RecordRankSprites[0], 0);
                RecordRankSprites[i]->sprite.v += (u_short)(image.ph / N_STAGE_RANKS) * i;
            }
        }
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_SMALL_DIGITS);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < 10; i++)
        {
            if (i == 0)
            {
                OptionDigitSprites[0] = SetupSprite(0, &image);
                OptionDigitSprites[0]->sprite.h /= 10;
                OptionDigitSprites[0]->sprite.my = image.ph / 20;
                OptionDigitSprites[0]->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
            }
            else
            {
                OptionDigitSprites[i] = SetupSprite(OptionDigitSprites[0], 0);
                OptionDigitSprites[i]->sprite.v += (u_short)(image.ph / 10) * i;
            }
        }
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_STAGES);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (i = 0; i < RECORD_STAGE_COUNT; i++)
        {
            if (i == 0)
            {
                RecordStageLabels[0] = SetupSprite(0, &image);
                RecordStageLabels[0]->sprite.h /= RECORD_STAGE_COUNT;
                RecordStageLabels[0]->sprite.my = image.ph / (RECORD_STAGE_COUNT * 2);
                RecordStageLabels[0]->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
                RecordStageLabels[0]->sprite.x = -132;
            }
            else
            {
                RecordStageLabels[i] = SetupSprite(RecordStageLabels[0], 0);
                RecordStageLabels[i]->sprite.v += (u_short)(image.ph / RECORD_STAGE_COUNT) * i;
            }
        }
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_RECORD_COLUMNS);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        RecordWidgets[RECORD_COLUMNS] = SetupSprite(0, &image);
        RecordWidgets[RECORD_COLUMNS]->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
        RecordWidgets[RECORD_COLUMNS]->sprite.x = 25;
        RecordWidgets[RECORD_COLUMNS]->sprite.y = -77;
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_RECORD_HIGHLIGHT);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        RecordWidgets[RECORD_RANK_BACKING] = SetupSprite(0, &image);
        RecordWidgets[RECORD_RANK_BACKING]->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
        data = get_tim_from_archive(OptionsImageArchive, OPTION_IMAGE_COLON);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        RecordWidgets[RECORD_COLON] = SetupSprite(0, &image);
        RecordWidgets[RECORD_COLON]->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
        RecordWidgets[RECORD_COLON]->sprite.x = 139;
        OptionsHeaders[0]->sprite.v = OptionsHeaders[0]->id + OptionsHeaders[0]->sprite.h;
        OptionsHeaders[0]->sprite.x = -90;
        RecordWidgets[RECORD_EXIT_HELP] = SetupSprite(OptionsHelp[0], 0);
        RecordWidgets[RECORD_EXIT_HELP]->sprite.w = RecordWidgets[RECORD_EXIT_HELP]->sprite.w / 2 - 2;
        RecordWidgets[RECORD_EXIT_HELP]->sprite.mx /= 2;
        RecordWidgets[RECORD_EXIT_HELP]->sprite.u += RecordWidgets[RECORD_EXIT_HELP]->sprite.w + 4;
        _PlayMusic(12, 1);
        RecordCharacter = RecordExiting = 0;
        break;
    case 1:
        vfree(RecordNameplates[0]);
        vfree(RecordNameplates[1]);
        i = 0;
        while (i < N_STAGE_RANKS)
            vfree(RecordRankSprites[i++]);
        i = 0;
        while (i < 10)
            vfree(OptionDigitSprites[i++]);
        i = 0;
        while (i < RECORD_STAGE_COUNT)
            vfree(RecordStageLabels[i++]);
        i = 0;
        while (i < 4)
            vfree(RecordWidgets[i++]);
        OptionsHeaders[0]->sprite.x = 0;
        break;
    default:
        for (i = 0; i < N_PLAYABLE_CHARACTERS; i++)
        {
            sprite = &RecordNameplates[i]->sprite;
            if (i != RecordCharacter)
                sprite->attribute |= GS_ATTR_SEMITRANS(GPU_BLEND_ADD_QUARTER);
            else
                sprite->attribute &= ~GS_ATTR_SEMITRANS_MASK;
            GsSortSprite(sprite, OTablePt, 9);
        }
        GsSortSprite(&OptionsHeaders[0]->sprite, OTablePt, 9);
        GsSortSprite(&RecordWidgets[RECORD_EXIT_HELP]->sprite, OTablePt, 9);
        row = 0;
        for (mission = 1; mission < RECORD_STAGE_COUNT; mission++)
        {
            for (i = 0; i < N_STAGE_LAYOUTS; i++)
            {
                if (RecordRanks[RecordCharacter][mission][i] != -1)
                {
                    sprite = &RecordRankSprites[RecordRanks[RecordCharacter][mission][i]]->sprite;
                    sprite->x = i * 67 - 77;
                    sprite->y = row * 15 - 65;
                    GsSortSprite(sprite, OTablePt, 2);
                    sprite = &RecordWidgets[RECORD_RANK_BACKING]->sprite;
                    sprite->x = i * 67 - 77;
                    sprite->y = row * 15 - 64;
                    GsSortSprite(sprite, OTablePt, 3);
                }
            }
            if (OptionRecords[RecordCharacter][mission].score != -1)
            {
                sprintf(text, RecordNumberFormat, OptionRecords[RecordCharacter][mission].score);
                DrawOptionNumber(text, 116, row * 15 - 63);
                /* The original display narrows elapsed seconds to a short. */
                i = OptionRecords[RecordCharacter][mission].time;
                j = i / 60;
                sprintf(text, RecordSecondsFormat, (short)(i % 60));
                DrawOptionNumber(text, 151, row * 15 - 63);
                sprintf(text, RecordNumberFormat, j);
                DrawOptionNumber(text, 133, row * 15 - 63);
                sprite = &RecordWidgets[RECORD_COLON]->sprite;
                sprite->y = row * 15 - 63;
                GsSortSprite(sprite, OTablePt, 3);
                sprite = &RecordStageLabels[mission - 1]->sprite;
                sprite->y = row * 15 - 65;
                GsSortSprite(sprite, OTablePt, 3);
                row++;
            }
        }
        if (RecordRanks[RecordCharacter][0][0] != -1)
        {
            sprite = &RecordRankSprites[RecordRanks[RecordCharacter][0][0]]->sprite;
            sprite->x = -77;
            sprite->y = row * 15 - 65;
            GsSortSprite(sprite, OTablePt, 2);
            sprite = &RecordWidgets[RECORD_RANK_BACKING]->sprite;
            sprite->x = -77;
            sprite->y = row * 15 - 64;
            GsSortSprite(sprite, OTablePt, 3);
            i = OptionRecords[RecordCharacter][0].time;
            j = i / 60;
            sprintf(text, RecordSecondsFormat, (short)(i % 60));
            DrawOptionNumber(text, 151, row * 15 - 63);
            sprintf(text, RecordNumberFormat, j);
            DrawOptionNumber(text, 133, row * 15 - 63);
            sprite = &RecordWidgets[RECORD_COLON]->sprite;
            sprite->y = row * 15 - 63;
            GsSortSprite(sprite, OTablePt, 3);
            sprite = &RecordStageLabels[RECORD_STAGE_COUNT - 1]->sprite;
            sprite->y = row * 15 - 65;
            GsSortSprite(sprite, OTablePt, 3);
            row++;
        }
        if (row != 0)
            GsSortSprite(&RecordWidgets[RECORD_COLUMNS]->sprite, OTablePt, 3);
        if (OptionsPreviousButtons == 0 && !RecordExiting)
        {
            if (OptionsButtons & (PADLleft | PADLright))
            {
                i = RecordCharacter;
                RecordCharacter = OptionsButtons != PADLleft;
                if (i != RecordCharacter)
                    Sound(0, RecordCharacter != 0 ? 8 : 7);
            }
            else if (OptionsButtons == PADRdown)
            {
                RecordExiting = 1;
                CdaStop();
                Sound(0, 4);
                OptionsNextScreen = OPTIONS_MAIN;
                return -1;
            }
        }
        break;
    }
    return 0;
}

int UnusedOptionHandler(void)
{
    return 0;
}

void DrawOptionTwoDigits(short value, short x, short y)
{
    short i;
    short digit;
    GsSPRITE *sprite;

    for (i = 0; i < 2; i++)
    {
        if (i == 0)
            digit = value / 10;
        else
            digit = value % 10;
        sprite = &OptionDigitSprites[digit]->sprite;
        sprite->x = x + i * 16;
        sprite->y = y;
        GsSortSprite(sprite, OTablePt, 7);
    }
}

void DrawOptionNumber(char *text, short x, short y)
{
    int i;
    unsigned int digit;
    GsSPRITE *sprite;

    for (i = strlen(text) - 1; i >= 0; i--, x -= 6)
    {
        digit = text[i] - '0';
        if (digit < 10)
        {
            sprite = &OptionDigitSprites[digit]->sprite;
            sprite->x = x;
            sprite->y = y;
            GsSortSprite(sprite, OTablePt, 5);
        }
    }
}
