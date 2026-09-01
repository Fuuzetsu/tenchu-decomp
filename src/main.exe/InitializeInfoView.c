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

/*
 * InitializeInfoView (0x8004a790, 0x160 bytes) — one-time HUD/inventory init,
 * called from DoInfoViewProc the first frame (guarded by fInitialize) and
 * from main(). Sets up the cursor/digit sprites, the shared item-image
 * table (ItemImage, the contiguous IMG_ICON_* range, then padded with the
 * otherwise-unused gunfire image),
 * and the retail-expanded 4-entry KehaiImage array, then resets enemy
 * layout/info-view state and marks fInitialize.
 *
 * STATUS: MATCHING — 352 bytes. The SetupSprite result is assigned to its
 * ItemImage slot in the same expression so the following attribute write
 * deliberately goes back through the shared table.
 *
 * Matching notes:
 *  - The three loops index ItemImage[] and KehaiImage[] directly. GCC
 *    strength-reduces those subscripts to the advancing cursors in retail.
 *  - The padding phase keeps its scale and attribute in named values across
 *    the GetImage/SetupSprite calls. The first phase can use the same values
 *    directly because the compiler naturally hoists or rematerializes them
 *    in the corresponding retail locations.
 *  - `fInitialize` is this TU's gp small; maspsxGpExterns is PER FILE (each
 *    split function is its own assembly unit), so this file needs its OWN
 *    Build.hs entry — DoInfoViewProc.c's entry only covers DoInfoViewProc.c.
 *  - `ItemImage`'s auto-computed address DRIFTS depending on which nearby
 *    functions are still raw-asm vs compiled C (this function converting to
 *    C removed the last raw xref that had been anchoring it, and it
 *    resolved +0x28 off) — bound explicitly in config/symbols.main.exe.txt
 *    (plain name: ReqItemDrop/ReqItemMakibishi/
 *    ReqItemManebue already reference this exact symbol as
 *    `extern Sprite3D *ItemImage[];` and need the SAME name fixed under
 *    them, unlike the 0x80097Dxx string-table drift where nothing else
 *    referenced the bad name).
 */
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
