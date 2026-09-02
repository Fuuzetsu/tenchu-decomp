#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeInfoView(void);
 *     INFOVIEW.C:119, 74 src lines, frame 64 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       int i
 *     stack sp+16     unsigned char [25] image
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE CursorImage;
 *     extern struct Sprite3D *ItemImage[25];
 *     extern unsigned char fInitialize;
 * END PSX.SYM */

extern u8 fInitialize;

extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern void leResetEnemyLayout(void);
extern void ResetInfoview(s32 stage);
extern void init_lifebar_(void);

void InitializeInfoView(void)
{
    GsIMAGE *image;
    Sprite3D *item;
    int i;
    s32 padding_scale;
    s32 padding_attribute;

    image = GetImage(IMG_CURSOR);
    InitSprite(image, &CursorImage);
    CursorImage.attribute = GS_ATTR_SEMITRANS_ADD;
    image = GetImage(IMG_FONT_NUMBER);
    InitSprite(image, &NumberImage);
    for (i = 0; i < N_LOADOUT_ITEMS; i++)
    {
        image = GetImage(i + IMG_ICON_KAGINAWA);
        item = (ItemImage[i] = SetupSprite(0, image));
        item->scale = 0x3000;
        ItemImage[i]->attribute = MODEL_ATTR_CULL_BEHIND |
                                  MODEL_ATTR_CULL_SCREEN |
                                  MODEL_ATTR_CULL_FAR;
    }
    if (i < N_ITEM_SLOTS)
    {
        padding_scale = 0x3000;
        padding_attribute = MODEL_ATTR_CULL_BEHIND | MODEL_ATTR_CULL_SCREEN |
                            MODEL_ATTR_CULL_FAR;
        do
        {
            image = GetImage(IMG_GUNFIRE);
            item = (ItemImage[i] = SetupSprite(0, image));
            item->scale = padding_scale;
            ItemImage[i]->attribute = padding_attribute;
            i++;
        } while (i < N_ITEM_SLOTS);
    }
    for (i = 0; i < N_KEHAI_IMAGES; i++)
    {
        image = GetImage(i + IMG_KEHAI_GREEN);
        InitSprite(image, &KehaiImage[i]);
        KehaiImage[i].attribute = GS_ATTR_SEMITRANS_ADD;
    }
    leResetEnemyLayout();
    ResetInfoview(-1);
    init_lifebar_();
    fInitialize = 1;
}
