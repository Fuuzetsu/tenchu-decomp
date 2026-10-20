#include "common.h"
#include "title.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/model.h"
#include "../main.exe/music.h"
#include "../main.exe/tim.h"
#include "../main.exe/tuning.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libpad.h>

extern char path_title_archive[];

short RunTitleMenu(void)
{
    RECT rect;
    short phase = 0;
    short fade = 128;
    int buttons; /* Retail does not initialize the previous-button value. */
    int previous;
    int selection;
    int i;

    TitleImageArchive = FileRead(path_title_archive);
    LoadTitleSprites();
    InitTitleMesh();
    vfree(TitleImageArchive);
    TitleLogoFrame = TitleGenreFrame = TitleSealFrame = TitleSubtitleFrame =
        TitleKunaiFrame = TitleCopyrightFrame = TitleBloodFrame =
        TitleSelection = -1;
    TitleSealAngle = 0;
    TitleSealSpeed = 50;
    _PlayMusic(14, 0);
    while (CdaGetCurrentLength() <= 0)
        ;
    ClearMenuPages();
    SetDispMask(1);
    GameClock = 0;

    while (phase != -3)
    {
        StartDrawing();
        DrawTitleMesh();
        DrawTitleSprites();
        switch (phase)
        {
        case 0:
            fade -= 8;
            if (fade <= 0)
            {
                phase++;
                fade = 0;
            }
            DrawMenuBackground(fade);
            break;
        case 1:
            if (GameClock >= 120)
            {
                TitleSealFrame = 0;
                phase++;
            }
            break;
        case 2:
            if (GameClock >= 250)
            {
                fade -= 8;
                if (fade < -127)
                    phase++;
            }
            DrawMenuBackground(fade);
            break;
        case 3:
            if (GameClock >= 301)
            {
                fade += 2;
                if (fade >= 0)
                {
                    phase++;
                    fade = 0;
                }
            }
            DrawMenuBackground(fade);
            break;
        case 4:
            if (GameClock >= 400)
            {
                TitleLogoFrame = 0;
                phase++;
            }
            break;
        case 5:
            if (GameClock >= 500)
            {
                TitleSubtitleFrame = 0;
                phase++;
            }
            break;
        case 6:
            if (GameClock >= 610)
            {
                TitleKunaiFrame = 0;
                phase++;
            }
            break;
        case 7:
            if (GameClock >= 620)
            {
                TitleBloodFrame = 0;
                phase++;
            }
            break;
        case 8:
            if (GameClock >= 650)
            {
                TitleGenreFrame = 0;
                phase++;
            }
            break;
        case 9:
            if (GameClock >= 700)
            {
                TitleCopyrightFrame = 0;
                phase++;
            }
            break;
        case 10:
            phase = -1;
            break;
        case -1:
            if (GameClock > 3000)
            {
                TitleSelection = -1;
                phase = -2;
            }
            break;
        case -2:
            fade += 8;
            if (fade >= 128)
            {
                phase = -3;
                fade = 128;
            }
            DrawMenuBackground(fade);
            break;
        }
        SkipFrame = 2;
        EndDrawing(0);

        if (phase != -2)
        {
            switch (PadGetState(0))
            {
            case PAD_STATE_FIND_CTP1:
            case PAD_STATE_STABLE:
                previous = buttons;
                buttons = GetRealPad(0);
                if (previous == 0 && buttons != 0)
                    break;
                continue;
            default:
                if (++TitlePadRetryFrames % 10 == 0)
                    PadSetMainMode(0, PAD_MAIN_MODE_ANALOG, PAD_MAIN_MODE_KEEP_LOCK);
                continue;
            }
            GameClock = 0;
            if (phase != -1)
            {
                /* Finish each animation on the next drawing pass. */
                fade = 0;
                TitleLogoFrame = 127;
                TitleGenreFrame = 95;
                TitleSealFrame = 199;
                TitleSubtitleFrame = 29;
                TitleKunaiFrame = 9;
                TitleBloodFrame = 26;
                for (i = 0; i < TITLE_SEAL_COUNT; i++)
                {
                    TitleBloodPalette.colors[TitleBloodColorOrder[i]] =
                        (rand() & 15) | 16;
                }
                setRECT(&rect, TitleBloodClutX, TitleBloodClutY,
                        TITLE_PALETTE_COLORS, 1);
                LoadImage(&rect, (u_long *)&TitleBloodPalette);
                phase = -1;
            }
            if (TitleSelection == -1)
            {
                Sound(0, 2);
                TitleSelection = 0;
                TitleCopyrightFrame = -1;
            }
            else if (buttons == PADLup && TitleSelection > 0)
            {
                Sound(0, 3);
                TitleSelection--;
            }
            else if (buttons == PADLdown && TitleSelection < TITLE_MENU_ITEMS - 1)
            {
                Sound(0, 3);
                TitleSelection++;
            }
            else if (buttons == PADRright || buttons == PADstart)
            {
                Sound(0, 4);
                phase = -2;
            }
        }
    }
    ClearMenuPages();
    SetDispMask(0);
    CdaStop();
    selection = TitleSelection;
    if (selection < -1)
        selection = -1;
    return selection;
}

void LoadTitleSprites(void)
{
    RECT rect;
    GsIMAGE image;
    GsSPRITE swap;
    u_long *data;
    Sprite3D *sprite;
    short i;
    short j;

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_LOGO);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    sprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
    TitleLogoSprite = sprite->sprite;
    vfree(sprite);

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_GENRE);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    TitleGenreSprite = sprite->sprite;
    vfree(sprite);
    TitleGenreSprite.y = -100;

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_SEALS_EVEN);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    image.ph /= 5;
    sprite = SetupSprite(0, &image);
    sprite->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
    for (i = 0; i < 5; i++)
    {
        TitleSealSprites[i * 2] = sprite->sprite;
        TitleSealSprites[i * 2].v += image.ph * i;
    }
    vfree(sprite);

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_SEALS_ODD);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    image.ph /= 4;
    sprite = SetupSprite(0, &image);
    sprite->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
    for (i = 0; i < 4; i++)
    {
        (TitleSealSprites + 1)[i * 2] = sprite->sprite;
        TitleSealSprites[i * 2 + 1].v += image.ph * i;
    }
    vfree(sprite);
    TitleSealPalette = *(TitlePalette *)image.clut;
    setRECT(&rect, TitleSealClutX = image.cx, TitleSealClutY = image.cy,
            TITLE_PALETTE_COLORS, 1);
    TitleSealPalette.colors[14] &= 0x7fff;
    LoadImage(&rect, (u_long *)&TitleSealPalette);

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_SUBTITLE);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    TitleSubtitleSprite = sprite->sprite;
    vfree(sprite);

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_ROMAN_LOGO);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    sprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
    TitleRomanLogoSprite = sprite->sprite;
    vfree(sprite);
    TitleRomanLogoSprite.x = 65;
    TitleRomanLogoSprite.y = 23;

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_KUNAI);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    TitleKunaiSprite = sprite->sprite;
    vfree(sprite);

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_TRADEMARK);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    sprite->sprite.attribute |= GS_ATTR_SEMITRANS_AVERAGE;
    TitleTrademarkSprite = sprite->sprite;
    vfree(sprite);
    TitleTrademarkSprite.x = 125;
    TitleTrademarkSprite.y = 12;

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_SPARK);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    sprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
    TitleSparkSprite = sprite->sprite;
    vfree(sprite);
    TitleSparkSprite.x = -60;
    TitleSparkSprite.y = 23;

    for (i = 0; i < 2; i++)
    {
        data = get_tim_from_archive(TitleImageArchive,
                                   TITLE_IMAGE_COPYRIGHT_LEFT + i);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        sprite = SetupSprite(0, &image);
        sprite->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
        TitleCopyrightSprites[i] = sprite->sprite;
        vfree(sprite);
        TitleCopyrightSprites[i].x = i != 0 ? 76 : -76;
        TitleCopyrightSprites[i].y = 87;
    }

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_IMPACT);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    TitleImpactSprite = sprite->sprite;
    vfree(sprite);
    TitleImpactSprite.x = -89;
    TitleImpactSprite.y = -65;

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_BLOOD);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    sprite = SetupSprite(0, &image);
    TitleBloodSprite = sprite->sprite;
    vfree(sprite);
    TitleBloodSprite.x = -101;
    TitleBloodSprite.y = -84;
    TitleBloodPalette = *(TitlePalette *)image.clut;
    memset(&TitleBloodPalette, 0, sizeof(TitleBloodPalette));
    setRECT(&rect, TitleBloodClutX = image.cx, TitleBloodClutY = image.cy,
            TITLE_PALETTE_COLORS, 1);
    LoadImage(&rect, (u_long *)&TitleBloodPalette);

    for (i = 0; i < 2; i++)
    {
        data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_MENU_TEXT + i);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        image.ph /= TITLE_MENU_ITEMS;
        sprite = SetupSprite(0, &image);
        if (i != 0)
            sprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
        for (j = 0; j < TITLE_MENU_ITEMS; j++)
        {
            *(TitleMenuSprites[j] + i) = sprite->sprite;
            TitleMenuSprites[j][i].v += image.ph * j;
        }
        vfree(sprite);
        /* Move options below the mission editor in the displayed menu. */
        swap = TitleMenuSprites[2][i];
        TitleMenuSprites[2][i] = TitleMenuSprites[3][i];
        TitleMenuSprites[3][i] = swap;
        for (j = 0; j < TITLE_MENU_ITEMS; j++)
            TitleMenuSprites[j][i].y = 47 + j * 16;
    }
}

/* Draft: all 1,892 bytes have the same control flow, but the compiler
 * reassociates the seal rotation's additions (eight differing bytes).
 * A named degree temporary changes allocation without resolving the tie. */
#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/menu.exe/asm/nonmatchings/TITLE", DrawTitleSprites);
#else
void DrawTitleSprites(void)
{
    RECT rect;
    GsSPRITE *sprite;
    short i;
    short remaining;
    short radius;
    short angle;
    int priority;

    if (TitleLogoFrame >= 0)
    {
        sprite = &TitleLogoSprite;
        if (TitleLogoFrame < 128)
        {
            if (++TitleLogoFrame >= 128)
                TitleLogoFrame = 128;
            remaining = 128 - TitleLogoFrame;
            sprite->x = 15;
            sprite->y = -40;
            sprite->scalex = sprite->scaley = FIXED_ONE + remaining * 40;
            sprite->r = sprite->g = sprite->b = TitleLogoFrame;
        }
        else
            GsSortSprite(&TitleTrademarkSprite, OTablePt, 1247);
        GsSortSprite(sprite, OTablePt, 1247);
    }

    if (TitleGenreFrame >= 0)
    {
        if (TitleGenreFrame < 96)
        {
            if (++TitleGenreFrame >= 96)
                TitleGenreFrame = 96;
            TitleGenreSprite.r = TitleGenreSprite.g = TitleGenreSprite.b =
                TitleRomanLogoSprite.r = TitleRomanLogoSprite.g =
                TitleRomanLogoSprite.b = TitleGenreFrame;
        }
        sprite = &TitleGenreSprite;
        sprite->attribute &= ~GS_ATTR_SEMITRANS_MASK;
        sprite->attribute |= GS_ATTR_SEMITRANS_AVERAGE;
        GsSortSprite(sprite, OTablePt, 1246);
        sprite->attribute |= GS_ATTR_SEMITRANS_ADD;
        GsSortSprite(sprite, OTablePt, 1245);
        GsSortSprite(&TitleRomanLogoSprite, OTablePt, 1245);
    }

    if (TitleSealFrame >= 0)
    {
        TitleSealAngle += TitleSealSpeed;
        if (TitleSealFrame < 200)
        {
            if (++TitleSealFrame == 200)
            {
                TitleSealPalette.colors[14] |= 0x8000;
                setRECT(&rect, TitleSealClutX, TitleSealClutY,
                        TITLE_PALETTE_COLORS, 1);
                LoadImage(&rect, (u_long *)&TitleSealPalette);
            }
        }
        else if (TitleSealSpeed > 5 && GameClock % 10 == 0)
            TitleSealSpeed--;
        priority = 0;
        radius = 80 - (200 - TitleSealFrame) * 2;
        if (TitleSealFrame >= 200)
            priority = 1248;
        for (i = 0; i < TITLE_SEAL_COUNT; i++)
        {
            sprite = &TitleSealSprites[i];
            angle = TitleSealAngle + (i << FIXED_SHIFT) / TITLE_SEAL_COUNT;
            sprite->x = (radius * rcos(angle)) >> FIXED_SHIFT;
            sprite->y = ((radius * rsin(angle)) >> FIXED_SHIFT) - 24;
            sprite->rotate =
                (90 + (i * 40 + ((TitleSealAngle * 360) >> FIXED_SHIFT)))
                << FIXED_SHIFT;
            GsSortSprite(sprite, OTablePt, priority);
        }
    }

    if (TitleSubtitleFrame >= 0)
    {
        sprite = &TitleSubtitleSprite;
        if (TitleSubtitleFrame < 30)
        {
            if (++TitleSubtitleFrame >= 30)
                TitleSubtitleFrame = 30;
            remaining = 30 - TitleSubtitleFrame;
            sprite->x = -45 + remaining * 2;
            sprite->y = 23 - remaining * 2;
            sprite->rotate = remaining << 16;
            sprite->scalex = sprite->scaley = (remaining + 16) << 8;
        }
        GsSortSprite(sprite, OTablePt, 1246);
    }

    if (TitleKunaiFrame >= 0)
    {
        sprite = &TitleKunaiSprite;
        if (TitleKunaiFrame < 10)
        {
            if (TitleKunaiFrame == 0)
                Sound(0, 31);
            if (++TitleKunaiFrame >= 10)
                TitleKunaiFrame = 10;
            remaining = 10 - TitleKunaiFrame;
            sprite->x = -28 + remaining * 10;
            sprite->y = -33 - remaining * 15;
        }
        else
            GsSortSprite(&TitleSparkSprite, OTablePt, 1244);
        GsSortSprite(sprite, OTablePt, 1243);
    }

    if (TitleCopyrightFrame >= 0)
    {
        if (TitleCopyrightFrame < 128)
        {
            if (++TitleCopyrightFrame >= 128)
                TitleCopyrightFrame = 128;
            TitleCopyrightSprites[0].r = TitleCopyrightSprites[0].g =
                TitleCopyrightSprites[0].b = TitleCopyrightSprites[1].r =
                TitleCopyrightSprites[1].g = TitleCopyrightSprites[1].b =
                TitleCopyrightFrame;
        }
        GsSortSprite(&TitleCopyrightSprites[0], OTablePt, 1242);
        GsSortSprite(&TitleCopyrightSprites[1], OTablePt, 1242);
    }

    if (TitleBloodFrame >= 0)
    {
        if (TitleBloodFrame == 0)
            Sound(0, 1);
        if (TitleBloodFrame >= 3 && TitleBloodFrame % 3 == 0)
        {
            TitleBloodPalette.colors[TitleBloodColorOrder[TitleBloodFrame / 3 - 1]] =
                (rand() & 15) | 16;
            setRECT(&rect, TitleBloodClutX, TitleBloodClutY,
                    TITLE_PALETTE_COLORS, 1);
            LoadImage(&rect, (u_long *)&TitleBloodPalette);
        }
        if (TitleBloodFrame < 28)
            TitleBloodFrame++;
        GsSortSprite(&TitleBloodSprite, OTablePt, 1241);
        GsSortSprite(&TitleImpactSprite, OTablePt, 1241);
    }

    if (TitleSelection >= 0)
    {
        for (i = 0; i < TITLE_MENU_ITEMS; i++)
        {
            sprite = &TitleMenuSprites[i][0];
            sprite->attribute =
                (sprite->attribute & ~GS_ATTR_SEMITRANS_MASK) |
                (i == TitleSelection ? GS_ATTR_SEMITRANS_ADD :
                 GS_ATTR_SEMITRANS(GPU_BLEND_ADD_QUARTER));
            GsSortSprite(sprite, OTablePt, 3);
            GsSortSprite(&TitleMenuSprites[i][1], OTablePt, 4);
        }
    }
}
#endif
