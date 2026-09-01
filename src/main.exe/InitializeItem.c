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
 *    Indexing TargetSprite directly lets loop.c derive retail's pointer
 *    induction value without an invented source-level `sprite` cursor.
 *  - GetArcData/GetImage feed their consumers directly. Their former `arc`
 *    and `image` carriers were absent from PSX.SYM and remove byte-exactly.
 *    `attr` cannot: spelling the additive sprite attribute at the store moves
 *    its `lui` three instructions later (ten differing bytes), so the carrier
 *    keeps the target's pre-loop materialization order.
 *  - `Item_fInitial = 1;` is ITEM.C's original file-static `fInitial`
 *    (qualified for the split decomp) and DoItemProc's lazy-init guard.
 */

extern GsSPRITE SpriteGoshikimai;

extern ModelType *LoadModel(u_long *adr);
extern GsIMAGE *GetImage(s32 index);
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
