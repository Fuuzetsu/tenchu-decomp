#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "effect.h"
#include "afterimage.h"
#include "misc.h"
#include "images.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitEffect(void);
 *     EFFECT.C:271, 91 src lines, frame 88 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct GsIMAGE img
 *     reg   $s2       short i
 *     stack sp+48     int [3] img
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprBlood;
 *     extern struct GsSPRITE sprBloodStay;
 *     extern struct GsSPRITE sprSplash;
 *     extern struct GsSPRITE sprFrame[4];
 *     extern struct Sprite3D *sprImpact[3];
 *     extern struct POLY_F4 plyBleed;
 *     extern struct Sprite3D *sprSmoke;
 *     extern struct Sprite3D *sprBomb[3];
 *     extern struct ModelType *ShadowMdl;
 *     extern struct GsIMAGE *AfterIMG;
 *     extern struct Sprite3D *SpriteSnow;
 *     extern struct ModelType *ModelHook;
 * END PSX.SYM */

/*
 * InitEffect (0x80032184, 0x388 bytes) -- initialize the sprites, models,
 * images, and draw primitive used by the game's visual effects.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - BloodSpriteImageIds is the real eight-byte table immediately after the
 *    effect cursor: four interleaved flying/stain image pairs. Referring to it
 *    through the preceding data anchor preserves the retail instruction
 *    schedule; taking its named address directly coalesces the address-forming
 *    instructions and does not match. The linker map still names the actual
 *    table, so the offset is isolated here rather than leaking into its users.
 *  - The blood image IDs must stay a flat byte array.  Indexing it with
 *    `i * 2` and `i * 2 + 1` reproduces the target's two independently
 *    formed addresses; caching a row of a two-dimensional array does not.
 *  - The `while (1) { if (i >= N) break; ... }` loop spelling is
 *    byte-required: the natural `for` form changes the emitted length
 *    (measured) — the manual top test suppresses the for-loop's
 *    entry-test/rotation treatment.
 *  - Each SetupSprite loop has its own block-scoped `sprite` temporary.
 *    Sharing one function-scoped pointer extends its lifetime and emits
 *    three extra return-value moves.
 *  - The one-shot smoke assignment is an RTL scheduling fence.  Keeping the
 *    scaled-index assignment in the comma expression gives the target's
 *    `li 58; sll; sw` order, while `smoke_address` keeps the subsequent
 *    address add between the two stack stores.  Splitting these into ordinary
 *    statements leaves the same semantics but swaps adjacent instructions.
 */

struct BloodSpriteImagePair
{
    u8 flying;
    u8 stain;
};

union BloodSpriteImageCatalog
{
    struct BloodSpriteImagePair variant[N_BLOOD_SPRITES];
    u8 packed[N_BLOOD_SPRITES * sizeof(struct BloodSpriteImagePair)];
};

extern union BloodSpriteImageCatalog BloodSpriteImageIds;
extern char str_newline[];
#define BLOOD_SPRITE_IMAGE_CATALOG \
    ((union BloodSpriteImageCatalog *)&str_newline[8])
/* Indexed by impact_sprite and the BOMB_SPRITE_* selectors respectively. */
extern u8 ImpactSpriteImageIds[MaxImpacts];
extern ImageArchiveId ExplosionSpriteImageIds[N_EXPLOSION_SPRITES];
extern ImageArchiveId FrameSpriteImageIds[MaxFrames];

extern ModelType *BLOOD_POOL_MODEL_;
extern s16 TexScrollX;
extern s16 TexScrollY;

extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern ModelType *LoadModel(u_long *adr);
extern void reset_effects_(void);

void InitEffect(void)
{
    union BloodSpriteImageCatalog blood_images;
    union BloodSpriteImageCatalog *blood_src;
    union BloodSpriteImageCatalog *bloodp;
    ImageArchiveId smoke_images[N_SMOKE_SPRITES];
    ImageArchiveId smoke_id;
    ImageArchiveId img[N_EXPLOSION_SPRITES];
    POLY_F4 *poly;
    GsIMAGE *image;
    s16 i;

    blood_src = BLOOD_SPRITE_IMAGE_CATALOG;
    blood_images = *blood_src;
    i = 0;
    bloodp = &blood_images;
    for (; i < N_BLOOD_SPRITES; i++)
    {
        /* Not a flattened image[4][2]: the target recomputes the index and
         * re-adds the base for the second element (addu/addu/lbu 0), where a
         * real 2D or paired-struct access folds it into the load as lbu 1.
         * Both spellings measured 24 lines off. */
        image = GetImage(bloodp->packed[i * 2]);
        InitSprite(image, &sprBlood[i]);
        sprBlood[i].attribute = GS_ATTR_SEMITRANS_ADD;
        image = GetImage(bloodp->packed[i * 2 + 1]);
        InitSprite(image, &sprBloodStay[i]);
        sprBloodStay[i].attribute = GS_ATTR_SEMITRANS_SUBTRACT;
    }

    image = GetImage(IMG_SPLASH);
    InitSprite(image, &sprSplash);
    sprSplash.attribute = GS_ATTR_SEMITRANS_ADD;
    sprSplash.my = sprSplash.h;

    i = 0;
    while (1)
    {
        if (i >= MaxFrames)
            break;
        image = GetImage(FrameSpriteImageIds[i]);
        InitSprite(image, &sprFrame[i]);
        sprFrame[i].attribute = GS_ATTR_SEMITRANS_ADD;
        i++;
    }

    i = 0;
    while (1)
    {
        if (i >= MaxImpacts)
            break;
        image = GetImage(ImpactSpriteImageIds[i]);
        InitSprite(image, &sprImpact[i]);
        sprImpact[i].attribute = GS_ATTR_SEMITRANS_ADD;
        i++;
    }

    poly = &plyBleed;
    setPolyF4(poly);

    {
        Sprite3D *sprite;
        s32 smoke_offset;
        u8 *smoke_address;

        i = 0;
        while (1)
        {
            if (i >= N_SMOKE_SPRITES)
                break;
            smoke_id = IMG_SMOKE;
            smoke_images[SMOKE_SPRITE_ALT] =
                (smoke_offset = i * 4, IMG_SMOKE_ALT);
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            smoke_address = (u8 *)smoke_images + smoke_offset;
            smoke_images[SMOKE_SPRITE_NORMAL] = smoke_id;
            image = GetImage(*(ImageArchiveId *)smoke_address);
            sprite = SetupSprite((Sprite3D *)0, image);
            sprSmoke[i] = sprite;
            sprite->sprite.attribute = GS_ATTR_SEMITRANS_ADD;
            i++;
        }
    }

    {
        Sprite3D *sprite;

        i = 0;
        while (1)
        {
            if (i >= N_EXPLOSION_SPRITES)
                break;
            __builtin_memcpy(img, ExplosionSpriteImageIds, sizeof(img));
            image = GetImage(img[i]);
            sprite = SetupSprite((Sprite3D *)0, image);
            sprBomb[i] = sprite;
            sprite->sprite.attribute = GS_ATTR_SEMITRANS_ADD;
            i++;
        }
    }

    ShadowMdl = LoadModel(GetArcData(MODEL_SHADOW));
    BLOOD_POOL_MODEL_ = LoadModel(GetArcData(MODEL_BLOOD_POOL));
    AfterIMG = GetImage(IMG_AFTERIMAGE);
    ModelHook = LoadModel(GetArcData(MODEL_KAGIHEAD));

    {
        Sprite3D *sprite;

        i = 0;
        do
        {
            image = GetImage(IMG_MISC_SNOW);
            sprite = SetupSprite((Sprite3D *)0, image);
            SpriteSnow[i] = sprite;
            sprite->sprite.attribute = GS_ATTR_SEMITRANS_ADD;
            i++;
        } while (i < N_SNOW_SPRITES);
    }

    TexScrollX = 0x340;
    TexScrollY = 0x100;
    reset_effects_();
}
