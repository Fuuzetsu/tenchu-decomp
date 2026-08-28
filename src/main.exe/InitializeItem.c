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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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

/*
 * InitializeItem (0x8003d3e4, 0x144 bytes) — one-time setup of the item
 * pool: loads four fixed models (the "launch"/"arrow"-family fixed visuals
 * ReqItemLaunch's SyurikenModel, ReqItemArrow's ArrowModel, NingyoModel, and
 * ProcItemHappou's HappouModel — all four retain PSX.SYM's `ModelType *`
 * declarations and the type LoadModel actually returns), blanks all 30
 * item[] slots, then sets up the on-screen
 * target-lock, item-count, and Goshikimai cursor sprites.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - sprNapalm/sprNapalm2 use the complete shared Sprite3D, including the
 *    embedded GsSPRITE `.sprite` field this function writes.
 *  - The `for (i=0;i<1;i++)` TargetSprite loop is Ghidra's own literal
 *    rendering (a `bgtz`-tested single-iteration loop) — transcribed as-is.
 *  - `Item_fInitial = 1;` is ITEM.C's original file-static `fInitial`
 *    (qualified for the split decomp) and DoItemProc's lazy-init guard.
 */

extern GsSPRITE SpriteGoshikimai;

extern ModelType *LoadModel(u_long *adr);
extern GsIMAGE *GetImage(s32 index);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);

void InitializeItem(void)
{
    u_long *arc;
    GsIMAGE *image;
    GsSPRITE *sprite;
    s32 i;
    u32 attr;

    arc = GetArcData(ARC_SYURIKEN_MODEL);
    SyurikenModel = LoadModel(arc);
    arc = GetArcData(ARC_ARROW_MODEL);
    ArrowModel = LoadModel(arc);
    arc = GetArcData(ARC_NINGYO_MODEL);
    NingyoModel = LoadModel(arc);
    arc = GetArcData(ARC_HAPPOU_MODEL);
    HappouModel = LoadModel(arc);

    for (i = 0; i < MAX_ITEMS; i++)
    {
        items[i].locate = LoadModel((u_long *)0);
        items[i].proc = 0;
    }

    i = 0;
    attr = 0x50000000;
    sprite = TargetSprite;
    while (1)
    {
        if (i >= 1)
            break;
        image = GetImage(IMG_SIGHT);
        InitSprite(image, sprite);
        sprite->attribute = attr;
        sprite++;
        i++;
    }

    image = GetImage(IMG_BOMB0);
    sprNapalm = SetupSprite((Sprite3D *)0, image);
    sprNapalm->sprite.attribute = 0x50000000;
    image = GetImage(IMG_SMOKE);
    sprNapalm2 = SetupSprite((Sprite3D *)0, image);
    sprNapalm2->sprite.attribute = 0x60000000;
    image = GetImage(IMG_GOSHIKIMAI);
    InitSprite(image, &SpriteGoshikimai);

    Item_fInitial = 1;
}
