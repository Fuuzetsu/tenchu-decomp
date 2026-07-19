#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitEffect(void);
 *     EFFECT.C:271, 91 src lines, frame 88 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     stack sp+16     struct GsIMAGE img
 *     reg   $s2       short i
 *     stack sp+48     int [3] img
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprBlood;
 *     extern struct GsSPRITE sprSplash;
 *     extern struct GsSPRITE sprFrame[4];
 *     extern struct POLY_F4 plyBleed;
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *sprSmoke;
 *     extern struct Sprite3D *sprBomb[3];
 *     extern struct ModelType *ModelHook;
 * END PSX.SYM */

/*
 * InitEffect (0x80032184, 0x388 bytes) -- initialize the sprites, models,
 * images, and draw primitive used by the game's visual effects.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Copying `D_80097A38.blood` through the named `blood_src` pointer makes
 *    cc1 materialize the enclosing object's full address before the two-word
 *    stack copy.  A direct member assignment folds the loads into the `%hi`
 *    base and does not match the target's `lui; addiu; lwl/lwr` sequence.
 *  - The blood image IDs must stay a flat byte array.  Indexing it with
 *    `i * 2` and `i * 2 + 1` reproduces the target's two independently
 *    formed addresses; caching a row of a two-dimensional array does not.
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

typedef struct
{
    u8 pad[8];
    BloodImageIds blood;
} EffectImageIds;

typedef struct
{
    s32 image[3];
} BombImageIds;

extern EffectImageIds D_80097A38;
extern u8 D_80097A48[5];
extern BombImageIds D_80011C90;
extern s32 pat[4];

extern GsSPRITE sprBlood[4];
extern GsSPRITE sprBlood2[4];
extern GsSPRITE sprSplash;
extern GsSPRITE sprFrame[4];
extern GsSPRITE D_800BEAA8[5];
extern POLY_F4 plyBleed;
extern Sprite3D *sprSmoke[2];
extern Sprite3D *sprBomb[3];
extern Sprite3D *D_80097F2C[1];

extern ModelType *ModelHook;
extern ModelType *D_80097F34;
extern ModelType *LOCAL_COORDINATES_;
extern GsIMAGE *D_80097F3C;
extern s16 D_80097F30;
extern s16 D_80097F32;

extern GsIMAGE *GetImage(s32 index);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern u_long *GetArcData(s32 index);
extern ModelType *LoadModel(u_long *adr);
extern void FUN_80039c14(void);

void InitEffect(void)
{
    BloodImageIds blood_images;
    BloodImageIds *bloodp;
    BloodImageIds *blood_src;
    s32 smoke_images[2];
    s32 smoke_id;
    BombImageIds bomb_images;
    POLY_F4 *poly;
    GsIMAGE *image;
    s16 i;

    blood_src = &D_80097A38.blood;
    blood_images = *blood_src;
    i = 0;
    bloodp = &blood_images;
    for (; i < 4; i++)
    {
        image = GetImage(bloodp->image[i * 2]);
        InitSprite(image, &sprBlood[i]);
        sprBlood[i].attribute = 0x50000000;
        image = GetImage(bloodp->image[i * 2 + 1]);
        InitSprite(image, &sprBlood2[i]);
        sprBlood2[i].attribute = 0x60000000;
    }

    image = GetImage(0xE);
    InitSprite(image, &sprSplash);
    sprSplash.attribute = 0x50000000;
    sprSplash.my = sprSplash.h;

    i = 0;
    while (1)
    {
        if (!(i < 4))
            break;
        image = GetImage(pat[i]);
        InitSprite(image, &sprFrame[i]);
        sprFrame[i].attribute = 0x50000000;
        i++;
    }

    i = 0;
    while (1)
    {
        if (!(i < 5))
            break;
        image = GetImage(D_80097A48[i]);
        InitSprite(image, &D_800BEAA8[i]);
        D_800BEAA8[i].attribute = 0x50000000;
        i++;
    }

    poly = &plyBleed;
    ((u8 *)&poly->tag)[3] = 5;
    poly->code = 0x28;

    {
        Sprite3D *sprite;
        s32 smoke_offset;
        u8 *smoke_address;

        i = 0;
        while (1)
        {
            if (!(i < 2))
                break;
            smoke_id = 6;
            do {
                smoke_images[1] = (smoke_offset = i * 4, 0x3A);
            } while (0);
            smoke_address = (u8 *)smoke_images + smoke_offset;
            smoke_images[0] = smoke_id;
            image = GetImage(*(s32 *)smoke_address);
            sprite = SetupSprite((Sprite3D *)0, image);
            sprSmoke[i] = sprite;
            sprite->sprite.attribute = 0x50000000;
            i++;
        }
    }

    {
        Sprite3D *sprite;

        i = 0;
        while (1)
        {
            if (!(i < 3))
                break;
            bomb_images = D_80011C90;
            image = GetImage(bomb_images.image[i]);
            sprite = SetupSprite((Sprite3D *)0, image);
            sprBomb[i] = sprite;
            sprite->sprite.attribute = 0x50000000;
            i++;
        }
    }

    D_80097F34 = LoadModel(GetArcData(0x19));
    LOCAL_COORDINATES_ = LoadModel(GetArcData(0x1F));
    D_80097F3C = GetImage(0xA);
    ModelHook = LoadModel(GetArcData(0x1A));

    {
        Sprite3D *sprite;

        i = 0;
        do
        {
            image = GetImage(0x37);
            sprite = SetupSprite((Sprite3D *)0, image);
            D_80097F2C[i] = sprite;
            sprite->sprite.attribute = 0x50000000;
            i++;
        } while (i < 1);
    }

    D_80097F30 = 0x340;
    D_80097F32 = 0x100;
    FUN_80039c14();
}

// triage: MEDIUM — 226 insns, 2 loop, 6 callees, ~0.07 to AddItem2
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void InitEffect(void)
//
// {
//   GsIMAGE *pGVar1;
//   Sprite3D *pSVar2;
//   ulong *puVar3;
//   int iVar4;
//   short sVar5;
//   int iVar6;
//   int aiStack_20040 [16382];
//   byte abStack_10048 [65536];
//   undefined4 local_48;
//   undefined4 local_44;
//   int local_40 [4];
//   undefined4 local_30;
//
//   iVar6 = 0;
//   local_48 = DAT_80097a40;
//   local_44 = DAT_80097a44;
//   do {
//     iVar4 = (int)(short)iVar6;
//     pGVar1 = GetImage((uint)*(byte *)((int)&local_48 + iVar4 * 2));
//     InitSprite(pGVar1,&sprBlood + iVar4);
//     (&sprBlood)[iVar4].attribute = 0x50000000;
//     pGVar1 = GetImage((uint)*(byte *)((int)&local_48 + iVar4 * 2 + 1));
//     InitSprite(pGVar1,(GsSPRITE *)(&DAT_800be9e8 + iVar4 * 9));
//     iVar6 = iVar6 + 1;
//     ((GsSPRITE *)(&DAT_800be9e8 + iVar4 * 9))->attribute = 0x60000000;
//   } while (iVar6 * 0x10000 >> 0x10 < 4);
//   pGVar1 = GetImage(0xe);
//   InitSprite(pGVar1,&sprSplash);
//   sVar5 = 0;
//   sprSplash.attribute = 0x50000000;
//   sprSplash.my = sprSplash.h;
//   while( true ) {
//     iVar6 = (int)sVar5;
//     if (3 < iVar6) break;
//     sVar5 = sVar5 + 1;
//     pGVar1 = GetImage(pat[iVar6]);
//     InitSprite(pGVar1,sprFrame + iVar6);
//     sprFrame[iVar6].attribute = 0x50000000;
//   }
//   sVar5 = 0;
//   while( true ) {
//     iVar6 = (int)sVar5;
//     if (4 < iVar6) break;
//     sVar5 = sVar5 + 1;
//     pGVar1 = GetImage((uint)(byte)(&DAT_80097a48)[iVar6]);
//     InitSprite(pGVar1,(GsSPRITE *)(&DAT_800beaa8 + iVar6 * 9));
//     ((GsSPRITE *)(&DAT_800beaa8 + iVar6 * 9))->attribute = 0x50000000;
//   }
//   plyBleed.tag._3_1_ = 5;
//   plyBleed.code = '(';
//   for (sVar5 = 0; iVar6 = (int)sVar5, iVar6 < 2; sVar5 = sVar5 + 1) {
//     local_40[1] = 0x3a;
//     local_40[0] = 6;
//     pGVar1 = GetImage(local_40[iVar6]);
//     pSVar2 = SetupSprite((Sprite3D *)0x0,pGVar1);
//     (&sprSmoke)[iVar6] = pSVar2;
//     (pSVar2->sprite).attribute = 0x50000000;
//   }
//   for (sVar5 = 0; iVar6 = (int)sVar5, iVar6 < 3; sVar5 = sVar5 + 1) {
//     local_40[2] = DAT_80011c90;
//     local_40[3] = DAT_80011c94;
//     local_30 = DAT_80011c98;
//     pGVar1 = GetImage(local_40[iVar6 + 2]);
//     pSVar2 = SetupSprite((Sprite3D *)0x0,pGVar1);
//     sprBomb[iVar6] = pSVar2;
//     (pSVar2->sprite).attribute = 0x50000000;
//   }
//   puVar3 = GetArcData(0x19);
//   DAT_80097f34 = LoadModel(puVar3);
//   puVar3 = GetArcData(0x1f);
//   DAT_80097f38 = LoadModel(puVar3);
//   DAT_80097f3c = GetImage(10);
//   puVar3 = GetArcData(0x1a);
//   DAT_80097f28 = LoadModel(puVar3);
//   iVar6 = 0;
//   do {
//     pGVar1 = GetImage(0x37);
//     pSVar2 = SetupSprite((Sprite3D *)0x0,pGVar1);
//     iVar4 = iVar6 << 0x10;
//     iVar6 = iVar6 + 1;
//     *(Sprite3D **)((int)&DAT_80097f2c + (iVar4 >> 0xe)) = pSVar2;
//     (pSVar2->sprite).attribute = 0x50000000;
//   } while (iVar6 * 0x10000 < 1);
//   DAT_80097f30 = 0x340;
//   DAT_80097f32 = 0x100;
//   FUN_80039c14();
//   return;
// }
