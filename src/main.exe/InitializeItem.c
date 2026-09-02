#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeItem(void);
 *     ITEM.C:533, 40 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType *SyurikenModel;
 *     extern struct ModelType *ArrowModel;
 *     extern struct ModelType *NingyoModel;
 *     extern struct tag_TItem items[30];
 *     extern struct ModelType *HappouModel;
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct Sprite3D *sprNapalm;
 *     extern struct Sprite3D *sprNapalm2;
 * END PSX.SYM */

extern GsSPRITE SpriteGoshikimai;

extern ModelType *LoadModel(u_long *adr);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);

void InitializeItem(void)
{
    s32 i;
    u32 attr;

    SyurikenModel = LoadModel(GetArcData(MODEL_SYURIKEN));
    ArrowModel = LoadModel(GetArcData(MODEL_ARROW));
    NingyoModel = LoadModel(GetArcData(MODEL_NINGYO));
    HappouModel = LoadModel(GetArcData(MODEL_HAPPOU));

    for (i = 0; i < MAX_ITEMS; i++)
    {
        items[i].locate = LoadModel((u_long *)0);
        items[i].proc = 0;
    }

    i = 0;
    attr = GS_ATTR_SEMITRANS_ADD;
    while (1)
    {
        if (i >= 1)
            break;
        InitSprite(GetImage(IMG_SIGHT), &TargetSprite[i]);
        TargetSprite[i].attribute = attr;
        i++;
    }

    sprNapalm = SetupSprite((Sprite3D *)0, GetImage(IMG_BOMB0));
    sprNapalm->sprite.attribute = GS_ATTR_SEMITRANS_ADD;
    sprNapalm2 = SetupSprite((Sprite3D *)0, GetImage(IMG_SMOKE));
    sprNapalm2->sprite.attribute = GS_ATTR_SEMITRANS_SUBTRACT;
    InitSprite(GetImage(IMG_GOSHIKIMAI), &SpriteGoshikimai);

    Item_fInitial = 1;
}
