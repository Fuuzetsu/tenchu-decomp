#include "common.h"
#include "text_entry.h"
#include "../main.exe/images.h"
#include "../main.exe/graphics.h"
#include "../main.exe/tim.h"

void LoadTrialTextGraphics(short mode)
{
    GsIMAGE image;
    RECT area = {640, 256, 240, 16};
    Sprite3D **destinations[] = {
        &TrialTextPanels, &TrialTextCategoryButtons[0],
        &TrialKeyboardCursor->sprite, &TrialEditedTextCursor->sprite,
        &TrialTextShoulderButtons[0], &TrialTextShoulderButtons[1],
        &TrialTextShoulderButtons[2], &TrialTextShoulderButtons[3],
        &TrialTextCategorySprites[0], &TrialTextHeading, &TrialTextScrollArrows[0]
    };
    GsSPRITE *sprite;
    int i;
    u16 code;

    TrialTextBackground = load_background_(get_tim_from_archive(TrialTextArchive, 0));
    for (i = 0; i < 11; i++)
        *destinations[i] = load_archive_sprite_(TrialTextArchive, i + 1);

    sprite = &TrialTextPanels->sprite;
    sprite->x = -128;
    sprite->mx = sprite->my = 0;
    sprite->y = -85;
    for (i = 0; i < 5; i++)
    {
        if (i != 0)
            TrialTextCategoryButtons[i] = SetupSprite(TrialTextCategoryButtons[0], &image);
        sprite = &TrialTextCategoryButtons[i]->sprite;
        sprite->x = -106 + i * 54;
        sprite->y = 80;
    }
    for (i = 0; i < 5; i++)
    {
        if (i != 0)
            TrialTextCategorySprites[i] = SetupSprite(TrialTextCategorySprites[0], &image);
        {
            Sprite3D *object = TrialTextCategorySprites[i];
            sprite = &object->sprite;
            sprite->h = 12;
            sprite->y = 78;
            sprite->r = sprite->g = sprite->b = 128;
            sprite->v += i * 12;
            sprite->x = -106 + i * 54;
            sprite->my = sprite->h >> 1;
            if (i == 0)
                sprite->r = 0;
            object->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
        }
    }
    sprite = &TrialKeyboardCursor->sprite->sprite;
    sprite->x = 108;
    sprite->y = -30;
    sprite = &TrialEditedTextCursor->sprite->sprite;
    sprite->x = -110;
    sprite->y = -62;
    sprite->r = 64;
    sprite->g = sprite->b = 0;
    {
        short positions[4][2] = {{-138, -70}, {-138, 75}, {138, -70}, {138, 75}};
        for (i = 0; i < 4; i++)
        {
            sprite = &TrialTextShoulderButtons[i]->sprite;
            sprite->x = positions[i][0];
            sprite->y = positions[i][1];
        }
    }
    sprite = &TrialTextHeading->sprite;
    sprite->x = -2;
    sprite->y = -100;
    sprite->h = 20;
    sprite->my = 10;
    sprite->v += mode * 20;

    sprite = &TrialTextScrollArrows[0]->sprite;
    sprite->x = -140;
    sprite->y = 7;
    sprite->h >>= 1;
    sprite->my = sprite->h >> 1;
    TrialTextScrollArrows[1] = SetupSprite(TrialTextScrollArrows[0], &image);
    {
        Sprite3D *object = TrialTextScrollArrows[1];
        sprite = &object->sprite;
        sprite->x = 140;
        sprite->y = 7;
        sprite->v += sprite->h;
        object->sprite.attribute |= GS_ATTR_HIDDEN;
    }
    PopulateTrialKeyboard(&TrialKeyboardArea, 13, 5);

    TrialKeyboardQuad = valloc(sizeof(*TrialKeyboardQuad));
    setPolyFT4(TrialKeyboardQuad);
    TrialKeyboardQuad->r0 = TrialKeyboardQuad->g0 = TrialKeyboardQuad->b0 = 128;
    setXY4(TrialKeyboardQuad, -116, -38, 123, -38, -116, 73, 123, 73);
    setUV4(TrialKeyboardQuad, 0, 32, 239, 32, 0, 143, 239, 143);
    TrialKeyboardQuad->tpage = 0x11a;
    TrialEditedTextQuad = valloc(sizeof(*TrialEditedTextQuad));
    setPolyFT4(TrialEditedTextQuad);
    TrialEditedTextQuad->r0 = TrialEditedTextQuad->g0 = TrialEditedTextQuad->b0 = 128;
    setXY4(TrialEditedTextQuad, -118, -78, 121, -78, -118, -63, 121, -63);
    setUV4(TrialEditedTextQuad, 0, 0, 239, 0, 0, 15, 239, 15);
    TrialEditedTextQuad->tpage = 0x11a;
    ClearImage(&area, 0, 0, 0);
    DrawSync(0);
    for (i = 0; i < 15; i++)
    {
        setRECT(&area, 640 + i * 16, 256, 16, 16);
        code = TrialEditedTextCursor->characters[i];
        if (code == 0)
            code = TRIAL_SJIS_SPACE;
        LoadTrialGlyphGrid(code, &area, 1, 1);
    }
}
