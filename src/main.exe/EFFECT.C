#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include "afterimage.h"
#include "images.h"
#include "item.h"
#include "misc.h"
#include "model.h"
#include "sound.h"
#include "tmdfast.h"
#include "tuning.h"
#include <psxsdk/libgpu.h>

/*
 * Retail reorganises much of the demo's EFFECT.C and adds fourteen helpers.
 * The translation-unit manifest retains the earlier source-line order.
 */

extern ModelType *BLOOD_POOL_MODEL_;
extern s16 TexScrollX;
extern s16 TexScrollY;
extern RECT ScreenRect; /* {0,0,320,480}: both pages */

extern Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image);
extern long ComputeAreaLevel(AreaNodeType *node, long x, long z);
extern long abs(long value);
extern void DrawTMD(GsDOBJ2 *object, GsOT *ot, s32 mode);
extern int VSync(int mode);
extern void *valloc(u32 size);
extern void vfree(void *p);

void reset_effects_(void);

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

void InitEffect(void)
{
    u8 blood_images[N_BLOOD_SPRITES * 2] = {
        IMG_BLOOD_FLY_0,
        IMG_BLOOD_STAY_0,
        IMG_BLOOD_FLY_1,
        IMG_BLOOD_STAY_1,
        IMG_BLOOD_FLY_2,
        IMG_BLOOD_STAY_2,
        IMG_BLOOD_FLY_3,
        IMG_BLOOD_STAY_3
    };
    POLY_F4 *poly;
    GsIMAGE *image;
    s16 i;

    i = 0;
    for (; i < N_BLOOD_SPRITES; i++)
    {
        /* Retail indexes the two image-id streams independently rather than as pairs. */
        image = GetImage(blood_images[i * 2]);
        InitSprite(image, &sprBlood[i]);
        sprBlood[i].attribute = GS_ATTR_SEMITRANS_ADD;
        image = GetImage(blood_images[i * 2 + 1]);
        InitSprite(image, &sprBloodStay[i]);
        sprBloodStay[i].attribute = GS_ATTR_SEMITRANS_SUBTRACT;
    }

    image = GetImage(IMG_SPLASH);
    InitSprite(image, &sprSplash);
    sprSplash.attribute = GS_ATTR_SEMITRANS_ADD;
    sprSplash.my = sprSplash.h;

    {
        static ImageArchiveId frame_images[MaxFrames] = {
            IMG_FRAME0,
            IMG_FRAME1,
            IMG_FRAME2,
            IMG_FRAME3
        };

        i = 0;
        while (1)
        {
            if (i >= MaxFrames)
                break;
            image = GetImage(frame_images[i]);
            InitSprite(image, &sprFrame[i]);
            sprFrame[i].attribute = GS_ATTR_SEMITRANS_ADD;
            i++;
        }
    }

    {
        static u8 impact_images[MaxImpacts] = {
            IMG_GUNFIRE,
            IMG_GUARD,
            IMG_HIT,
            IMG_SHINSOKU,
            IMG_GOSIN
        };

        i = 0;
        while (1)
        {
            if (i >= MaxImpacts)
                break;
            image = GetImage(impact_images[i]);
            InitSprite(image, &sprImpact[i]);
            sprImpact[i].attribute = GS_ATTR_SEMITRANS_ADD;
            i++;
        }
    }

    poly = &plyBleed;
    setPolyF4(poly);

    {
        Sprite3D *sprite;

        i = 0;
        while (1)
        {
            if (i >= N_SMOKE_SPRITES)
                break;
            {
                ImageArchiveId smoke_images[N_SMOKE_SPRITES] = {
                    IMG_SMOKE,
                    IMG_SMOKE_ALT
                };

                image = GetImage(smoke_images[i]);
                sprite = SetupSprite((Sprite3D *)0, image);
                sprSmoke[i] = sprite;
                sprite->sprite.attribute = GS_ATTR_SEMITRANS_ADD;
            }
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
            {
                ImageArchiveId explosion_images[N_EXPLOSION_SPRITES] = {
                    IMG_BOMB0,
                    IMG_BOMB1,
                    IMG_BOMB2
                };

                image = GetImage(explosion_images[i]);
                sprite = SetupSprite((Sprite3D *)0, image);
                sprBomb[i] = sprite;
                sprite->sprite.attribute = GS_ATTR_SEMITRANS_ADD;
            }
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

    TexScrollX = TEXSCROLL_VRAM_ORIGIN_X;
    TexScrollY = TEXSCROLL_VRAM_ORIGIN_Y;
    reset_effects_();
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTargetS(long x, long y, long z, long color);
 *     EFFECT.C:442, 23 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       long x
 *     param $s1       long y
 *     param $s3       long z
 *     param $a3       long color
 *     stack sp+16     struct GsLINE line
 *     reg   $a2       int z
 * END PSX.SYM */

void DrawTargetS(long x, long y, long z, long color)
{
    GsLINE line;
    long priority;

    z >>= 2;
    priority = z < 0 ? 0 :
        (z >= DEPTH_LIMIT ? DEPTH_LIMIT - 1 : z);

    line.r = (u8)(color >> 16);
    line.attribute = 0;
    line.g = (u8)(color >> 8);
    line.b = (u8)color;
    if (color < 0)
    {
        line.x0 = x - 20;
        line.y0 = y - 20;
        line.x1 = x + 20;
        line.y1 = y + 20;
        GsSortLine(&line, OTablePt, priority);
        line.x0 = x + 20;
        line.y0 = y - 20;
        line.x1 = x - 20;
        line.y1 = y + 20;
        GsSortLine(&line, OTablePt, priority);
    }
    else
    {
        line.x0 = x - 2;
        line.y0 = y - 2;
        line.x1 = x + 2;
        line.y1 = y + 2;
        GsSortLine(&line, OTablePt, priority);
        line.x0 = x + 2;
        line.y0 = y - 2;
        line.x1 = x - 2;
        line.y1 = y + 2;
        GsSortLine(&line, OTablePt, priority);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateTexScroll(struct TexScroll *tscr);
 *     EFFECT.C:1884, 24 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct TexScroll * tscr
 *     reg   $s0       struct DR_MOVE * prim
 * END PSX.SYM */

/* Demo symbols use TexScroll *; retail callback ABI passes its TEffectSlot owner. */
void UpdateTexScroll(TEffectSlot *ef)
{
    TexScroll *tscr;
    DR_MOVE *prim;

    tscr = &ef->param.texscroll;
    tscr->px = (u32)(tscr->px + tscr->vx) %
               (u32)(tscr->image.w << TEXSCROLL_SUBPIXEL_BITS);
    tscr->py = (u32)(tscr->py + tscr->vy) %
               (u32)(tscr->image.h << TEXSCROLL_SUBPIXEL_BITS);

    tscr->image.x = tscr->sx + tscr->px / TEXSCROLL_SUBPIXEL_SCALE;
    tscr->image.y = tscr->sy + tscr->py / TEXSCROLL_SUBPIXEL_SCALE;

    prim = (DR_MOVE *)GsGetWorkBase();
    GsSetWorkBase(prim + 1);
    SetDrawMove(prim, &tscr->image, tscr->x, tscr->y);
    AddPrim(OTablePt->org, prim);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct TexScroll * SetupTexScroll(struct GsIMAGE *img, short x, short y, short mode);
 *     EFFECT.C:1853, 27 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsIMAGE * img
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short mode
 * END PSX.SYM */

void SetupTexScroll(GsIMAGE *img, short vx, short vy)
{
    int idx;
    TEffectSlot *slot;
    int count;
    TexScroll *tscr;
    s16 scrollX;
    short scrollY;
    short j;
    short i;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
{
    u32 scrollYShifted;
    int sx;
    short mask;

    tscr = &slot->param.texscroll;
    slot->param.texscroll.px = tscr->py = 0;

    scrollX = TexScrollX;
    scrollY = TexScrollY;
    tscr->sx = scrollX;
    tscr->sy = scrollY;

    tscr->image.x = tscr->x = img->px;
    tscr->image.y = tscr->y = img->py;
    tscr->image.w = img->pw;
    tscr->image.h = img->ph;
    sx = scrollX;
    scrollYShifted = (u32)(u16)scrollY << 16;

    j = 0;
    while (1)
    {
        for (i = 0; i < TEXSCROLL_GRID_COLUMNS; i++)
        {
            mask = TEXSCROLL_COPY_ALL;
            if ((mask >> (j * TEXSCROLL_GRID_COLUMNS + i)) & 1)
            {
                MoveImage(&tscr->image, sx + img->pw * i,
                          ((s32)scrollYShifted >> 16) + img->ph * j);
            }
        }
        j++;
        if (j >= TEXSCROLL_GRID_ROWS)
        {
            scrollYShifted = 0;
            break;
        }
    }

    TexScrollY += TEXSCROLL_VRAM_SLOT_STRIDE;
    tscr->vx = vx;
    tscr->vy = vy;
    slot->proc = UpdateTexScroll;
    if (TexScrollY > TEXSCROLL_VRAM_Y_LIMIT)
    {
        TexScrollY = TEXSCROLL_VRAM_ORIGIN_Y;
        TexScrollX += TEXSCROLL_VRAM_SLOT_STRIDE;
    }
}
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawBlood(struct tag_EffectSlot *ef);
 *     EFFECT.C:657, 85 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_EffectSlot * ef
 *     reg   $s2       struct BloodType * blood
 *     reg   $s3       struct GsSPRITE * spr
 *     reg   $s2       struct AreaNodeType ** hint
 *     reg   $t3       long x
 *     reg   $t2       long y
 *     reg   $a3       long z
 *     reg   $a2       int sz
 *     reg   $t1       int sy
 *     reg   $a1       int sx
 *     reg   $v1       long rety
 *     reg   $a0       struct AreaNodeType * area
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vec
 *     reg   $s3       struct GsSPRITE * sprt
 *     reg   $s0       long scale
 *     stack sp+24     struct SVECTOR scr
 *     reg   $a0       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprBlood;
 *     extern struct GsSPRITE sprBloodStay;
 *     extern struct GsOT *OTablePt;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern long GameClock;
 * END PSX.SYM */

void DrawBlood(TEffectSlot *ef)
{
    enum
    {
        JITTER_RADIUS = 80
    };
    BloodType *blood;
    GsSPRITE *spr;
    GsSPRITE *sprt;
    SVECTOR scr;
    VECTOR pos;
    VECTOR temp;
    s32 brightness;

    blood = &ef->param.blood;
    spr = &sprBlood[blood->sprite];
    sprt = &sprBloodStay[blood->sprite];

    switch (blood->mode)
    {
    case BLOOD_MODE_FADE:
    {
        s32 scale;
        long rotate;
        s32 otz;
        s32 sort_depth;
        s32 priority;

        blood->brightness -= 5;
        if ((s16)blood->brightness <= 0)
        {
            blood->brightness = 0;
            ef->proc = 0;
        }
        spr->attribute = GS_ATTR_SEMITRANS_ADD;
        scale = blood->scale;
        blood->py += blood->vy;
        rotate = blood->rotate;
        brightness = (s16)blood->brightness;
        GetScreenPosition(blood->px, blood->py, blood->pz, &scr);
        otz = scr.vz;
        if (otz <= NEAR_DEPTH)
        {
            return;
        }
        sprt->scalex = sprt->scaley = spr->scalex = spr->scaley =
            (s16)((scale * PROJECTION_DISTANCE) / otz) + 1;
        spr->rotate = rotate;
        sprt->rotate = rotate;
        sprt->x = spr->x = scr.vx;
        sprt->y = spr->y = scr.vy;
        spr->r = (u8)brightness;
        spr->g = (u8)brightness;
        spr->b = (u8)brightness;
        sprt->r = (u8)(brightness / 2);
        sprt->g = (u8)(brightness / 2);
        sprt->b = (u8)(brightness / 2);

        sort_depth = (s16)(u16)scr.vz >> 2;
        CLAMP_SORT_DEPTH(priority, sort_depth);
        GsSortSprite(spr, OTablePt, (u16)priority);

        sort_depth = (s16)(u16)scr.vz >> 2;
        CLAMP_SORT_DEPTH(priority, sort_depth);
        GsSortSprite(sprt, OTablePt, (u16)priority);
        return;
    }

    case BLOOD_MODE_LINGER:
    {
        u16 oldtime;

        oldtime = blood->time;
        blood->time = oldtime - 1;
        if ((s16)oldtime <= 0)
        {
            blood->time = 0x80;
            blood->mode++;
        }
        break;
    }

    case BLOOD_MODE_SPREAD:
    {
        u16 oldtime;

        blood->scale += rand() % FIXED_ONE;
        oldtime = blood->time;
        blood->time = oldtime - 1;
        if ((s16)oldtime <= 0)
        {
            blood->mode++;
            blood->time = rand() % 90;
        }
        break;
    }

    default: /* BLOOD_MODE_AIRBORNE */
    {
        long x;
        long y;
        long z;
        int sx;
        int sy;
        int sz;
        long rety;
        AreaNodeType *area;
        u16 oldtime;
        s32 scale_random;
        s32 random_x;
        s32 random_y;
        s32 random_z;
        long base_x;
        long base_y;
        long base_z;

        x = blood->px;
        y = blood->py;
        sx = x / 10;
        blood->vy += 10;
        area = blood->hint;
        sy = y / 10;
        z = blood->pz;
        sz = z / 10;
        if (area == 0 || area->y - 200 > sy || sy > area->y ||
            sx < area->x1 || sz < area->z1 || area->x2 < sx || area->z2 < sz)
        {
            rety = GetAreaMapLevel(GlobalAreaMap, x, y - 300, z,
                                   AREA_LEVEL_DEFAULT);
            if (y <= rety && FieldArea->division == AREA_DIVISION_ALL)
            {
                blood->hint = FieldArea;
            }
        }
        else if (area->dy != 0)
        {
            rety = ComputeAreaLevel(area, sx, sz);
            if (rety != LEVEL_NONE)
            {
                rety *= 10;
            }
        }
        else
        {
            rety = area->y * 10;
        }

        if (blood->py >= rety)
        {
            blood->vx = blood->vy = blood->vz = 0;
            if (rety != LEVEL_NONE)
            {
                blood->py = rety;
            }
            else
            {
                blood->vy = rand() % 8 + 8;
                blood->rotate = 0;
                scale_random = rand();
                blood->sprite += 2;
                /* random scale in [1/3, 1/2) of 4.12 one */
                blood->scale = scale_random % 0x2ab + 0x555;
            }
            blood->mode = BLOOD_MODE_SPREAD;
            blood->time = rand() % 10;
            SoundEx((VECTOR *)&blood->px, SE_BLOOD_SPLATTER);
        }
        else
        {
            oldtime = blood->time;
            blood->time = oldtime - 1;
            if ((s16)oldtime <= 0)
            {
                ef->proc = 0;
            }
        }

        if (GameClock & 1)
        {
            memset(&temp, 0, sizeof(VECTOR));
            random_x = rand();
            base_x = blood->px - JITTER_RADIUS;
            temp.vx =
                base_x + random_x % (JITTER_RADIUS * 2);
            random_y = rand();
            base_y = blood->py - JITTER_RADIUS;
            temp.vy =
                base_y + random_y % (JITTER_RADIUS * 2);
            random_z = rand();
            base_z = blood->pz - JITTER_RADIUS;
            temp.vz =
                base_z + random_z % (JITTER_RADIUS * 2);
            pos = temp;
            memset((SVECTOR *)&temp, 0, sizeof(SVECTOR));
            ((SVECTOR *)&temp)->vx = blood->vx / 2;
            ((SVECTOR *)&temp)->vy = blood->vy / 2;
            ((SVECTOR *)&temp)->vz = blood->vz / 2;
            scr = *(SVECTOR *)&temp;
            SetBleed(&pos, &scr, rand() % 10 + 10,
                     RGB24(127, 16, 23));
        }
        break;
    }
    }
{
    s32 scale;
    s32 otz;
    s32 sort_depth;
    s32 priority;

    blood->px += blood->vx;
    blood->py += blood->vy;
    blood->pz += blood->vz;
    spr->rotate = blood->rotate;
    spr->attribute = 0;
    spr->r = blood->brightness;
    spr->g = blood->brightness;
    spr->b = blood->brightness;
    scale = blood->scale;
    GetScreenPosition(blood->px, blood->py, blood->pz, &scr);
    otz = scr.vz;
    if (otz <= NEAR_DEPTH)
    {
        return;
    }
    spr->scalex = spr->scaley =
        (s16)((scale * PROJECTION_DISTANCE) / otz) + 1;
    spr->x = scr.vx;
    spr->y = scr.vy;
    sort_depth = (s16)(u16)scr.vz >> 2;
    CLAMP_SORT_DEPTH(priority, sort_depth);
    GsSortSprite(spr, OTablePt, (u16)priority);
}
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBlood(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:745, 45 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct VECTOR * pos
 *     param $s5       struct SVECTOR * vect
 *     param $s7       short n
 *     param $fp       short time
 *     reg   $s4       short i
 *     reg   $s6       struct AreaNodeType * hint
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct BloodType * blood
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/* Retail dropped the demo build's SVECTOR * argument; every retail caller
 * and the callee use the three-argument form below. */

void SetBlood(VECTOR *pos, short n, short time)
{
    int idx;
    TEffectSlot *slot;
    int count;
    BloodType *blood;
    struct AreaNodeType *hint;
    short i;
    int half;
    int half2;

    GetAreaMapLevel(GlobalAreaMap, pos->vx, pos->vy, pos->vz,
                    AREA_LEVEL_DEFAULT);
    hint = FieldArea;
    i = 0;
    do
    {
        if (i >= n)
        {
            return;
        }
        FIND_EFFECT_SLOT(idx, count, slot, found);
    found:
        blood = &slot->param.blood;
        blood->sprite = rand() % N_AIRBORNE_BLOOD_SPRITES;
        blood->scale = rand() % FIXED_ONE + 2 * FIXED_ONE;
        blood->rotate = (rand() % 360) * FIXED_ONE;
        blood->px = pos->vx;
        blood->py = pos->vy;
        blood->pz = pos->vz;
        blood->vx = rand() % 120 - 60;
        blood->vy = rand() % 60 - 120;
        blood->vz = rand() % 120 - 60;
        half = time / 2;
        half2 = time - half;
        if (half2 > 0)
        {
            blood->time = rand() % half2 + half;
        }
        else
        {
            blood->time = half;
        }
        i++;
        blood->brightness = 0x80;
        blood->hint = hint;
        blood->mode = BLOOD_MODE_AIRBORNE;
        slot->proc = DrawBlood;
    } while (1);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawSmoke(struct tag_EffectSlot *ef);
 *     EFFECT.C:794, 39 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s0       struct SmokeType * smoke
 *     reg   $s1       struct Sprite3D * spr
 *     reg   $s2       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprSmoke;
 * END PSX.SYM */

static void DrawSmoke(TEffectSlot *ef)
{
    SmokeType *param = &ef->param.smoke;
    Sprite3D *spr;
    Sprite3D **sprp;
    u8 alfa;
    u8 oldtime;
    s32 vz_old;
    s32 r;
    s32 m;
    s32 rotate;

    sprp = &sprSmoke[param->sprite];
    spr = *sprp;
    alfa = 0x80;

    if (param->time == param->evtime)
    {
        if (param->vec.vy < -20)
        {
            param->vec.vx = (param->vec.vx * 80) / 100;
            vz_old = param->vec.vz;
            param->vec.vy = param->vec.vy / 2;
            param->vec.vz = (vz_old * 80) / 100;
        }
        param->scale += 0x400;
        r = rand();
        m = param->time - 1;
        param->evtime = m - r % 5;
    }

    if (param->time < 26)
    {
        alfa = param->time * 5;
    }

    param->pos.vx += param->vec.vx;
    param->pos.vy += param->vec.vy;
    param->pos.vz += param->vec.vz;
    spr->locate.coord.t[0] = param->pos.vx;
    spr->locate.coord.t[1] = param->pos.vy;
    spr->locate.coord.t[2] = param->pos.vz;
    spr->scale = param->scale;
    rotate = param->rotate;
    spr->sprite.b = spr->sprite.g = spr->sprite.r = alfa;
    spr->sprite.rotate = rotate;
    UpdateCoordinate((ModelType *)spr);
    DrawSprite(spr);

    oldtime = param->time;
    param->time += 0xff;
    if (oldtime == 0)
    {
        ef->proc = 0;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSmoke(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:835, 21 src lines, frame 56 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct VECTOR * pos
 *     param $s6       struct SVECTOR * vect
 *     param $s7       short n
 *     param $s3       short time
 *     reg   $s4       short i
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct SmokeType * smoke
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetSmoke(VECTOR *pos, SVECTOR *vect, short n, short time)
{
    short i;
    int idx;
    TEffectSlot *slot;
    int count;
    SmokeType *smoke;
    int r;
    int m;

    i = 0;
    do
    {
        if (i >= n)
        {
            return;
        }
        FIND_EFFECT_SLOT(idx, count, slot, found);
    found:
        smoke = &slot->param.smoke;
        r = rand();
        smoke->scale = r % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;
        smoke->rotate = (rand() % 360) * FIXED_ONE;
        smoke->pos.vx = pos->vx;
        smoke->pos.vy = pos->vy;
        smoke->pos.vz = pos->vz;
        smoke->vec.vx = vect->vx + (rand() % 100 - 50);
        smoke->vec.vy = vect->vy + (rand() % 100 - 50);
        smoke->vec.vz = vect->vz + (rand() % 100 - 50);
        smoke->time = time + rand() % 160;
        r = rand();
        i++;
        smoke->sprite = SMOKE_SPRITE_NORMAL;
        m = smoke->time - 1;
        smoke->evtime = m - (time / 2 + r % time);
        slot->proc = DrawSmoke;
    } while (1);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSmokeS(struct VECTOR *pos, short vx, short vy, short vz, int time);
 *     EFFECT.C:858, 14 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       short vx
 *     param $a2       short vy
 *     param $a3       short vz
 *     param stack+16  int time
 * END PSX.SYM */

/* Retail narrows the demo build's int time parameter to u16. */

void SetSmokeS(VECTOR *pos, short vx, short vy, short vz, unsigned short time)
{
    int idx;
    TEffectSlot *slot;
    int count;
    SmokeType *smoke;
    int r;
    int m;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    smoke = &slot->param.smoke;
    smoke->scale = rand() % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;
    smoke->rotate = (rand() % 360) * FIXED_ONE;
    smoke->pos.vx = pos->vx;
    smoke->pos.vy = pos->vy;
    smoke->pos.vz = pos->vz;
    smoke->vec.vx = vx;
    smoke->vec.vy = vy;
    smoke->vec.vz = vz;
    smoke->time = time;
    r = rand();
    smoke->sprite = SMOKE_SPRITE_NORMAL;
    m = smoke->time - 1;
    smoke->evtime = m - ((short)time / 2 + r % (short)time);
    slot->proc = DrawSmoke;
}

void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count)
{
    int i;
    int idx;
    TEffectSlot *slot;
    int searched;
    SmokeType *smoke;
    short vx;
    short vy;
    short vz;
    u32 r;
    int m;

    i = 0;
    do
    {
        if (i >= count)
        {
            return;
        }
        FIND_EFFECT_SLOT(idx, searched, slot, found);
    found:
        {
            int width;

            width = (s16)spread * 2;
            smoke = &slot->param.smoke;
            if (width > 0)
            {
                smoke->vec.vx = rand() % width - spread;
            }
            else
            {
                smoke->vec.vx = -spread;
            }
        }
        smoke->vec.vy = -5;
        {
            int width;

            width = (s16)spread * 2;
            if (width > 0)
            {
                smoke->vec.vz = rand() % width - spread;
            }
            else
            {
                smoke->vec.vz = -spread;
            }
        }

        {
            int div;

            div = (s16)divisor;
            vx = smoke->vec.vx / div;
            vy = smoke->vec.vy / div;
            vz = smoke->vec.vz / div;
        }
        copyVector(&smoke->pos, pos);
        smoke->pos.vx += smoke->vec.vx;
        smoke->pos.vy += smoke->vec.vy;
        smoke->pos.vz += smoke->vec.vz;
        smoke->vec.vx = vx;
        smoke->vec.vy = vy;
        smoke->vec.vz = vz;

        smoke->scale = rand() % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;
        smoke->rotate = 0;
        smoke->time = 15;
        r = rand();
        i++;
        m = smoke->time - 8;
        smoke->sprite = SMOKE_SPRITE_ALT;
        smoke->evtime = m - ((s32)r % 15);
        slot->proc = DrawSmoke;
    } while (1);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawImpact(struct tag_EffectSlot *ef);
 *     EFFECT.C:876, 15 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct tag_EffectSlot * ef
 *     reg   $s1       struct ImpactType * param
 *     reg   $s0       struct Sprite3D * spr
 * END PSX.SYM */

static void DrawImpact(TEffectSlot *ef)
{
    ImpactType *param;
    GsSPRITE *spr;
    SVECTOR scr;
    s32 ratio;
    s32 inverse;
    s32 start;
    s32 start2;
    s32 end;
    s32 end_raw;
    s32 size;
    s32 priority;
    s32 work;

    param = &ef->param.impact;
    ratio = (param->count << FIXED_SHIFT) / param->time;
    spr = &sprImpact[param->type];
    spr->rotate = param->rotate << FIXED_SHIFT;
    inverse = FIXED_ONE - ratio;

    start = param->start_size * inverse;
    param->rotate += param->rotate_speed;
    if (start < 0)
    {
        start += FIXED_TRUNC_BIAS;
    }

    size = (start >> FIXED_SHIFT) +
           (param->end_size * ratio) / FIXED_ONE;

    start = param->start_color.channel.r;
    start = start * inverse;
    end_raw = param->end_color.channel.r;
    if (start < 0)
    {
        start += FIXED_TRUNC_BIAS;
    }
    spr->r = (start >> FIXED_SHIFT) + (end_raw * ratio) / FIXED_ONE;

    work = param->start_color.channel.g;
    start2 = work * inverse;
    end_raw = param->end_color.channel.g;
    if (start2 < 0)
    {
        start2 += FIXED_TRUNC_BIAS;
    }
    start2 = start2 >> FIXED_SHIFT;
    spr->g = start2 + (end_raw * ratio) / FIXED_ONE;

    work = param->start_color.channel.b;
    start2 = work * inverse;
    end_raw = param->end_color.channel.b;
    if (start2 < 0)
    {
        start2 += FIXED_TRUNC_BIAS;
    }
    start2 = start2 >> FIXED_SHIFT;
    spr->b = start2 + (end_raw * ratio) / FIXED_ONE;

    end = param->px;
    /* Empty loop retained for code layout; its original source construct is unknown. */
    do
    {
    } while (0);
    start2 = param->py;
    work = (s32)param->super;
    inverse = param->pz;
    if (work != 0)
    {
        *SCREEN_PROJECTION_POINT_X = end;
        *SCREEN_PROJECTION_POINT_Y = start2;
        *SCREEN_PROJECTION_POINT_Z = inverse;
        GsGetLs((GsCOORDINATE2 *)work, SCREEN_PROJECTION_MATRIX);
        GsSetLsMatrix(SCREEN_PROJECTION_MATRIX);
        scr.vz = (s16)RotTransPers(
            SCREEN_PROJECTION_POINT, (s32 *)&scr,
            SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);
    }
    else
    {
        GetScreenPosition(end, start2, inverse, &scr);
    }

    if (scr.vz > NEAR_DEPTH)
    {
        spr->scalex = spr->scaley =
            (s16)((size * PROJECTION_DISTANCE) / scr.vz) + 1;
        spr->x = scr.vx;
        spr->y = scr.vy;

        start2 = (s16)(u16)scr.vz >> 2;
        CLAMP_SORT_DEPTH(priority, start2);
        GsSortSprite(spr, OTablePt, (u16)priority);
    }

    if (param->count >= param->time)
    {
        ef->proc = 0;
    }
    param->count++;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetImpact(struct VECTOR *pos, short size, short type);
 *     EFFECT.C:893, 13 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       short size
 *     param $a2       short type
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetImpact(VECTOR *pos, short size, short type)
{
    short spd;
    long start_color;
    long end_color;
    int idx;
    TEffectSlot *slot;
    int count;
    ImpactType *param;
    long pz;

    spd = rand() % 90 + 90;
    start_color = COLOR_GRAY;
    end_color = COLOR_GRAY;
    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->proc = DrawImpact;
    slot->param.impact.px = pos->vx;
    param = &slot->param.impact;
    param->py = pos->vy;
    pz = pos->vz;
    param->super = 0;
    param->rotate = 0;
    param->rotate_speed = spd;
    param->start_color.word = start_color;
    param->end_color.word = end_color;
    param->start_size = size;
    param->end_size = 0;
    param->time = 15;
    param->count = 0;
    param->type = type;
    param->pz = pz;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawBleed(struct tag_EffectSlot *ef);
 *     EFFECT.C:910, 34 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct tag_EffectSlot * ef
 *     reg   $s1       struct BleedType * param
 *     stack sp+16     struct SVECTOR scr
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct POLY_F4 plyBleed;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

static void DrawBleed(TEffectSlot *ef)
{
    BleedType *param = &ef->param.bleed;
    SVECTOR scr;
    SVECTOR *projected;
    long x, y, z;
    s32 otz;
    s16 pri;
    s16 sz;

    if (param->mode == 0)
    {
        if (param->time == 0)
        {
            ef->proc = 0;
        }
        else
        {
            param->pos.vx += param->vec.vx;
            param->pos.vy += param->vec.vy;
            param->pos.vz += param->vec.vz;
            param->vec.vy++;
        }
    }
    x = param->pos.vx;
    /* Preserve the scalar view of these VECTOR fields; it affects alias scheduling. */
    y = *(s32 *)&param->pos.vy;
    z = *(s32 *)&param->pos.vz;
    param->time--;

    *SCREEN_PROJECTION_TRANSLATION_X = 0;
    *SCREEN_PROJECTION_TRANSLATION_Y = 0;
    *SCREEN_PROJECTION_TRANSLATION_Z = 0;
    *SCREEN_PROJECTION_POINT_X = x - (short)ViewInfo.vpx;
    *SCREEN_PROJECTION_POINT_Y = y - (short)ViewInfo.vpy;
    *SCREEN_PROJECTION_POINT_Z = z - (short)ViewInfo.vpz;
    SetTransMatrix(SCREEN_PROJECTION_MATRIX);
    SetRotMatrix(&GsWSMATRIX);
    projected = &scr;
    projected->vz = (s16)RotTransPers(
        SCREEN_PROJECTION_POINT, (s32 *)projected,
        SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);

    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        plyBleed.x0 = scr.vx;
        plyBleed.y0 = scr.vy;
        plyBleed.y1 = scr.vy;
        plyBleed.x2 = scr.vx;
        sz = (s16)(900 / otz) + 1;
        plyBleed.x1 = scr.vx + sz;
        plyBleed.y2 = scr.vy + sz;
        plyBleed.x3 = scr.vx + sz;
        plyBleed.y3 = scr.vy + sz;
        plyBleed.r0 = param->r;
        plyBleed.g0 = param->g;
        plyBleed.b0 = param->b;
        pri = otz >> 2;
        if (pri >= 0)
        {
            pri = DEPTH_LIMIT - 1;
            if ((otz >> 2) < DEPTH_LIMIT)
            {
                pri = otz >> 2;
            }
        }
        else
        {
            pri = 0;
        }
        GsSortPoly(&plyBleed, OTablePt, (u16)pri);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleeds(struct VECTOR *pos, short grange, short srange, short n, int time, long col);
 *     EFFECT.C:963, 11 src lines, frame 88 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param stack+0   struct VECTOR * pos
 *     param $a1       short grange
 *     param $s5       short srange
 *     param $s6       short n
 *     param stack+16  int time
 *     param stack+20  long col
 *     reg   $fp       int time
 *     reg   $s7       long col
 *     stack sp+16     struct VECTOR npos
 *     stack sp+32     struct SVECTOR v
 *     reg   $a3       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $s7       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetBleeds(VECTOR *pos, short grange, short srange, short n, int time, long col)
{
    int grange2;
    int g;
    int z2, z3;
    int half;
    int rem;
    int btime;

    g = grange;
    grange2 = g * 2;
    z2 = 0;
    z3 = 0;
    do
    {
        if (n <= 0)
        {
            return;
        }
        {
            VECTOR npos = {
                .vx = pos->vx + (grange2 > 0 ? rand() % grange2 - g : -g),
                .vy = pos->vy + (grange2 > 0 ? rand() % grange2 - g : -g),
                .vz = pos->vz + (grange2 > 0 ? rand() % grange2 - g : -g)
            };
            SVECTOR v = {
                .vx = srange * 2 > 0
                    ? rand() % (srange * 2) - srange
                    : -srange,
                .vy = srange * 2 > 0
                    ? rand() % (srange * 2) - srange
                    : z2 - srange,
                .vz = srange * 2 > 0
                    ? rand() % (srange * 2) - srange
                    : z3 - srange
            };

            half = time / 2;
            rem = time - half;
            if (rem > 0)
            {
                btime = rand() % rem + half;
            }
            else
            {
                btime = half;
            }
            {
                VECTOR *pos = &npos;
                int time = btime;
                int idx;
                TEffectSlot *slot;
                int count;
                BleedType *param;
                u8 r;

                FIND_EFFECT_SLOT(idx, count, slot, found);
            found:
                n--;
                param = &slot->param.bleed;
                r = col >> 16;
                slot->param.bleed.pos = *pos;
                slot->param.bleed.vec = v;
                param->r = r;
                param->g = col >> 8;
                param->time = time;
                param->b = col;
                param->mode = 0;
                slot->proc = DrawBleed;
            }
        }
    } while (1);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawSplash(struct tag_EffectSlot *ef);
 *     EFFECT.C:978, 43 src lines, frame 88 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s1       struct SplashType * param
 *     reg   $s2       struct GsSPRITE * spr
 *     stack sp+24     struct SVECTOR scr
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     stack sp+32     struct VECTOR pos
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

static void DrawSplash(TEffectSlot *ef)
{
    SplashType *param;
    GsSPRITE *spr;
    SVECTOR scr;
    SVECTOR *projected;
    long x;
    long y;
    long z;
    s32 priority;

    param = &ef->param.splash;
    spr = &sprSplash;
    x = param->px;
    y = *(s32 *)&param->py;
    z = *(s32 *)&param->pz;

    *SCREEN_PROJECTION_TRANSLATION_X = 0;
    *SCREEN_PROJECTION_TRANSLATION_Y = 0;
    *SCREEN_PROJECTION_TRANSLATION_Z = 0;
    *SCREEN_PROJECTION_POINT_X = x - (s16)ViewInfo.vpx;
    *SCREEN_PROJECTION_POINT_Y = y - (s16)ViewInfo.vpy;
    *SCREEN_PROJECTION_POINT_Z = z - (s16)ViewInfo.vpz;
    SetTransMatrix(SCREEN_PROJECTION_MATRIX);
    SetRotMatrix(&GsWSMATRIX);
    projected = &scr;
    projected->vz = (s16)RotTransPers(
        SCREEN_PROJECTION_POINT, (s32 *)projected,
        SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);
    {
        s32 z;

        z = scr.vz;
        if (z > NEAR_DEPTH)
        {
            spr->x = scr.vx;
            spr->y = scr.vy;
            spr->scalex = (param->sx * PROJECTION_DISTANCE) / z + 1;
            spr->scaley = (param->sy * PROJECTION_DISTANCE) / z + 1;

            switch (param->mode)
            {
            case SPLASH_MODE_SPAWN:
                param->count = 0;
                param->mode++;
                {
                    VECTOR pos = {param->px, param->py, param->pz};
                    SVECTOR direction = {
                        .vx = 0,
                        .vy = -20,
                        .vz = 0
                    };

                    SetBleedsDir(&pos, &direction, 100, 6, 30, RGB24(144, 152, 160));
                }
                /* fall through */
            case SPLASH_MODE_RISE:
                spr->scaley = (spr->scaley * param->count) / param->speed;
                param->count++;
                if (param->count >= param->speed)
                {
                    param->count = 0;
                    param->mode++;
                }
                break;
            case SPLASH_MODE_FALL:
                spr->scaley = (spr->scaley * (param->speed - param->count)) /
                              param->speed;
                spr->scalex = spr->scalex / 2;
                param->count++;
                if (param->count >= param->speed)
                {
                    ef->proc = 0;
                }
                break;
            }

            {
                s32 t;

                t = (s16)(u16)scr.vz >> 2;
                if (t >= 0)
                {
                    priority = DEPTH_LIMIT - 1;
                    if (t < DEPTH_LIMIT)
                    {
                        priority = t;
                    }
                }
                else
                {
                    priority = 0;
                }
            }
            GsSortSprite(spr, OTablePt, (u16)priority);
        }
    }
}

void DrawSnow(TEffectSlot *ef)
{
    SnowParticleType *param;
    Sprite3D *model;
    GsSPRITE *sprite;
    SVECTOR screen;
    s32 view_z;
    s32 view_x;
    s32 view_y;
    s32 x;
    s32 y;
    s32 z;
    s32 ground;
    /* delta widths are u32: the unsigned % SNOW_SPAN is in the bytes. */
    u32 delta_y;
    u32 delta;
    u32 offset;
    s32 wrapped;
    s32 size;
    s16 depth;
    s16 otz;
    s32 priority;

    param = &ef->param.snow;
    view_x = ViewInfo.vrx;
    view_y = ViewInfo.vry;
    view_z = ViewInfo.vrz;
    {
        s16 velocity_x;
        s16 velocity_z;
        s16 velocity_y;

        x = param->x;
        y = param->y;
        velocity_x = param->velocity[0];
        z = param->z;
        velocity_y = param->velocity[1];
        x += velocity_x;
        y += velocity_y;
        velocity_z = param->velocity[2];
        ground = param->ground;
        z += velocity_z;
    }
    wrapped = 0;

    if (ground < y)
    {
        ef->proc = 0;
        return;
    }

    delta_y = y - view_y;
    delta = x - view_x;
    if ((s32)delta_y > SNOW_RANGE)
    {
        wrapped = 1;
        offset = delta_y % SNOW_SPAN - SNOW_RANGE;
        y = view_y + offset;
    }
    if (SNOW_RANGE < abs(delta))
    {
        wrapped = 1;
        offset = delta % SNOW_SPAN - SNOW_RANGE;
        x = view_x + offset;
    }
    delta = z - view_z;
    if (SNOW_RANGE < abs(delta))
    {
        wrapped = 1;
        offset = delta % SNOW_SPAN - SNOW_RANGE;
        z = view_z + offset;
    }

    if (wrapped != 0)
    {
        wrapped = GetAreaMapLevel(GlobalAreaMap, x, param->sample_y, z,
                                  AREA_LEVEL_FIRST_HIT);
        if (wrapped < y)
        {
            ef->proc = 0;
            return;
        }
        param->ground = wrapped;
    }

    param->x = x;
    param->y = y;
    param->z = z;
    model = SpriteSnow[param->sprite];
    sprite = &model->sprite;
    size = param->size;
    GetScreenPosition(x, y, z, &screen);
    depth = screen.vz;
    if (depth > NEAR_DEPTH)
    {
        sprite->scalex = sprite->scaley =
            (s16)((size * PROJECTION_DISTANCE) / depth) + 1;
        sprite->x = screen.vx;
        sprite->y = screen.vy;
        otz = (s16)(u16)screen.vz >> 2;
        CLAMP_SORT_DEPTH(priority, otz);
        GsSortSprite(sprite, OTablePt, (u16)priority);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawFrame(struct tag_EffectSlot *ef);
 *     EFFECT.C:1044, 49 src lines, frame 104 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s1       struct FrameType * param
 *     reg   $s2       struct GsSPRITE * spr
 *     stack sp+16     struct SVECTOR scr
 *     stack sp+24     struct SVECTOR sv
 *     stack sp+40     struct MATRIX mat
 *     stack sp+72     long p
 *     stack sp+76     long flag
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprFrame[4];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

static void DrawFrame(TEffectSlot *ef)
{
    enum
    {
        FRAME_FLASH_LEVEL = 0x80
    };
    FrameType *param = &ef->param.frame;
    GsSPRITE *spr;
    SVECTOR scr;
    s16 idx;
    s32 otz;
    s32 t;
    s32 pri;
    s32 px, py, pz;
    GsCOORDINATE2 *hint;
    s32 size;

    idx = param->count % MaxFrames;
    spr = &sprFrame[idx];

    switch (param->mode)
    {
    case FRAME_MODE_FLASH:
        spr->b = FRAME_FLASH_LEVEL;
        spr->g = FRAME_FLASH_LEVEL;
        spr->r = FRAME_FLASH_LEVEL;
        param->count--;
        if (param->count <= 0)
        {
            param->count = FRAME_FLASH_LEVEL;
            param->mode++;
        }
        break;
    case FRAME_MODE_FADE:
        spr->r = spr->g = spr->b = (u8)param->count;
        param->count -= 29;
        if (param->count <= 0)
        {
            ef->proc = 0;
        }
        break;
    }
    px = param->px;
    py = param->py;
    pz = param->pz;
    hint = param->super;
    size = param->size;

    if (hint != 0)
    {
        *SCREEN_PROJECTION_POINT_X = px;
        *SCREEN_PROJECTION_POINT_Y = py;
        *SCREEN_PROJECTION_POINT_Z = pz;
        GsGetLs(hint, SCREEN_PROJECTION_MATRIX);
        GsSetLsMatrix(SCREEN_PROJECTION_MATRIX);
        scr.vz = (s16)RotTransPers(
            SCREEN_PROJECTION_POINT, (s32 *)&scr,
            SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);
    }
    else
    {
        GetScreenPosition(px, py, pz, &scr);
    }

    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        spr->scalex = spr->scaley =
            (s16)((size * PROJECTION_DISTANCE) / otz) + 1;
        spr->x = scr.vx;
        spr->y = scr.vy;
        t = scr.vz - 0x32;
        t = t >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(spr, OTablePt, (u16)pri);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleedsDir(struct VECTOR *pos, struct SVECTOR *vec, short grange, short n, int time, long col);
 *     EFFECT.C:1115, 12 src lines, frame 88 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s7       struct VECTOR * pos
 *     param $fp       struct SVECTOR * vec
 *     param $a2       short grange
 *     param $s5       short n
 *     param stack+16  int time
 *     param stack+20  long col
 *     reg   $s4       int time
 *     reg   $s6       long col
 *     stack sp+16     struct VECTOR npos
 *     stack sp+32     struct SVECTOR v
 *     reg   $a3       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $s6       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n, int time, long col)
{
    int btime;

    do
    {
        if (n <= 0)
        {
            return;
        }
        {
            VECTOR npos = {
                .vx = pos->vx + (grange * 2 > 0
                                     ? rand() % (grange * 2) - grange
                                     : -grange),
                .vy = pos->vy + (grange * 2 > 0
                                     ? rand() % (grange * 2) - grange
                                     : -grange),
                .vz = pos->vz + (grange * 2 > 0
                                     ? rand() % (grange * 2) - grange
                                     : -grange)
            };
            SVECTOR v = {
                .vx = vec->vx,
                .vy = vec->vy,
                .vz = vec->vz
            };

            if (time - time / 8 > 0)
            {
                btime = rand() % (time - time / 8) + time / 8;
            }
            else
            {
                btime = time / 8;
            }
            {
                VECTOR *pos = &npos;
                int time = btime;
                int idx;
                TEffectSlot *slot;
                int count;
                BleedType *param;
                u8 r;

                FIND_EFFECT_SLOT(idx, count, slot, found);
            found:
                n--;
                param = &slot->param.bleed;
                r = col >> 16;
                slot->param.bleed.pos = *pos;
                slot->param.bleed.vec = v;
                param->r = r;
                param->g = col >> 8;
                param->time = time;
                param->b = col;
                param->mode = 0;
                slot->proc = DrawBleed;
            }
        }
    } while (1);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawGore(struct tag_EffectSlot *ef);
 *     EFFECT.C:1131, 28 src lines, frame 56 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_EffectSlot * ef
 *     reg   $s1       struct GoreType * param
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     reg   $t1       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $a3       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a2       int i
 * END PSX.SYM */

static void DrawGore(TEffectSlot *ef)
{
    enum
    {
        JITTER_RADIUS = 60,
        GORE_COLOR = RGB24(127, 16, 23)
    };
    BloodType *param;
    GsSPRITE *spr;
    GsSPRITE *spr2;
    SVECTOR vector;
    VECTOR position;
    VECTOR temporary;

    param = &ef->param.blood;
    spr = &sprBlood[param->sprite];
    spr2 = &sprBloodStay[param->sprite];
    switch (param->mode)
    {
    case GORE_MODE_FADE:
    {
        s32 brightness;
        s32 size;
        s32 rotate;
        s32 sort_depth;
        s32 priority;

        param->brightness -= 5;
        if ((s16)param->brightness <= 0)
        {
            param->brightness = 0;
            ef->proc = 0;
        }

        spr->attribute = GS_ATTR_SEMITRANS_ADD;
        param->py += param->vy;
        size = param->scale;
        rotate = param->rotate;
        brightness = (s16)param->brightness;
        GetScreenPosition(param->px, param->py, param->pz, &vector);
        if (vector.vz <= NEAR_DEPTH)
        {
            return;
        }
        spr2->scalex = spr2->scaley = spr->scalex = spr->scaley =
            (s16)((size * PROJECTION_DISTANCE) / vector.vz) + 1;
        spr->rotate = rotate;
        spr2->rotate = rotate;
        spr2->x = spr->x = vector.vx;
        spr2->y = spr->y = vector.vy;
        spr->r = (u8)brightness;
        spr->g = (u8)brightness;
        spr->b = (u8)brightness;
        spr2->r = (u8)(brightness / 2);
        spr2->g = (u8)(brightness / 2);
        spr2->b = (u8)(brightness / 2);

        sort_depth = (s16)(u16)vector.vz >> 2;
        CLAMP_SORT_DEPTH(priority, sort_depth);
        GsSortSprite(spr, OTablePt, (u16)priority);

        sort_depth = (s16)(u16)vector.vz >> 2;
        CLAMP_SORT_DEPTH(priority, sort_depth);
        GsSortSprite(spr2, OTablePt, (u16)priority);
        return;
    }

    case GORE_MODE_LINGER:
    {
        u16 count;

        count = param->time;
        param->time = count - 1;
        if ((s16)count <= 0)
        {
            param->time = 0x80;
            param->mode++;
        }
        break;
    }

    case GORE_MODE_SPREAD:
    {
        u16 count;

        param->scale += rand() % FIXED_ONE;
        count = param->time;
        param->time = count - 1;
        if ((s16)count <= 0)
        {
            param->mode++;
            param->time = rand() % 90;
        }
        break;
    }

    default: /* GORE_MODE_AIRBORNE */
    {
        s32 x;
        s32 y;
        s32 z;
        s32 x10;
        s32 y10;
        s32 z10;
        s32 level;
        AreaNodeType *node;
        int scale_random;
        int random_x;
        int random_y;
        int random_z;
        s32 base_x;
        s32 base_y;
        s32 base_z;
        SVECTOR *velocity;
        long color;
        long green;
        int cursor;
        int i;
        TEffectSlot *slot;
        BleedType *bleed;

        x = param->px;
        y = param->py;
        z = param->pz;
        x10 = x / 10;
        y10 = y / 10;
        z10 = z / 10;
        param->vy += 10;
        node = param->hint;
        if (node == 0 || y10 < node->y - 200 || node->y < y10 ||
            x10 < node->x1 || z10 < node->z1 || node->x2 < x10 ||
            node->z2 < z10)
        {
            level = GetAreaMapLevel(GlobalAreaMap, x, y - 300, z,
                                    AREA_LEVEL_DEFAULT);
            if (y <= level && FieldArea->division == AREA_DIVISION_ALL)
            {
                param->hint = FieldArea;
            }
        }
        else if (node->dy != 0)
        {
            level = ComputeAreaLevel(node, x10, z10);
            if (level != LEVEL_NONE)
            {
                level *= 10;
            }
        }
        else
        {
            level = node->y * 10;
        }
        if (param->py >= level)
        {
            param->vx = param->vy = param->vz = 0;
            if (level != LEVEL_NONE)
            {
                param->py = level;
            }
            else
            {
                param->vy = rand() % 8 + 8;
                param->rotate = 0;
                scale_random = rand();
                param->sprite += N_AIRBORNE_BLOOD_SPRITES;
                /* random scale in [1/3, 1/2) of 4.12 one */
                param->scale = scale_random % 0x2ab + 0x555;
            }
            param->mode = GORE_MODE_SPREAD;
            param->time = rand() % 10;
            SoundEx((VECTOR *)&param->px, SE_BLOOD_SPLATTER);
        }
        else
        {
            u16 count;

            count = param->time;
            param->time = count - 1;
            if ((s16)count <= 0)
            {
                ef->proc = 0;
            }
        }

        memset(&temporary, 0, sizeof(VECTOR));
        random_x = rand();
        base_x = param->px - JITTER_RADIUS;
        temporary.vx =
            base_x + random_x % (JITTER_RADIUS * 2);
        random_y = rand();
        base_y = param->py - JITTER_RADIUS;
        temporary.vy =
            base_y + random_y % (JITTER_RADIUS * 2);
        random_z = rand();
        base_z = param->pz - JITTER_RADIUS;
        temporary.vz =
            base_z + random_z % (JITTER_RADIUS * 2);
        position = temporary;
        memset((SVECTOR *)&temporary, 0, sizeof(SVECTOR));
        velocity = &vector;
        color = GORE_COLOR;
        ((SVECTOR *)&temporary)->vx = param->vx / 2;
        ((SVECTOR *)&temporary)->vy = param->vy / 2;
        ((SVECTOR *)&temporary)->vz = param->vz / 2;
        *velocity = *(SVECTOR *)&temporary;

        FIND_EFFECT_SLOT(cursor, i, slot, bleed_found);
    bleed_found:
        bleed = &slot->param.bleed;
        slot->param.bleed.pos = position;
        slot->param.bleed.vec = *velocity;
        bleed->time = 7;
        bleed->r = GORE_COLOR >> 16;
        green = GORE_COLOR >> 8;
        bleed->g = green;
        bleed->b = color;
        bleed->mode = 0;
        slot->proc = DrawBleed;
        break;
    }
    }

    {
        s32 size;
        s32 otz;
        s32 sort_depth;
        s32 priority;

        param->px += param->vx;
        param->py += param->vy;
        param->pz += param->vz;
        spr->rotate = param->rotate;
        spr->attribute = 0;
        spr->r = param->brightness;
        spr->g = param->brightness;
        spr->b = param->brightness;
        size = param->scale;
        GetScreenPosition(param->px, param->py, param->pz, &vector);
        otz = vector.vz;
        if (otz <= NEAR_DEPTH)
        {
            return;
        }
        spr->scalex = spr->scaley =
            (s16)((size * PROJECTION_DISTANCE) / otz) + 1;
        spr->x = vector.vx;
        spr->y = vector.vy;
        sort_depth = (s16)(u16)vector.vz >> 2;
        CLAMP_SORT_DEPTH(priority, sort_depth);
        GsSortSprite(spr, OTablePt, (u16)priority);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetGore(struct VECTOR *pos, struct SVECTOR *vec, int time, long col);
 *     EFFECT.C:1161, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       struct SVECTOR * vec
 *     param $a2       int time
 *     param $a3       long col
 * END PSX.SYM */

void SetGore(GsCOORDINATE2 *coord, SVECTOR *local_position,
             SVECTOR *local_velocity)
{
    enum
    {
        GORE_INITIAL_SCALE = 2 * FIXED_ONE,
        GORE_TIME_SPREAD = 15,
        GORE_TIME_MIN = 10,
        GORE_INITIAL_BRIGHTNESS = 0x80,
        GORE_IMPACT_INTERVAL = 4,
        GORE_IMPACT_ROTATE_SPEED = 80,
        GORE_IMPACT_SIZE = 2 * FIXED_ONE,
        GORE_IMPACT_DURATION = 3,
        GORE_IMPACT_SPRITE = 2
    };
    VECTOR world_position;
    MATRIX local_to_world;
    SVECTOR velocity_copy;
    VECTOR world_velocity;
    VECTOR impact_position;
    long transform_flags[2];
    u32 impact_phase;

    GsGetLw(coord, &local_to_world);
    GsSetLsMatrix(&local_to_world);
    RotTrans(local_position, &world_position, transform_flags);

    {
        int gore_index;
        TEffectSlot *gore_slot;
        int gore_slots_searched;
        BloodType *gore;

        FIND_EFFECT_SLOT(gore_index, gore_slots_searched, gore_slot, gore_found);
    gore_found:
        gore = &gore_slot->param.blood;
        gore->sprite = rand() % N_AIRBORNE_BLOOD_SPRITES;
        gore->scale = GORE_INITIAL_SCALE;
        gore->rotate = (rand() % 360) * FIXED_ONE;
        gore->px = world_position.vx;
        gore->py = world_position.vy;
        gore->pz = world_position.vz;
        velocity_copy = *local_velocity;
        ApplyRotMatrix(&velocity_copy, &world_velocity);
        gore->vx = (short)world_velocity.vx;
        gore->vy = (short)world_velocity.vy;
        gore->vz = (short)world_velocity.vz;
        gore->time = rand() % GORE_TIME_SPREAD + GORE_TIME_MIN;
        gore->hint = 0;
        gore->brightness = GORE_INITIAL_BRIGHTNESS;
        gore->mode = GORE_MODE_AIRBORNE;
        impact_phase = GameClock & (GORE_IMPACT_INTERVAL - 1);
        gore_slot->proc = DrawGore;
    }

    if (impact_phase == 0)
    {
        int impact_index;
        TEffectSlot *impact_slot;
        int impact_slots_searched;
        ImpactType *impact;
        long impact_pz;
        long start_color;
        long end_color;

        start_color = COLOR_GRAY;
        end_color = COLOR_GRAY;
        impact_position.vx = local_position->vx;
        impact_position.vy = local_position->vy;
        impact_position.vz = local_position->vz;
        FIND_EFFECT_SLOT(impact_index, impact_slots_searched,
                         impact_slot, impact_found);
    impact_found:
        impact_slot->proc = DrawImpact;
        impact_slot->param.impact.px = impact_position.vx;
        impact = &impact_slot->param.impact;
        impact->py = impact_position.vy;
        impact_pz = impact_position.vz;
        impact->rotate_speed = GORE_IMPACT_ROTATE_SPEED;
        impact->start_size = GORE_IMPACT_SIZE;
        impact->end_size = GORE_IMPACT_SIZE;
        impact->time = GORE_IMPACT_DURATION;
        impact->super = coord;
        impact->rotate = 0;
        impact->start_color.word = start_color;
        impact->end_color.word = end_color;
        impact->count = 0;
        impact->type = GORE_IMPACT_SPRITE;
        impact->pz = impact_pz;
    }
}

void draw_fade_(TEffectSlot *ef)
{
    FadeType *fade;
    POLY_XF4 local;
    POLY_XF4 *ply;
    long elapsed;
    u32 duration;
    u8 r;
    u8 g;
    u8 b;
    u8 mode;

    fade = &ef->param.fade;
    SetPolyXF4(&local, GPU_BLEND_ADD);
    local.ply.x0 = -SCREEN_W / 2;
    local.ply.y0 = -SCREEN_H / 2;
    local.ply.x1 = SCREEN_W / 2;
    local.ply.y1 = -SCREEN_H / 2;
    local.ply.x2 = -SCREEN_W / 2;
    local.ply.y2 = SCREEN_H / 2;
    local.ply.x3 = SCREEN_W / 2;
    local.ply.y3 = SCREEN_H / 2;

    mode = fade->mode;
    elapsed = GameClock - fade->start_time;
    duration = fade->end_time - fade->start_time;
    switch (mode)
    {
    case FADE_MODE_IN:
        r = (elapsed * fade->r) / duration;
        g = (elapsed * fade->g) / duration;
        b = (elapsed * fade->b) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        if ((u32)GameClock >= (u32)fade->end_time)
        {
            fade->mode++;
            fade->end_time += 3;
        }
        break;
    case FADE_MODE_HOLD:
        local.ply.r0 = fade->r;
        local.ply.g0 = fade->g;
        local.ply.b0 = fade->b;
        if ((u32)GameClock >= (u32)fade->end_time)
        {
            fade->mode++;
            fade->end_time += 0x28;
        }
        break;
    case FADE_MODE_OUT:
        if ((u32)GameClock >= (u32)fade->end_time)
        {
            ef->proc = 0;
            return;
        }
        r = ((duration - elapsed) * fade->r) / duration;
        g = ((duration - elapsed) * fade->g) / duration;
        b = ((duration - elapsed) * fade->b) / duration;
        local.ply.r0 = r;
        local.ply.g0 = g;
        local.ply.b0 = b;
        break;
    }

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    *ply = local;
    AddXF4(OTablePt->org + fade->priority, ply);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawExplosion(struct tag_EffectSlot *ef);
 *     EFFECT.C:1177, 57 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a1       struct tag_EffectSlot * ef
 *     reg   $a2       struct ExplosionType * param
 *     reg   $s0       struct Sprite3D * spr
 *     reg   $a3       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprBomb[3];
 * END PSX.SYM */

static void DrawExplosion(TEffectSlot *ef)
{
    enum
    {
        fo = 5
    };
    ExplosionType *param;
    Sprite3D *spr;
    u8 alfa;
    long rotate;

    param = &ef->param.explosion;
    alfa = 0x80;
    switch (param->mode)
    {
    case EXPLOSION_MODE_FLASH:
        if (param->time == 0)
        {
            param->time = 3;
            param->mode++;
        }
        else
        {
            param->scale += 0x2000;
            param->rotate += 100 * FIXED_ONE; /* 100 deg/frame */
        }
        spr = sprBomb[BOMB_SPRITE_FLASH];
        break;
    case EXPLOSION_MODE_EXPAND:
        if (param->time == 0)
        {
            param->time = fo;
            param->mode++;
        }
        param->scale += 0x2000;
        param->rotate += 100 * FIXED_ONE; /* 100 deg/frame */
        spr = sprBomb[BOMB_SPRITE_EXPANDED];
        break;
    case EXPLOSION_MODE_FADE:
        alfa = (u8)((param->time << 7) / fo);
        param->scale -= 0x333;
        param->rotate += 90 * FIXED_ONE; /* 90 deg/frame */
        if (param->time == 0)
        {
            ef->proc = 0;
        }
        spr = sprBomb[BOMB_SPRITE_EXPANDED];
        break;
    }
    param->time--;
    param->pos.vx += param->vec.vx;
    param->pos.vy += param->vec.vy;
    param->pos.vz += param->vec.vz;
    spr->locate.coord.t[0] = param->pos.vx;
    spr->locate.coord.t[1] = param->pos.vy;
    spr->locate.coord.t[2] = param->pos.vz;
    spr->scale = param->scale;
    rotate = param->rotate;
    spr->sprite.b = spr->sprite.g = spr->sprite.r = alfa;
    spr->sprite.rotate = rotate;
    UpdateCoordinate((ModelType *)spr);
    DrawSprite(spr);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetExplosion(struct VECTOR *pos, struct SVECTOR *vect);
 *     EFFECT.C:1236, 18 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct VECTOR * pos
 *     param $s3       struct SVECTOR * vect
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct ExplosionType * param
 *     reg   $v1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetExplosion(VECTOR *pos, SVECTOR *vect)
{
    int idx;
    TEffectSlot *slot;
    int count;
    ExplosionType *param;
    int r;
    short vz;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    param = &slot->param.explosion;
    param->scale = FIXED_ONE;
    r = rand();
    param->rotate = (r % 360) * FIXED_ONE;
    param->pos.vx = pos->vx;
    param->pos.vy = pos->vy;
    param->pos.vz = pos->vz;
    param->vec.vx = vect->vx;
    param->vec.vy = vect->vy;
    vz = vect->vz;
    param->time = 5;
    param->mode = EXPLOSION_MODE_FLASH;
    param->vec.vz = vz;
    slot->proc = DrawExplosion;
    SetBleeds(pos, 200, 150, 20, 30, COLOR_YELLOW);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawHinoko(struct tag_EffectSlot *ef);
 *     EFFECT.C:1258, 61 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a1       struct tag_EffectSlot * ef
 *     reg   $a2       struct ExplosionType * param
 *     reg   $s0       struct Sprite3D * spr
 *     reg   $a3       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprBomb[3];
 * END PSX.SYM */

/* Originally static in EFFECT.C; global here because SetHinoko is split into
 * a separate translation unit and stores this function's address. */
static void DrawHinoko(TEffectSlot *ef)
{
    enum
    {
        fo = 30
    };
    ExplosionType *param;
    Sprite3D *spr;
    u8 alfa;

    param = &ef->param.hinoko;
    spr = sprBomb[BOMB_SPRITE_HINOKO];
    alfa = 0x80;
    switch (param->mode)
    {
    case EXPLOSION_MODE_FLASH:
        if (param->time == 0)
        {
            param->mode = EXPLOSION_MODE_EXPAND;
            param->time = fo;
        }
        break;
    case EXPLOSION_MODE_EXPAND:
        alfa = (u8)((param->time * 0x80) / fo);
        if (param->time == 0)
        {
            ef->proc = 0;
        }
        break;
    }

    param->time--;
    param->vec.vy += 5;
    param->scale += 0xc00;
    param->pos.vx += param->vec.vx;
    param->pos.vy += param->vec.vy;
    param->pos.vz += param->vec.vz;
    spr->locate.coord.t[0] = param->pos.vx;
    spr->locate.coord.t[1] = param->pos.vy;
    spr->locate.coord.t[2] = param->pos.vz;
    spr->scale = param->scale;
    spr->sprite.rotate = param->rotate;
    spr->sprite.b = spr->sprite.g = spr->sprite.r = alfa;
    UpdateCoordinate((ModelType *)spr);
    DrawSprite(spr);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetHinoko(struct VECTOR *pos, struct SVECTOR *power, int n);
 *     EFFECT.C:1321, 21 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct VECTOR * pos
 *     param $s4       struct SVECTOR * power
 *     param $s5       int n
 *     reg   $s2       short i
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct ExplosionType * param
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetHinoko(VECTOR *pos, SVECTOR *power, int n)
{
    int idx;
    TEffectSlot *slot;
    int count;
    ExplosionType *param;
    short i;
    int r;

    i = 0;
    while (1)
    {
        if (i >= n)
        {
            break;
        }
        FIND_EFFECT_SLOT(idx, count, slot, found);
    found:
        param = &slot->param.hinoko;
        param->scale = rand() % FIXED_ONE + FIXED_ONE;
        param->rotate = (rand() % 360) * FIXED_ONE;
        param->pos.vx = pos->vx;
        param->pos.vy = pos->vy;
        param->pos.vz = pos->vz;
        param->vec.vx = rand() % power->vx - power->vx / 2;
        param->vec.vy = -(rand() % power->vy + power->vy / 2);
        param->vec.vz = rand() % power->vz - power->vz / 2;
        r = rand();
        i++;
        param->mode = EXPLOSION_MODE_FLASH;
        param->time = r % 15 + 15;
        slot->proc = DrawHinoko;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawFlyWire(struct tag_EffectSlot *ef);
 *     EFFECT.C:1346, 36 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_EffectSlot * ef
 *     reg   $s0       struct FlyWireType * param
 *     stack sp+24     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

static void DrawFlyWire(TEffectSlot *ef)
{
    enum
    {
        FLYWIRE_STRAIGHTEN_FRAMES = 5
    };
    FlyWireType *param;

    param = &ef->param.flywire;
    switch (param->mode)
    {
    case FLYWIRE_MODE_EXTEND:
    {
        s16 time;
        s32 sum;

        time = param->time;
        sum = (u16)param->count + FIXED_ONE / time;
        param->count = sum;
        if ((s16)sum > FIXED_ONE)
        {
            param->count = 0;
            param->mode++;
            SetBleeds(&param->end, 0, 50, 10, 30, COLOR_YELLOW);
            Sound(CamState.Owner, SE_PROJECTILE_IMPACT);
        }
        else
        {
            SetWire(&param->start, &param->end, &param->center, (s16)sum);
        }
        return;
    }
    case FLYWIRE_MODE_STRAIGHTEN:
    {
        VECTOR pos = {
            .vx = ((param->center.vx *
                    (FLYWIRE_STRAIGHTEN_FRAMES - param->count)) +
                   (param->NCenter.vx * param->count)) /
                FLYWIRE_STRAIGHTEN_FRAMES,
            .vy = ((param->center.vy *
                    (FLYWIRE_STRAIGHTEN_FRAMES - param->count)) +
                   (param->NCenter.vy * param->count)) /
                FLYWIRE_STRAIGHTEN_FRAMES,
            .vz = ((param->center.vz *
                    (FLYWIRE_STRAIGHTEN_FRAMES - param->count)) +
                   (param->NCenter.vz * param->count)) /
                FLYWIRE_STRAIGHTEN_FRAMES
        };

        SetWire(&param->start, &param->end, &pos, FIXED_ONE);
        if (param->count >= FLYWIRE_STRAIGHTEN_FRAMES)
        {
            ef->proc = 0;
        }
        param->count++;
        return;
    }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int SetFlyWire(struct VECTOR *start, struct VECTOR *end);
 *     EFFECT.C:1384, 40 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a2       struct VECTOR * start
 *     param $a3       struct VECTOR * end
 *     reg   $s5       struct tag_EffectSlot * slot
 *     reg   $s3       struct FlyWireType * param
 *     reg   $s2       int dist
 *     reg   $a1       int i
 *     reg   $s3       struct VECTOR * v1
 *     reg   $a0       long dz
 *     reg   $a2       long dy
 *     reg   $t0       long dx
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

int SetFlyWire(VECTOR *start, VECTOR *end)
{
    TEffectSlot *slot;
    FlyWireType *param;
    int idx;
    int i;
    int dist;
    int result;

    FIND_EFFECT_SLOT(idx, i, slot, found);

found:
    param = &slot->param.flywire;
    param->start = *start;
    param->end = *end;
    param->count = 0;
    param->mode = FLYWIRE_MODE_EXTEND;

    {
        VECTOR *v1;
        long dx;
        long dy;
        long dz;
        int big;
        long root;
        long base_x;
        long base_y;
        long base_z;
        long value_x;
        long value_y;
        long value_z;

        v1 = &param->end;
        dx = param->start.vx - v1->vx;
        dy = param->start.vy - v1->vy;
        dz = param->start.vz - v1->vz;

        big = 0;
        if (abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
            abs(dz) > FIXED_ONE)
        {
            big = 1;
        }
        if (big)
        {
            dx /= 0x100;
            dy /= 0x100;
            dz /= 0x100;
            root = SquareRoot0(dx * dx + dy * dy + dz * dz) << 8;
        }
        else
        {
            root = SquareRoot0(dx * dx + dy * dy + dz * dz);
        }
        dist = root;

        param->NCenter.vx = (param->start.vx + param->end.vx) / 2;
        param->NCenter.vy = (param->start.vy + param->end.vy) / 2;
        param->NCenter.vz = (param->start.vz + param->end.vz) / 2;
        param->time = dist / 1000;

        dist /= 16;

        base_x = param->NCenter.vx;
        if (dist * 2 > 0)
        {
            value_x = base_x + (rand() % (dist * 2) - dist);
        }
        else
        {
            value_x = base_x - dist;
        }
        param->center.vx = value_x;

        base_y = param->NCenter.vy;
        if (dist > 0)
        {
            value_y = base_y + (rand() % dist - dist);
        }
        else
        {
            value_y = base_y - dist;
        }
        param->center.vy = value_y;

        base_z = param->NCenter.vz;
        if (dist * 2 > 0)
        {
            value_z = base_z + (rand() % (dist * 2) - dist);
        }
        else
        {
            value_z = base_z - dist;
        }
        param->center.vz = value_z;
    }

    if (param->time > 0)
    {
        slot->proc = DrawFlyWire;
        result = param->time + 5;
    }
    else
    {
        result = 0;
    }
    return result;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetWire(struct VECTOR *start, struct VECTOR *end, struct VECTOR *center, long len);
 *     EFFECT.C:1428, 68 src lines, frame 120 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct VECTOR * start
 *     param $s2       struct VECTOR * end
 *     param $s1       struct VECTOR * center
 *     param $s0       long len
 *     stack sp+16     struct VECTOR StockCenter
 *     reg   $s4       long lcount
 *     reg   $s0       int i
 *     reg   $s5       int ecount
 *     stack sp+32     struct SVECTOR scr
 *     stack sp+40     struct SVECTOR oldscr
 *     stack sp+72     int x
 *     stack sp+76     int y
 *     reg   $s6       int z
 *     stack sp+48     struct GsLINE line
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s3       struct VECTOR * v1
 *     reg   $s2       struct VECTOR * v2
 *     reg   $a1       long dz
 *     reg   $a3       long dy
 *     reg   $t0       long dx
 *     reg   $v0       long t
 *     reg   $a0       long Q
 *     reg   $a2       long R
 *     stack sp+72     long x
 *     stack sp+76     long y
 *     reg   $s6       long z
 *     reg   $v1       int z
 *     stack sp+64     int rx
 *     stack sp+68     int ry
 *     reg   $s3       struct VECTOR * start
 *     reg   $s2       struct VECTOR * end
 *     reg   $s0       int dz
 *     reg   $s2       int dy
 *     reg   $s1       int dx
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT *OTablePt;
 *     extern struct ModelType *ModelHook;
 * END PSX.SYM */

static inline void GetWireScreenPosition(long x, long y, long z,
                                         SVECTOR *screen)
{
    MATRIX *matrix = SCREEN_PROJECTION_MATRIX;
    SVECTOR *vector = SCREEN_PROJECTION_POINT;

    matrix->t[0] = 0;
    matrix->t[1] = 0;
    matrix->t[2] = 0;
    setVector(vector, x - ViewInfo.vpx, y - ViewInfo.vpy, z - ViewInfo.vpz);
    SetTransMatrix(matrix);
    SetRotMatrix(&GsWSMATRIX);
    screen->vz = (s16)RotTransPers(vector, (s32 *)screen,
                                   SCREEN_PROJECTION_PERSPECTIVE,
                                   SCREEN_PROJECTION_FLAG);
}

static inline void GetWireRotation(VECTOR *start, VECTOR *end, int *rx,
                                   int *ry)
{
    int dx, dy, dz;

    dx = end->vx - start->vx;
    dz = end->vz - start->vz;
    dy = end->vy - start->vy;
    *ry = ratan2(-dx, -dz);
    *rx = ratan2(dy, SquareRoot0(dx * dx + dz * dz));
}

void SetWire(VECTOR *start, VECTOR *end, VECTOR *center, long len)
{
    VECTOR StockCenter;
    long lcount;
    int i;
    int ecount;
    SVECTOR scr;
    SVECTOR oldscr;
    int x, y, z;
    GsLINE line;
    long distance;

    GetWireScreenPosition(start->vx, start->vy, start->vz, &oldscr);

    line.attribute = 0;
    line.r = 0x50;
    line.g = 0x48;
    line.b = 0x38;

    {
        VECTOR *v1;
        VECTOR *v2;
        long dx, dy, dz;
        int big;

        v1 = start;
        v2 = end;
        dx = v1->vx - v2->vx;
        dy = v1->vy - v2->vy;
        dz = v1->vz - v2->vz;
        big = 0;
        if (abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
            abs(dz) > FIXED_ONE)
        {
            big = 1;
        }
        if (big)
        {
            dx /= 0x100;
            dy /= 0x100;
            dz /= 0x100;
            distance = SquareRoot0(dx * dx + dy * dy + dz * dz) << 8;
        }
        else
        {
            distance = SquareRoot0(dx * dx + dy * dy + dz * dz);
        }
    }

    lcount = distance / WIRE_SEG_LEN;
    if (center == 0)
    {
        StockCenter.vx = (end->vx + start->vx) / 2;
        center = &StockCenter;
        center->vy = (end->vy + start->vy) / 2 + distance / WIRE_SAG_DIV;
        center->vz = (end->vz + start->vz) / 2;
    }

    ecount = lcount * len / FIXED_ONE;
    i = 0;
    while (1)
    {
        long t, Q, R;
        long one_value;

        if (i >= ecount)
        {
            break;
        }

        one_value = FIXED_ONE;
        t = one_value - i * FIXED_ONE / lcount;
        Q = t * 2;
        R = t * t / FIXED_ONE;
        x = ((one_value - Q + R) * end->vx +
             (Q - R * 2) * center->vx + R * start->vx) / FIXED_ONE;
        y = ((one_value - Q + R) * end->vy +
             (Q - R * 2) * center->vy + R * start->vy) / FIXED_ONE;
        z = ((one_value - Q + R) * end->vz +
             (Q - R * 2) * center->vz + R * start->vz) / FIXED_ONE;

        GetWireScreenPosition(x, y, z, &scr);

        if (scr.vz > 0 && oldscr.vz > 0)
        {
            int z;
            int p;

            line.x0 = oldscr.vx;
            line.y0 = oldscr.vy;
            z = scr.vz >> 2;
            line.x1 = scr.vx;
            line.y1 = scr.vy;
            CLAMP_SORT_DEPTH(p, z);
            GsSortLine(&line, OTablePt, (u16)p);
        }
        oldscr = scr;
        i++;
    }

    {
        int rx, ry;

        GetWireRotation(start, end, &rx, &ry);
        ModelHook->locate.coord.t[0] = x;
        ModelHook->locate.coord.t[1] = y;
        ModelHook->locate.coord.t[2] = z;
        ModelHook->rotate.vx = rx;
        ModelHook->rotate.vy = ry;
        ModelHook->rotate.vz = 0;
        UpdateCoordinate(ModelHook);
        DrawModel(ModelHook);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetLightningI(struct VECTOR *start, struct VECTOR *end, int gen, short r, int g, int b);
 *     EFFECT.C:1500, 60 src lines, frame 152 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s6       struct VECTOR * start
 *     param $s7       struct VECTOR * end
 *     param stack+4294967216 int gen
 *     param stack+4294967224 short r
 *     param stack+16  int g
 *     param stack+20  int b
 *     stack sp+88     short g
 *     stack sp+96     short b
 *     reg   $s0       long lcount
 *     reg   $s4       int i
 *     stack sp+24     struct SVECTOR scr
 *     stack sp+32     struct SVECTOR oldscr
 *     reg   $s1       int x
 *     reg   $s2       int y
 *     reg   $s3       int z
 *     stack sp+40     struct GsLINE line
 *     reg   $v0       long x
 *     reg   $t1       long y
 *     reg   $t2       long z
 *     reg   $s6       struct VECTOR * v1
 *     reg   $s7       struct VECTOR * v2
 *     reg   $a1       long dz
 *     reg   $a3       long dy
 *     reg   $t0       long dx
 *     stack sp+56     struct VECTOR sv
 *     reg   $s1       long x
 *     reg   $s2       long y
 *     reg   $s3       long z
 *     reg   $fp       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

static inline void PrepareLightningScreenPosition(void)
{
    MATRIX *matrix = (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS;

    matrix->t[0] = 0;
    matrix->t[1] = 0;
    matrix->t[2] = 0;
    SetTransMatrix(matrix);
    SetRotMatrix(&GsWSMATRIX);
}

static inline void GetLightningScreenPosition(long x, long y, long z,
                                              SVECTOR *screen)
{
    SVECTOR *vector = (SVECTOR *)TENCHU_SCRATCHPAD(0x80);

    vector->vx = x - (short)ViewInfo.vpx;
    vector->vy = y - (short)ViewInfo.vpy;
    vector->vz = z - (short)ViewInfo.vpz;
    screen->vz = (s16)RotTransPers(
        vector, (s32 *)screen, (s32 *)TENCHU_SCRATCHPAD_ADDRESS,
        (s32 *)TENCHU_SCRATCHPAD(0x10));
}

static void SetLightningI(VECTOR *start, VECTOR *end, int gen, short r, short g, short b)
{
    enum
    {
        SplitLen = 200
    };
    enum
    {
        Range = 80
    };
    SVECTOR scr;
    SVECTOR oldscr;
    GsLINE line;
    int next_gen;
    short lr;
    short lg;
    short lb;
    long distance;
    long lcount;
    int i;
    int x;
    int y;
    int z;

    lr = r;
    next_gen = gen - 1;
    lg = g;
    lb = b;
    if (gen != 0)
    {
        PrepareLightningScreenPosition();
        GetLightningScreenPosition(start->vx, start->vy, start->vz, &oldscr);

        line.attribute = GS_ATTR_SEMITRANS_ADD;
        line.r = lr;
        line.g = lg;
        line.b = lb;

        {
            VECTOR *v1, *v2;
            long dx, dy, dz;
            int large;

            v1 = start;
            v2 = end;
            large = 0;
            dx = v1->vx - v2->vx;
            dy = v1->vy - v2->vy;
            dz = v1->vz - v2->vz;
            if (abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
                abs(dz) > FIXED_ONE)
            {
                large = 1;
            }
            if (large)
            {
                dx /= 0x100;
                dy /= 0x100;
                dz /= 0x100;
                distance = SquareRoot0(dx * dx + dy * dy + dz * dz) << 8;
            }
            else
            {
                distance = SquareRoot0(dx * dx + dy * dy + dz * dz);
            }
        }

        lcount = distance / SplitLen;
        i = 1;
        if (lcount > 0)
        {
            while (1)
            {
                if (i >= lcount)
                {
                    break;
                }

                x = ((end->vx - start->vx) * i) / lcount + start->vx;
                y = ((end->vy - start->vy) * i) / lcount + start->vy;
                z = ((end->vz - start->vz) * i) / lcount + start->vz;
                x += -Range + rand() % (Range * 2);
                y += -Range + rand() % (Range * 2);
                z += -Range + rand() % (Range * 2);

                if ((rand() & 2) == 0)
                {
                    VECTOR sv = {
                        .vx = x,
                        .vy = y,
                        .vz = z
                    };

                    SetLightningI(&sv, end, next_gen, lr, lg, lb);
                }

                GetLightningScreenPosition(x, y, z, &scr);

                if (scr.vz > 0 && oldscr.vz > 0)
                {
                    int z;
                    int p;

                    line.x0 = oldscr.vx;
                    line.y0 = oldscr.vy;
                    z = scr.vz >> 2;
                    line.x1 = scr.vx;
                    line.y1 = scr.vy;
                    if (z >= 0)
                    {
                        if (z < DEPTH_LIMIT)
                            p = z;
                        else
                            p = DEPTH_LIMIT - 1;
                    }
                    else
                    {
                        p = 0;
                    }
                    GsSortLine(&line, OTablePt, (u16)p);
                }
                oldscr = scr;
                i++;
            }
        }
    }
}

/*
 * The union models mutually exclusive stack scratch for the two branches;
 * the original source form is still uncertain.
 */
void spawn_damage_effect_(Humanoid *human, DamageEffectKind kind)
{
    union
    {
        PARAM_ITEM_LAUNCH launch;
        struct
        {
            VECTOR pos;
            VECTOR scratch;
        } blood;
    } work;

    if (kind != DAMAGE_EFFECT_ATTACHED_FLASH)
    {
        s32 x;
        s32 y;
        s32 z;
        s32 vx;
        s32 vz;

        work.launch.type = ITEM_NAPALM;
        work.launch.user = human;
        /* The start.vy/end.vx/vy/vz double stores below are retail's own
         * (both writes of each pair are in the bytes). */
        x = human->model->locate.coord.t[0];
        work.launch.start.vx = x;
        y = human->model->locate.coord.t[1];
        work.launch.start.vy = y;
        z = human->model->locate.coord.t[2];
        work.launch.start.vz = z;
        work.launch.start.vy = y - 100;
        work.launch.end.vx = x;
        work.launch.end.vy = y - 100;
        work.launch.end.vz = z;
        vx = human->vector.vx;
        work.launch.end.vx = x + vx;
        vz = human->vector.vz;
        work.launch.end.vy = y - 115;
        work.launch.end.vz = z + vz;
        ReqItemUse(&work.launch);
    }
    else
    {
        ModelType **objects;
        ModelType *model;
        VECTOR *position_base;
        VECTOR *position;
        short time;
        int idx;
        int count;
        TEffectSlot *slot;
        FrameType *frame;

        objects = human->model->object;
        if (human->model->n > 0)
        {
            objects += rand() % human->model->n;
        }
        model = *objects;

        memset(&work.blood.scratch, 0, sizeof(VECTOR));
        work.blood.scratch.vx = rand() % 200 - 100;
        work.blood.scratch.vy = rand() % 200 - 100;
        work.blood.scratch.vz = rand() % 200 - 100;
        work.blood.pos = work.blood.scratch;
        position_base = &work.blood.pos;

        *(SVECTOR *)&work.blood.scratch = (SVECTOR){
            .vx = 0,
            .vy = -60,
            .vz = 0
        };
        time = rand() % 60 + 60;
        position = position_base;

        FIND_EFFECT_SLOT(idx, count, slot, found);
    found:
        frame = &slot->param.frame;
        frame->px = position->vx;
        frame->py = position->vy;
        frame->pz = position->vz;
        frame->mode = FRAME_MODE_FLASH;
        frame->size = 3 * FIXED_ONE;
        frame->count = time;
        frame->super = &model->locate;
        slot->proc = DrawFrame;

        SetBleedsDir(GetAbsolutePosition(model, 0, 0, 0),
                     (SVECTOR *)&work.blood.scratch,
                     100, 10, 30, RGB24(100, 100, 60));
        SoundEx(MODEL_POSITION(human->model), SE_LIGHTNING);
    }
}

void spread_blood_pool_(Humanoid *human)
{
    VECTOR scale;
    MATRIX matrix;
    SVECTOR screen;
    s32 p;
    s32 flag;
    VECTOR *position;
    s32 *chase;
    s32 timer;
    s32 height;
    s32 scaled;
    s32 depth;

    chase = human->chase;
    if (human->motion->loop >= 0 || (human->map.attrib & (MAP_WATER | MAP_WOOD)) != 0 ||
        human->chase[0] < 0)
    {
        chase[0] = 0;
        return;
    }

    if ((human->map.attrib & MAP_DAMAGE) != 0)
    {
        if ((GameClock & 0xf) == 0)
        {
            spawn_damage_effect_(human, DAMAGE_EFFECT_ATTACHED_FLASH);
        }
        return;
    }

    timer = human->chase[0] + 0x88;
    human->chase[0] = timer;
    if (timer > FIXED_ONE)
    {
        human->chase[0] = FIXED_ONE;
    }

    position = GetAbsolutePosition(human->model->object[MODEL_PART_WAIST], 0, 0, 0);
    height = human->model->rotate.pad;
    position->vy = human->model->locate.coord.t[1];
    BLOOD_POOL_MODEL_->locate.coord.t[0] = position->vx;
    BLOOD_POOL_MODEL_->locate.coord.t[1] = position->vy;
    BLOOD_POOL_MODEL_->locate.coord.t[2] = position->vz;

    scaled = human->chase[0] * -height / 1024;
    scale.vx = scale.vy = scale.vz =
        scaled - (human->map.height >> 1);

    if (human->map.angleH != 0)
    {
        BLOOD_POOL_MODEL_->rotate.vx = 0x100;
        BLOOD_POOL_MODEL_->rotate.vy = RefrectVector[human->map.angleH];
        BLOOD_POOL_MODEL_->rotate.vz = 0;
    }
    else
    {
        BLOOD_POOL_MODEL_->rotate.vx = 0;
        BLOOD_POOL_MODEL_->rotate.vy = 0;
        BLOOD_POOL_MODEL_->rotate.vz = 0;
    }

    RotMatrixYXZ(&BLOOD_POOL_MODEL_->rotate,
                 &BLOOD_POOL_MODEL_->locate.coord);
    ScaleMatrix(&BLOOD_POOL_MODEL_->locate.coord, &scale);
    BLOOD_POOL_MODEL_->locate.flg = 0;
    GsGetLs(&BLOOD_POOL_MODEL_->locate, &matrix);
    GsSetLsMatrix(&matrix);
    depth = RotTransPers(&UnitVector, (s32 *)&screen, &p, &flag);
    screen.vz = depth;
    if ((s16)depth >> 2 < DEPTH_LIMIT)
    {
        DrawTMDmode = TMD_BANK_FOG;
        DrawTMD(&BLOOD_POOL_MODEL_->object, OTablePt, 0);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawShadow(struct Humanoid *human);
 *     EFFECT.C:1572, 89 src lines, frame 168 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct Humanoid * human
 *     stack sp+24     struct VECTOR scl
 *     reg   $s1       struct VECTOR * loc
 *     stack sp+40     struct MATRIX mat
 *     reg   $s0       int height
 *     reg   $s1       struct VECTOR * pos
 *     reg   $v1       struct SplashType * param
 *     reg   $a1       struct tag_EffectSlot * slot
 *     reg   $a2       int i
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct ModelType * model
 *     stack sp+112    struct VECTOR pos
 *     stack sp+128    struct SVECTOR sv
 *     reg   $t2       short time
 *     reg   $s1       struct _GsCOORDINATE2 * super
 *     reg   $v1       struct FrameType * param
 *     reg   $t1       struct tag_EffectSlot * slot
 *     reg   $a2       int i
 *     stack sp+72     struct SVECTOR scr
 *     stack sp+148    long flag
 *     stack sp+144    long p
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern struct ModelType *ShadowMdl;
 *     extern short RefrectVector[16];
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

void DrawShadow(Humanoid *human)
{
    VECTOR scl;
    MATRIX mat;
    SVECTOR scr;
    s32 p;
    s32 flag;
    VECTOR *position;
    s32 height;

    height = -human->model->rotate.pad;
    position = GetAbsolutePosition(human->model->object[MODEL_PART_WAIST], 0, 0, 0);

    if (human->map.level < position->vy || human->map.level == LEVEL_NONE)
    {
        human->map.attrib |= MAP_WOOD; /* airborne overload */
    }
    position->vy = human->map.level;
    if (human->map.attrib & MAP_WOOD)
    {
        if ((human->vector.vx != 0 || human->vector.vy != 0 ||
             human->motion->mid == MOT_STATE_LAND ||
             human->motion->mid == MOT_ATTACK_DIVE_LAND) &&
            human->map.height == 0 && (GameClock & 1) != 0)
        {
            s32 idx;
            s32 count;
            TEffectSlot *slot;
            SplashType *param;
            s32 z;

            if (human->status == STAT_SWIM)
            {
                s32 r = rand();

                position->vy += 100;
                position->vx += r % 600 - 300;
                position->vz += rand() % 600 - 300;
            }
            else
            {
                position->vx += rand() % 200 - 100;
                position->vz += rand() % 200 - 100;
            }

            FIND_EFFECT_SLOT(idx, count, slot, found);
        found:
            param = &slot->param.splash;
            param->px = position->vx;
            param->py = position->vy;
            z = position->vz;
            param->sx = 0x2000;
            param->sy = 0x2000;
            param->speed = 4;
            param->mode = SPLASH_MODE_SPAWN;
            param->pz = z;
            slot->proc = DrawSplash;
        }
    }
    else if (human->map.attrib & MAP_DAMAGE)
    {
        if (human->map.height == 0)
        {
            if ((GameClock & 0x3f) == 1)
            {
                spawn_damage_effect_(human, DAMAGE_EFFECT_NAPALM);
            }
            else if ((GameClock & 0xf) == 0)
            {
                spawn_damage_effect_(human, DAMAGE_EFFECT_ATTACHED_FLASH);
            }
        }
    }
    else
    {
        ShadowMdl->locate.coord.t[0] = position->vx;
        ShadowMdl->locate.coord.t[1] = position->vy;
        ShadowMdl->locate.coord.t[2] = position->vz;

        scl.vx = scl.vy = scl.vz = height * 4 - (human->map.height >> 1);
        if (human->map.angleH != 0)
        {
            ShadowMdl->rotate.vx = 0x100;
            ShadowMdl->rotate.vy = RefrectVector[human->map.angleH];
            ShadowMdl->rotate.vz = 0;
        }
        else
        {
            ShadowMdl->rotate.vx = 0;
            ShadowMdl->rotate.vy = 0;
            ShadowMdl->rotate.vz = 0;
        }

        RotMatrixYXZ(&ShadowMdl->rotate, &ShadowMdl->locate.coord);
        ScaleMatrix(&ShadowMdl->locate.coord, &scl);
        ShadowMdl->locate.flg = 0;
        GsGetLs(&ShadowMdl->locate, &mat);
        GsSetLsMatrix(&mat);
        scr.vz = RotTransPers(&UnitVector, (s32 *)&scr, &p, &flag);
        if (scr.vz >> 2 < DEPTH_LIMIT)
        {
            GsSortObject4(&ShadowMdl->object, OTablePt, 2,
                          (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawAfterimage(struct AfterimageType *afi, short disp);
 *     EFFECT.C:1717, 49 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct AfterimageType * afi
 *     param $a1       short disp
 *     reg   $s0       struct POLY_GT4 * poly
 *     reg   $s3       short i
 *     reg   $s1       short tplv
 *     stack sp+16     struct MATRIX mat
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

short DrawAfterimage(AfterimageType *afi, short disp)
{
    GpuPolyGT4Packet *poly;
    MATRIX mat;
    short i;
    s32 otz;
    s16 tplv;
    s32 pri;
    long tmp1, tmp2;

    tplv = 0x7f;
    poly = &afi->poly;

    if (disp != 0)
    {
        if (afi->n < afi->maxn - 1)
        {
            afi->n++;
        }
        for (i = afi->n - 1; i > 0; i--)
        {
            afi->p1[i] = afi->p1[i - 1];
            afi->p2[i] = afi->p2[i - 1];
        }
        GsGetLs(&afi->model->locate, &mat);
        GsSetLsMatrix(&mat);
        RotTransPers(&afi->vector1, (s32 *)afi->p1, 0, 0);
        RotTransPers(&afi->vector2, (s32 *)afi->p2, 0, 0);
        afi->sz = RotTransPers(&UnitVector, 0, 0, 0);
        if (afi->sz == 0)
        {
            return 0;
        }
    }
    else
    {
        if (afi->n <= 0)
        {
            return 0;
        }
        afi->n--;
    }

    poly->gpu.vertex[0].screen.word = afi->p1[0].word;
    poly->gpu.vertex[2].screen.word = afi->p2[0].word;

    i = 1;
    while (1)
    {
        if (i >= afi->n)
        {
            break;
        }
        poly->gpu.vertex[1].screen.word = poly->gpu.vertex[0].screen.word;
        tmp1 = afi->p1[i].word;
        poly->gpu.vertex[3].screen.word = poly->gpu.vertex[2].screen.word;
        poly->gpu.vertex[0].screen.word = tmp1;
        tmp2 = afi->p2[i].word;
        poly->packet.r1 = poly->packet.g1 = poly->packet.b1 = tplv;
        poly->packet.r3 = poly->packet.g3 = poly->packet.b3 = tplv;
        poly->gpu.vertex[2].screen.word = tmp2;

        tplv = ((afi->n - i) * 127) / afi->n;
        poly->packet.r0 = poly->packet.g0 = poly->packet.b0 = tplv;
        poly->packet.r2 = poly->packet.g2 = poly->packet.b2 = tplv;

        otz = afi->sz;
        otz = otz >> 2;
        CLAMP_SORT_DEPTH(pri, otz);
        GsSortPoly(&poly->packet, OTablePt, (u16)pri);
        i++;
    }

    return afi->n;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void FadeOutDirect(short time, short attrib, unsigned char r, unsigned char g, int b);
 *     EFFECT.C:1816, 32 src lines, frame 296 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short time
 *     param $a1       short attrib
 *     param $a2       unsigned char r
 *     param $a3       unsigned char g
 *     param stack+16  int b
 *     stack sp+16     struct DISPENV o_disp
 *     stack sp+40     struct DRAWENV o_draw
 *     stack sp+136    struct DRAWENV n_draw
 *     stack sp+232    struct POLY_XF4 ply
 * END PSX.SYM */

void FadeOutDirect(short time, short attrib, u8 r, u8 g, u8 b)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;
    POLY_XF4 ply;
    POLY_XF4 *packet;

    GetDrawEnv(&o_draw);
    GetDispEnv(&o_disp);
    n_draw = o_draw;
    n_draw.clip = o_disp.disp;
    n_draw.ofs[0] = o_disp.disp.x;
    n_draw.ofs[1] = o_disp.disp.y;
    PutDrawEnv(&n_draw);
    packet = &ply;
    setPolyF4(&packet->ply);
    setSemiTrans(&ply.ply, 1);
    setlen(&packet->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply.tpage.code[0] = GPU_DRAWMODE_BLEND(attrib) | GPU_DRAWMODE_DITHER;
    ply.ply.x0 = 0;
    ply.ply.y0 = 0;
    ply.ply.y1 = 0;
    ply.ply.x2 = 0;
    ply.ply.r0 = r;
    ply.ply.g0 = g;
    ply.ply.b0 = b;
    ply.ply.x1 = o_disp.disp.w;
    ply.ply.y2 = o_disp.disp.h;
    ply.ply.x3 = o_disp.disp.w;
    ply.ply.y3 = o_disp.disp.h;
loop:
    if (time == 0)
    {
        goto end;
    }
    DrawPrim((u8 *)&ply.ply);
    DrawPrim((u8 *)&ply.tpage);
    DrawSync(0);
    VSync(0);
    time--;
    goto loop;
end:
    PutDrawEnv(&o_draw);
}

void draw_shade_quad_(void *ot, s8 r, s8 g, s8 b)
{
    POLY_XF4 *ply;

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 6);
    setPolyF4(&ply->ply);
    setSemiTrans(&ply->ply, 1);
    setlen(&ply->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply->ply.x0 = -SCREEN_W / 2;
    ply->tpage.code[0] =
        GPU_DRAWMODE_BLEND(GPU_BLEND_SUBTRACT) | GPU_DRAWMODE_DITHER;
    ply->ply.y0 = -SCREEN_H / 2;
    ply->ply.y1 = -SCREEN_H / 2;
    ply->ply.x1 = SCREEN_W / 2;
    ply->ply.x2 = -SCREEN_W / 2;
    ply->ply.y2 = SCREEN_H / 2;
    ply->ply.x3 = SCREEN_W / 2;
    ply->ply.y3 = SCREEN_H / 2;
    ply->ply.r0 = r;
    ply->ply.g0 = g;
    ply->ply.b0 = b;
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}

void clear_screen_(void)
{
    ClearImage(&ScreenRect, 0, 0, 0);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawXG4(struct POLY_XG4 *ply);
 *     EFFECT.C:1808, 3 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct POLY_XG4 * ply
 * END PSX.SYM */

void DrawXG4(POLY_XG4 *ply)
{
    DrawPrim(&ply->ply);
    DrawPrim(&ply->tpage);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddXG4(void *ot, struct POLY_XG4 *ply);
 *     EFFECT.C:1803, 3 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * ot
 *     param $a1       struct POLY_XG4 * ply
 * END PSX.SYM */

void AddXG4(void *ot, POLY_XG4 *ply)
{
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetPolyXG4(struct POLY_XG4 *ply, short attrib);
 *     EFFECT.C:1793, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct POLY_XG4 * ply
 *     param $a1       short attrib
 * END PSX.SYM */

void SetPolyXG4(POLY_XG4 *ply, short attrib)
{
    setPolyG4(&ply->ply);
    setSemiTrans(&ply->ply, 1);
    setlen(&ply->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply->tpage.code[0] = GPU_DRAWMODE_BLEND(attrib) | GPU_DRAWMODE_DITHER;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawXF4(struct POLY_XF4 *ply);
 *     EFFECT.C:1785, 3 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct POLY_XF4 * ply
 * END PSX.SYM */

void DrawXF4(POLY_XF4 *ply)
{
    DrawPrim(&ply->ply);
    DrawPrim(&ply->tpage);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddXF4(void *ot, struct POLY_XF4 *ply);
 *     EFFECT.C:1780, 3 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * ot
 *     param $a1       struct POLY_XF4 * ply
 * END PSX.SYM */

void AddXF4(void *ot, POLY_XF4 *ply)
{
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetPolyXF4(struct POLY_XF4 *ply, short attrib);
 *     EFFECT.C:1770, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct POLY_XF4 * ply
 *     param $a1       short attrib
 * END PSX.SYM */

void SetPolyXF4(POLY_XF4 *ply, short attrib)
{
    setPolyF4(&ply->ply);
    setSemiTrans(&ply->ply, 1);
    setlen(&ply->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply->tpage.code[0] = GPU_DRAWMODE_BLEND(attrib) | GPU_DRAWMODE_DITHER;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeAfterimage(struct AfterimageType *afi);
 *     EFFECT.C:1706, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct AfterimageType * afi
 * END PSX.SYM */

void DisposeAfterimage(AfterimageType *afi)
{
    if (afi != 0)
    {
        vfree(afi->p1);
        vfree(afi->p2);
        vfree(afi);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct AfterimageType * SetupAfterimage(struct ModelType *model, short len);
 *     EFFECT.C:1667, 35 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct ModelType * model
 *     param $a1       short len
 *     reg   $s2       struct GsIMAGE * image
 *     reg   $s1       struct AfterimageType * afi
 *     reg   $a1       short px
 *     reg   $a2       short py
 *     reg   $v1       short ph
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsIMAGE *AfterIMG;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

AfterimageType *SetupAfterimage(ModelType *model, short len)
{
    GsIMAGE *image;
    AfterimageType *afi;
    GpuScreenPosition *points;
    s32 size;

    image = AfterIMG;
    afi = (AfterimageType *)valloc(sizeof(AfterimageType));
    size = len * sizeof(*points);
    afi->model = model;
    afi->vector1 = UnitVector;
    afi->vector2 = UnitVector;
    afi->n = 0;
    afi->maxn = len;
    points = (GpuScreenPosition *)valloc(size);
    afi->p1 = points;
    points = (GpuScreenPosition *)valloc(size);
    afi->p2 = points;
    afi->sz = 0;
    SetupImageToPolyGT4(image, &afi->poly.packet, 0, 0);
    SetSemiTrans(&afi->poly.packet, 1);
    return afi;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetLightning(struct VECTOR *start, struct VECTOR *end, short r, short g, int b);
 *     EFFECT.C:1562, 3 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * start
 *     param $a1       struct VECTOR * end
 *     param $a2       short r
 *     param $a3       short g
 *     param stack+16  int b
 * END PSX.SYM */

void SetLightning(VECTOR *start, VECTOR *end, short r, short g, short b)
{
    SetLightningI(start, end, 1, r, g, b);
}

void set_fade_(u8 r, u8 g, u8 b, long priority)
{
    long start_time;
    int idx;
    TEffectSlot *slot;
    int count;
    FadeType *fade;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->param.fade.r = r;
    fade = &slot->param.fade;
    fade->g = g;
    fade->b = b;
    fade->mode = FADE_MODE_IN;
    start_time = GameClock;
    fade->priority = priority;
    fade->start_time = start_time;
    fade->end_time = start_time + 5;
    slot->proc = draw_fade_;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetFrame(struct VECTOR *pos, short size, short time, struct _GsCOORDINATE2 *super);
 *     EFFECT.C:1095, 17 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       short size
 *     param $a2       short time
 *     param $a3       struct _GsCOORDINATE2 * super
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetFrame(VECTOR *pos, short size, short time, GsCOORDINATE2 *super)
{
    long z;
    int idx;
    TEffectSlot *slot;
    int count;
    FrameType *fp;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    fp = &slot->param.frame;
    fp->px = pos->vx;
    fp->py = pos->vy;
    z = pos->vz;
    fp->mode = FRAME_MODE_FLASH;
    fp->size = size;
    fp->count = time;
    fp->pz = z;
    slot->param.frame.super = super;
    slot->proc = DrawFrame;
}

void SetSnow(VECTOR *pos, SVECTOR *velocity, s32 size, u8 sprite)
{
    int idx;
    int count;
    TEffectSlot *slot;
    SnowParticleType *particle;
    s16 vz;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->param.snow.x = pos->vx;
    particle = &slot->param.snow;
    particle->y = pos->vy;
    particle->z = pos->vz;
    particle->velocity[0] = velocity->vx;
    particle->velocity[1] = velocity->vy;
    vz = velocity->vz;
    particle->sprite = sprite;
    particle->size = size;
    particle->sample_y = particle->y - 8000;
    particle->velocity[2] = vz;
    particle->ground = GetAreaMapLevel(GlobalAreaMap, slot->param.snow.x,
                                       particle->sample_y, particle->z,
                                       AREA_LEVEL_FIRST_HIT);
    slot->proc = DrawSnow;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSplash(struct VECTOR *pos, short sx, short sy, int speed);
 *     EFFECT.C:1023, 17 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       short sx
 *     param $a2       short sy
 *     param $a3       int speed
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetSplash(VECTOR *pos, short sx, short sy, int speed)
{
    long z;
    int idx;
    TEffectSlot *slot;
    int count;
    SplashType *fp;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->param.splash.px = pos->vx;
    fp = &slot->param.splash;
    fp->py = pos->vy;
    z = pos->vz;
    fp->mode = SPLASH_MODE_SPAWN;
    fp->sx = sx;
    fp->sy = sy;
    fp->speed = speed;
    fp->pz = z;
    slot->proc = DrawSplash;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleed(struct VECTOR *pos, struct SVECTOR *vec, int time, long col);
 *     EFFECT.C:946, 14 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       struct SVECTOR * vec
 *     param $a2       int time
 *     param $a3       long col
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void SetBleed(VECTOR *pos, SVECTOR *vec, int time, long col)
{
    int idx;
    TEffectSlot *slot;
    int count;
    BleedType *param;
    u8 r;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    param = &slot->param.bleed;
    r = col >> 16;
    slot->param.bleed.pos = *pos;
    slot->param.bleed.vec = *vec;
    param->r = r;
    param->g = col >> 8;
    param->time = time;
    param->b = col;
    param->mode = 0;
    slot->proc = DrawBleed;
}

void set_impact_ex_(VECTOR *pos, GsCOORDINATE2 *super,
                    short start_size, short end_size,
                    long start_color, long end_color,
                    s32 rotate, s32 rotate_speed, s32 time,
                    enum impact_sprite type)
{
    int idx;
    TEffectSlot *slot;
    int count;
    ImpactType *param;
    long pz;
    u16 stored_rotation = rotate;
    u16 stored_rotate_speed = rotate_speed;
    u16 stored_time = time;
    u16 stored_type = type;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->proc = DrawImpact;
    slot->param.impact.px = pos->vx;
    param = &slot->param.impact;
    param->py = pos->vy;
    pz = pos->vz;
    param->super = super;
    param->rotate = stored_rotation;
    param->rotate_speed = stored_rotate_speed;
    param->start_color.word = start_color;
    param->end_color.word = end_color;
    param->start_size = start_size;
    param->end_size = end_size;
    param->time = stored_time;
    param->count = 0;
    param->type = stored_type;
    param->pz = pz;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTarget(long x, long y, long z, long color);
 *     EFFECT.C:602, 6 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long color
 *     stack sp+16     struct SVECTOR scr
 * END PSX.SYM */

void DrawTarget(s32 x, s32 y, s32 z, s32 color)
{
    SVECTOR scr;
    SVECTOR *projected;

    *SCREEN_PROJECTION_TRANSLATION_X = 0;
    *SCREEN_PROJECTION_TRANSLATION_Y = 0;
    *SCREEN_PROJECTION_TRANSLATION_Z = 0;
    *SCREEN_PROJECTION_POINT_X = x - (s16)ViewInfo.vpx;
    *SCREEN_PROJECTION_POINT_Y = y - (s16)ViewInfo.vpy;
    *SCREEN_PROJECTION_POINT_Z = z - (s16)ViewInfo.vpz;
    SetTransMatrix(SCREEN_PROJECTION_MATRIX);
    SetRotMatrix(&GsWSMATRIX);
    projected = &scr;
    projected->vz = RotTransPers(
        SCREEN_PROJECTION_POINT, (s32 *)projected,
        SCREEN_PROJECTION_PERSPECTIVE, SCREEN_PROJECTION_FLAG);
    DrawTargetS(scr.vx, scr.vy, scr.vz - 5, color);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetScreenPositionS(long x, long y, long z, struct SVECTOR *scr);
 *     EFFECT.C:588, 11 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       struct SVECTOR * scr
 * END PSX.SYM */

void GetScreenPositionS(s32 x, s32 y, s32 z, SVECTOR *scr)
{
    SVECTOR *point = (SVECTOR *)TENCHU_SCRATCHPAD(0x80);

    point->vx = x - (short)ViewInfo.vpx;
    point->vy = y - (short)ViewInfo.vpy;
    point->vz = z - (short)ViewInfo.vpz;
    scr->vz = RotTransPers(
        point, (s32 *)scr, (s32 *)TENCHU_SCRATCHPAD_ADDRESS,
        (s32 *)TENCHU_SCRATCHPAD(0x10));
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PrepareGetScreenPositionS(void);
 *     EFFECT.C:577, 9 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

void PrepareGetScreenPositionS(void)
{
    MATRIX *m = (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS;

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetScreenPosition(long x, long y, long z, struct SVECTOR *scr);
 *     EFFECT.C:543, 32 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       struct SVECTOR * scr
 * END PSX.SYM */

void GetScreenPosition(long x, long y, long z, SVECTOR *scr)
{
    MATRIX *m = SCREEN_PROJECTION_MATRIX;
    SVECTOR *sv = SCREEN_PROJECTION_POINT;

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    sv->vx = x - (short)ViewInfo.vpx;
    sv->vy = y - (short)ViewInfo.vpy;
    sv->vz = z - (short)ViewInfo.vpz;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
    scr->vz = RotTransPers(sv, (s32 *)scr, SCREEN_PROJECTION_PERSPECTIVE,
                           SCREEN_PROJECTION_FLAG);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetVectorRotation(struct VECTOR *start, struct VECTOR *end, int *rx, int *ry);
 *     EFFECT.C:532, 7 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * start
 *     param $a1       struct VECTOR * end
 *     param $a2       int * rx
 *     param $a3       int * ry
 * END PSX.SYM */

void GetVectorRotation(VECTOR *start, VECTOR *end, int *rx, int *ry)
{
    s32 dx;
    s32 dy;
    s32 dz;

    dx = end->vx - start->vx;
    dz = end->vz - start->vz;
    dy = end->vy - start->vy;
    *ry = ratan2(-dx, -dz);
    *rx = ratan2(dy, SquareRoot0((dx * dx) + (dz * dz)));
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int GetVectorDistance(struct VECTOR *v1, struct VECTOR *v2);
 *     EFFECT.C:509, 19 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * v1
 *     param $a1       struct VECTOR * v2
 * END PSX.SYM */

int GetVectorDistance(VECTOR *v1, VECTOR *v2)
{
    enum
    {
        div = 256
    };
    long dx, dy, dz;
    long len;
    int big;
    long v;

    dx = v1->vx - v2->vx;
    dy = v1->vy - v2->vy;
    dz = v1->vz - v2->vz;

    big = 0;
    if (abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
        abs(dz) > FIXED_ONE)
    {
        big = 1;
    }
    if (big)
    {
        v = dx;
        if (dx < 0)
            v = dx + div - 1;
        dx = v >> 8;
        v = dy;
        if (dy < 0)
            v = dy + div - 1;
        dy = v >> 8;
        v = dz;
        if (dz < 0)
            v = dz + div - 1;
        dz = v >> 8;
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
        len = len << 8;
    }
    else
    {
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }
    return len;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetVectorLength(long dx, long dy, long dz);
 *     EFFECT.C:493, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long dx
 *     param $a1       long dy
 *     param $a2       long dz
 * END PSX.SYM */

long GetVectorLength(long dx, long dy, long dz)
{
    enum
    {
        div = 256
    };
    long len;
    int big;
    long v;

    big = 0;
    if (abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
        abs(dz) > FIXED_ONE)
    {
        big = 1;
    }
    if (big)
    {
        v = dx;
        if (dx < 0)
            v = dx + div - 1;
        dx = v >> 8;
        v = dy;
        if (dy < 0)
            v = dy + div - 1;
        dy = v >> 8;
        v = dz;
        if (dz < 0)
            v = dz + div - 1;
        dz = v >> 8;
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
        len = len * div;
    }
    else
    {
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }
    return len;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RotateVectorS(struct SVECTOR *vec, int rx, int ry, int rz);
 *     EFFECT.C:480, 9 src lines, frame 88 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct SVECTOR * vec
 *     param $a1       int rx
 *     param $a2       int ry
 *     param $a3       int rz
 *     stack sp+16     struct MATRIX SMAT
 *     stack sp+48     struct SVECTOR rot
 *     stack sp+56     struct SVECTOR vo
 * END PSX.SYM */

void RotateVectorS(SVECTOR *vec, int rx, int ry, int rz)
{
    MATRIX SMAT;
    SVECTOR rot = {
        .vx = (short)rx,
        .vy = (short)ry,
        .vz = (short)rz
    };
    SVECTOR vo;

    RotMatrixYXZ(&rot, &SMAT);
    ApplyMatrixSV(&SMAT, vec, &vo);
    setVector(vec, vo.vx, vo.vy, vo.vz);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void RotateVector(struct VECTOR *vec, int rx, int ry, int rz);
 *     EFFECT.C:470, 9 src lines, frame 96 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * vec
 *     param $a1       int rx
 *     param $a2       int ry
 *     param $a3       int rz
 *     stack sp+16     struct MATRIX SMAT
 *     stack sp+48     struct SVECTOR rot
 *     stack sp+56     struct VECTOR vo
 * END PSX.SYM */

void RotateVector(VECTOR *vec, int rx, int ry, int rz)
{
    MATRIX SMAT;
    SVECTOR rot = {
        .vx = (short)rx,
        .vy = (short)ry,
        .vz = (short)rz
    };
    VECTOR vo;

    RotMatrixYXZ(&rot, &SMAT);
    ApplyMatrixLV(&SMAT, vec, &vo);
    setVector(vec, vo.vx, vo.vy, vo.vz);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawEffect(void);
 *     EFFECT.C:366, 73 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

void DrawEffect(void)
{
    TEffectSlot *p;
    s32 i;

    for (i = 0; i < N_EFFECT_SLOTS; i++)
    {
        p = &EffectSlot[i];
        if (p->proc != 0)
        {
            p->proc(p);
        }
    }
}

void reset_effects_(void)
{
    s32 i;

    for (i = 0; i < N_EFFECT_SLOTS; i++)
    {
        if (EffectSlot[i].proc != UpdateTexScroll)
        {
            EffectSlot[i].proc = 0;
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long CGetLevel(struct AreaNodeType **hint, long x, long y, long z, unsigned long flag);
 *     EFFECT.C:220, 34 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct AreaNodeType ** hint
 *     param $a1       long x
 *     param $a2       long y
 *     param $a3       long z
 *     param stack+16  unsigned long flag
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/* Retail narrows flag to s16 at the map query despite the public u_long
 * parameter. */

long CGetLevel(AreaNodeType **hint, long x, long y, long z, unsigned long flag)
{
    long x10;
    long y10;
    long z10;
    AreaNodeType *node;
    long ret;

    x10 = x / 10;
    y10 = y / 10;
    node = *hint;
    z10 = z / 10;
    if (node == 0 || node->y - 200 > y10 || y10 > node->y || x10 < node->x1 ||
        z10 < node->z1 || node->x2 < x10 || node->z2 < z10)
    {
        ret = GetAreaMapLevel(GlobalAreaMap, x, y - 300, z, (short)flag);
        if (y <= ret && FieldArea->division == AREA_DIVISION_ALL)
        {
            *hint = FieldArea;
        }
    }
    else if (node->dy != 0)
    {
        ret = ComputeAreaLevel(node, x10, z10);
        if (ret != (u32)LEVEL_NONE)
        {
            ret *= 10;
        }
    }
    else
    {
        ret = node->y * 10;
    }
    return ret;
}

s32 trace_ground_(VECTOR *from, VECTOR *to, VECTOR *out, u32 flag)
{
    AreaNodeType *hint;
    s32 x, y, z;
    s32 dx, dy, dz;
    s32 step;
    s32 t;
    s32 lx, ly, lz;
    s32 tx, ty, tz;
    s32 rawx, rawy, rawz;

    x = from->vx;
    dx = to->vx - x;
    y = from->vy;
    dy = to->vy - y;
    z = from->vz;
    dz = to->vz - z;
    step = (500 << FIXED_SHIFT) / GetVectorLength(dx, dy, dz);
    lx = x;
    ly = y;
    lz = z;
    hint = 0;
    t = step;
    while (1)
    {
        if (t >= FIXED_ONE)
            break;
        /* The bias preserves signed division's truncation toward zero. */
        rawx = dx * t;
        if (rawx < 0)
            rawx += FIXED_TRUNC_BIAS;
        rawy = dy * t;
        tx = x + (rawx >> FIXED_SHIFT);
        if (rawy < 0)
            rawy += FIXED_TRUNC_BIAS;
        rawz = dz * t;
        ty = y + (rawy >> FIXED_SHIFT);
        if (rawz < 0)
            rawz += FIXED_TRUNC_BIAS;
        tz = z + (rawz >> FIXED_SHIFT);
        if (CGetLevel(&hint, tx, ty, tz, flag) < ty)
            break;
        lx = tx;
        ly = ty;
        lz = tz;
        t += step;
    }
    if (out != 0)
    {
        setVector(out, lx, ly, lz);
    }
    return t;
}

void draw_sprite_pair_(GsSPRITE *sp1, GsSPRITE *sp2, s32 x, s32 y, s32 z, s32 size, long rotate, s32 color)
{
    SVECTOR out;
    s32 otz;
    s16 sx;
    s16 sy;
    s32 t;
    s32 pri;

    GetScreenPosition(x, y, z, &out);
    otz = out.vz;
    if (otz > NEAR_DEPTH)
    {
        sp1->scalex = sp1->scaley = sp2->scalex = sp2->scaley =
            (s16)((size * PROJECTION_DISTANCE) / otz) + 1;
        sp2->rotate = rotate;
        sp1->rotate = rotate;
        sx = out.vx;
        sp2->x = sx;
        sp1->x = sx;
        sy = out.vy;
        sp2->y = sy;
        sp1->y = sy;
        sp2->b = sp2->g = sp2->r = (u8)color;
        sp1->b = sp1->g = sp1->r = (u8)(color / 2);

        t = (s16)(u16)out.vz >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sp2, OTablePt, (u16)pri);

        t = (s16)(u16)out.vz >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sp1, OTablePt, (u16)pri);
    }
}

void draw_sprite_coord_(GsSPRITE *sp, s32 x, s32 y, s32 z, s32 size, GsCOORDINATE2 *coord, short zbias)
{
    SVECTOR scr;
    s32 otz;
    s32 t;
    s32 pri;

    if (coord != 0)
    {
        SVECTOR *sv = SCREEN_PROJECTION_POINT;
        setVector(sv, x, y, z);
        GsGetLs(coord, SCREEN_PROJECTION_MATRIX);
        GsSetLsMatrix(SCREEN_PROJECTION_MATRIX);
        scr.vz = (s16)RotTransPers(
            sv, (s32 *)&scr, SCREEN_PROJECTION_PERSPECTIVE,
            SCREEN_PROJECTION_FLAG);
    }
    else
    {
        GetScreenPosition(x, y, z, &scr);
    }
    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        sp->scalex = sp->scaley =
            (s16)((size * PROJECTION_DISTANCE) / otz) + 1;
        sp->x = scr.vx;
        sp->y = scr.vy;
        t = (scr.vz + (s32)zbias) >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sp, OTablePt, (u16)pri);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawSpriteXYZ(struct GsSPRITE *sprt, long x, long y, long z, long scale);
 *     EFFECT.C:204, 11 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsSPRITE * sprt
 *     param $a1       long x
 *     param $a2       long y
 *     param $a3       long z
 *     param stack+16  long scale
 *     stack sp+16     struct SVECTOR scr
 * END PSX.SYM */

void DrawSpriteXYZ(GsSPRITE *sprt, s32 x, s32 y, s32 z, s32 scale)
{
    SVECTOR scr;
    s32 otz;
    s32 t;
    s32 pri;

    GetScreenPosition(x, y, z, &scr);
    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        sprt->scalex = sprt->scaley =
            (s16)((scale * PROJECTION_DISTANCE) / otz) + 1;
        sprt->x = scr.vx;
        sprt->y = scr.vy;
        t = (s16)(u16)scr.vz >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sprt, OTablePt, (u16)pri);
    }
}
