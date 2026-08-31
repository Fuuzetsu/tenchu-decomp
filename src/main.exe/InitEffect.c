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
 *  - Copying the table at `str_newline + 8` through the named `blood_src`
 *    pointer makes cc1 materialize the anchored +8 address before the two-word
 *    stack copy.  A direct member assignment folds the loads into
 *    the `%hi` base and does not match the target's `lui; addiu; lwl/lwr`
 *    sequence.
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

typedef struct
{
    u8 image[8];
} BloodImageIds;

/* "\n"; +4 is an independent effect-pool cursor, and the image table starts at +8. */
extern char str_newline[];
/* Retail extends EFFECT.C's original three-entry static image-ID table. */
extern u8 Effect_img[MaxImpacts];
extern s32 EffectImages[3];
extern s32 pat[MaxFrames];

extern ModelType *BLOOD_POOL_MODEL_;
extern s16 TexScrollX;
extern s16 TexScrollY;

extern GsIMAGE *GetImage(s32 index);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern ModelType *LoadModel(u_long *adr);
extern void reset_effects_(void);

void InitEffect(void)
{
    BloodImageIds blood_images;
    BloodImageIds *bloodp;
    BloodImageIds *blood_src;
    s32 smoke_images[2];
    s32 smoke_id;
    s32 img[3];
    POLY_F4 *poly;
    GsIMAGE *image;
    s16 i;

    blood_src = (BloodImageIds *)&str_newline[8];
    blood_images = *blood_src;
    i = 0;
    bloodp = &blood_images;
    for (; i < 4; i++)
    {
        image = GetImage(bloodp->image[i * 2]);
        InitSprite(image, &sprBlood[i]);
        sprBlood[i].attribute = SPR_TRANS_ADD;
        image = GetImage(bloodp->image[i * 2 + 1]);
        InitSprite(image, &sprBloodStay[i]);
        sprBloodStay[i].attribute = SPR_TRANS_SUB;
    }

    image = GetImage(IMG_SPLASH);
    InitSprite(image, &sprSplash);
    sprSplash.attribute = SPR_TRANS_ADD;
    sprSplash.my = sprSplash.h;

    i = 0;
    while (1)
    {
        if (i >= MaxFrames)
            break;
        image = GetImage(pat[i]);
        InitSprite(image, &sprFrame[i]);
        sprFrame[i].attribute = SPR_TRANS_ADD;
        i++;
    }

    i = 0;
    while (1)
    {
        if (i >= MaxImpacts)
            break;
        image = GetImage(Effect_img[i]);
        InitSprite(image, &sprImpact[i]);
        sprImpact[i].attribute = SPR_TRANS_ADD;
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
            if (i >= 2)
                break;
            smoke_id = IMG_SMOKE;
            smoke_images[1] = (smoke_offset = i * 4, IMG_SMOKE_ALT);
            /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
            do
            {
            } while (0);
            smoke_address = (u8 *)smoke_images + smoke_offset;
            smoke_images[0] = smoke_id;
            image = GetImage(*(s32 *)smoke_address);
            sprite = SetupSprite((Sprite3D *)0, image);
            sprSmoke[i] = sprite;
            sprite->sprite.attribute = SPR_TRANS_ADD;
            i++;
        }
    }

    {
        Sprite3D *sprite;

        i = 0;
        while (1)
        {
            if (i >= 3)
                break;
            __builtin_memcpy(img, EffectImages, sizeof(img));
            image = GetImage(img[i]);
            sprite = SetupSprite((Sprite3D *)0, image);
            sprBomb[i] = sprite;
            sprite->sprite.attribute = SPR_TRANS_ADD;
            i++;
        }
    }

    ShadowMdl = LoadModel(GetArcData(MODEL_SHADOW));
    BLOOD_POOL_MODEL_ = LoadModel(GetArcData(ARC_BLOOD_POOL_MODEL));
    AfterIMG = GetImage(IMG_AFTERIMAGE);
    ModelHook = LoadModel(GetArcData(MODEL_KAGIHEAD));

    {
        Sprite3D *sprite;

        i = 0;
        do
        {
            image = GetImage(IMG_SNOW);
            sprite = SetupSprite((Sprite3D *)0, image);
            SpriteSnow[i] = sprite;
            sprite->sprite.attribute = SPR_TRANS_ADD;
            i++;
        } while (i < 1);
    }

    TexScrollX = 0x340;
    TexScrollY = 0x100;
    reset_effects_();
}
