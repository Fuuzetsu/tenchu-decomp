#include "common.h"
#include "tuning.h"
#include "../main.exe/main.exe.h"
#include "../main.exe/effect.h"
#include "../main.exe/afterimage.h"
#include "../main.exe/images.h"
#include "../main.exe/item.h"
#include "../main.exe/misc.h"
#include "../main.exe/model.h"
#include "../main.exe/sound.h"
#include "../main.exe/tmdfast.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libgpu.h>

/*
 * TRIAL keeps the shared effects with a 100-slot pool. Snow, explosion,
 * spark, lightning, queued-fade, and texture-scroll entry points are empty.
 * The direct frame-by-frame fade (FadeOutDirect) is still implemented.
 */

extern ModelType *BLOOD_POOL_MODEL_;
extern s16 TexScrollX;
extern s16 TexScrollY;
extern RECT ScreenRect; /* {0,0,320,480}: both pages */

enum
{
    BLOOD_SPREAD_SCALE_MIN = FIXED_ONE / 3,
    BLOOD_SPREAD_SCALE_RANGE = FIXED_ONE / 2 - BLOOD_SPREAD_SCALE_MIN
};

void reset_effects_(void);

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

    for (i = 0; i < N_BLOOD_SPRITES; i++)
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

        for (i = 0; i < N_SNOW_SPRITES; i++)
        {
            image = GetImage(IMG_MISC_SNOW);
            sprite = SetupSprite((Sprite3D *)0, image);
            SpriteSnow[i] = sprite;
            sprite->sprite.attribute = GS_ATTR_SEMITRANS_ADD;
        }
    }

    TexScrollX = TEXSCROLL_VRAM_ORIGIN_X;
    TexScrollY = TEXSCROLL_VRAM_ORIGIN_Y;
    reset_effects_();
}

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


enum
{
    BLOOD_BLEED_JITTER_RADIUS = 80
};

static __inline__ s32 JitterBloodBleedCoordinate(const s32 *coordinate)
{
    s32 random;
    s32 base;

    random = rand();
    base = *coordinate - BLOOD_BLEED_JITTER_RADIUS;
    return base + random % (BLOOD_BLEED_JITTER_RADIUS * 2);
}

void DrawBlood(TEffectSlot *ef)
{
    BloodType *blood;
    GsSPRITE *spr;
    GsSPRITE *sprt;
    SVECTOR scr;
    VECTOR pos;
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
        u16 previous_time;

        previous_time = blood->time--;
        if ((s16)previous_time <= 0)
        {
            blood->time = 0x80;
            blood->mode++;
        }
        break;
    }

    case BLOOD_MODE_SPREAD:
    {
        u16 previous_time;

        blood->scale += rand() % FIXED_ONE;
        previous_time = blood->time--;
        if ((s16)previous_time <= 0)
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
        u16 previous_time;
        s32 scale_random;

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
                blood->sprite += N_AIRBORNE_BLOOD_SPRITES;
                blood->scale = scale_random % BLOOD_SPREAD_SCALE_RANGE +
                               BLOOD_SPREAD_SCALE_MIN;
            }
            blood->mode = BLOOD_MODE_SPREAD;
            blood->time = rand() % 10;
            SoundEx((VECTOR *)&blood->px, SE_BLOOD_SPLATTER);
        }
        else
        {
            previous_time = blood->time--;
            if ((s16)previous_time <= 0)
            {
                ef->proc = 0;
            }
        }

        if (GameClock & 1)
        {
            pos = (VECTOR){
                .vx = JitterBloodBleedCoordinate(&blood->px),
                .vy = JitterBloodBleedCoordinate(&blood->py),
                .vz = JitterBloodBleedCoordinate(&blood->pz)
            };
            scr = (SVECTOR){
                .vx = blood->vx / 2,
                .vy = blood->vy / 2,
                .vz = blood->vz / 2
            };
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

/* Retail dropped the demo build's SVECTOR * argument; every retail caller
 * and the callee use the three-argument form below. */

void SetBlood(VECTOR *pos, short n, short time)
{
    TEffectSlot *slot;
    BloodType *blood;
    struct AreaNodeType *hint;
    short i;
    int half;
    int half2;

    GetAreaMapLevel(GlobalAreaMap, pos->vx, pos->vy, pos->vz,
                    AREA_LEVEL_DEFAULT);
    hint = FieldArea;
    for (i = 0; ; i++)
    {
        if (i >= n)
        {
            return;
        }
        slot = GetFreeEffectSlot();
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
        blood->brightness = 0x80;
        blood->hint = hint;
        blood->mode = BLOOD_MODE_AIRBORNE;
        slot->proc = DrawBlood;
    }
}

static void DrawSmoke(TEffectSlot *ef)
{
    enum
    {
        SMOKE_ALPHA_MAX = 0x80,
        SMOKE_FAST_RISE_LIMIT = -20,
        SMOKE_HORIZONTAL_SPEED_PERCENT = 80,
        PERCENT_SCALE = 100,
        SMOKE_VERTICAL_SPEED_DIVISOR = 2,
        SMOKE_SCALE_STEP = FIXED_ONE / 4,
        SMOKE_EVENT_JITTER = 5,
        SMOKE_FADE_START = 26,
        SMOKE_FADE_STEP = 5
    };
    SmokeType *param = &ef->param.smoke;
    Sprite3D *spr;
    u8 alfa;
    u8 oldtime;
    s32 r;
    s32 m;
    s32 rotate;

    spr = sprSmoke[param->sprite];
    alfa = SMOKE_ALPHA_MAX;

    if (param->time == param->evtime)
    {
        if (param->vec.vy < SMOKE_FAST_RISE_LIMIT)
        {
            param->vec.vx =
                (param->vec.vx * SMOKE_HORIZONTAL_SPEED_PERCENT) /
                PERCENT_SCALE;
            param->vec.vy /= SMOKE_VERTICAL_SPEED_DIVISOR;
            param->vec.vz =
                (param->vec.vz * SMOKE_HORIZONTAL_SPEED_PERCENT) /
                PERCENT_SCALE;
        }
        param->scale += SMOKE_SCALE_STEP;
        r = rand();
        m = param->time - 1;
        param->evtime = m - r % SMOKE_EVENT_JITTER;
    }

    if (param->time < SMOKE_FADE_START)
    {
        alfa = param->time * SMOKE_FADE_STEP;
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

    oldtime = param->time--;
    if (oldtime == 0)
    {
        ef->proc = 0;
    }
}

void SetSmoke(VECTOR *pos, SVECTOR *vect, short n, short time)
{
    short i;
    TEffectSlot *slot;
    SmokeType *smoke;
    int r;
    int m;

    for (i = 0; ; i++)
    {
        if (i >= n)
        {
            return;
        }
        slot = GetFreeEffectSlot();
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
        smoke->sprite = SMOKE_SPRITE_NORMAL;
        m = smoke->time - 1;
        smoke->evtime = m - (time / 2 + r % time);
        slot->proc = DrawSmoke;
    }
}

/* Retail narrows the demo build's int time parameter to u16. */

void SetSmokeS(VECTOR *pos, short vx, short vy, short vz, unsigned short time)
{
    TEffectSlot *slot;
    SmokeType *smoke;
    int r;
    int m;

    slot = GetFreeEffectSlot();
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
    enum
    {
        SMOKE_BURST_RISE_SPEED = -5,
        SMOKE_BURST_DURATION = 15,
        SMOKE_BURST_EVENT_LEAD = 8,
        SMOKE_BURST_EVENT_JITTER = 15,
    };
    int i;
    TEffectSlot *slot;
    SmokeType *smoke;
    short vx;
    short vy;
    short vz;
    u32 r;
    int m;

    for (i = 0; ; i++)
    {
        if (i >= count)
        {
            return;
        }
        slot = GetFreeEffectSlot();
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
        smoke->vec.vy = SMOKE_BURST_RISE_SPEED;
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

        vx = smoke->vec.vx / divisor;
        vy = smoke->vec.vy / divisor;
        vz = smoke->vec.vz / divisor;
        copyVector(&smoke->pos, pos);
        smoke->pos.vx += smoke->vec.vx;
        smoke->pos.vy += smoke->vec.vy;
        smoke->pos.vz += smoke->vec.vz;
        smoke->vec.vx = vx;
        smoke->vec.vy = vy;
        smoke->vec.vz = vz;

        smoke->scale = rand() % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;
        smoke->rotate = 0;
        smoke->time = SMOKE_BURST_DURATION;
        r = rand();
        m = smoke->time - SMOKE_BURST_EVENT_LEAD;
        smoke->sprite = SMOKE_SPRITE_ALT;
        smoke->evtime = m - ((s32)r % SMOKE_BURST_EVENT_JITTER);
        slot->proc = DrawSmoke;
    }
}

static void DrawImpact(TEffectSlot *ef)
{
    ImpactType *param;
    GsSPRITE *spr;
    SVECTOR scr;
    s32 ratio;
    s32 inverse;
    s32 start;
    s32 start2;
    s32 value;
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
    value = start2 + (end_raw * ratio) / FIXED_ONE;
    spr->b = value;

    value = param->px;
    start2 = param->py;
    work = (s32)param->super;
    inverse = param->pz;
    if (work != 0)
    {
        *SCREEN_PROJECTION_POINT_X = value;
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
        GetScreenPosition(value, start2, inverse, &scr);
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

void SetImpact(VECTOR *pos, short size, short type)
{
    short spd;
    long start_color;
    long end_color;
    TEffectSlot *slot;
    ImpactType *param;
    long pz;

    spd = rand() % 90 + 90;
    start_color = COLOR_GRAY;
    end_color = COLOR_GRAY;
    slot = GetFreeEffectSlot();
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
    for (; ; n--)
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
                TEffectSlot *slot;
                BleedType *param;
                u8 r;

                slot = GetFreeEffectSlot();
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
    }
}

static void DrawSplash(TEffectSlot *ef)
{
    enum
    {
        SPLASH_DROPLET_SPEED_Y = -20,
        SPLASH_DROPLET_SPREAD = 100,
        SPLASH_DROPLET_COUNT = 6,
        SPLASH_DROPLET_LIFETIME = 30,
        SPLASH_DROPLET_COLOR = RGB24(144, 152, 160)
    };
    SplashType *param = &ef->param.splash;
    GsSPRITE *spr = &sprSplash;
    SVECTOR scr;
    SVECTOR *projected;
    long x;
    long y;
    long z;
    s32 priority;

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
        s32 depth;

        depth = scr.vz;
        if (depth > NEAR_DEPTH)
        {
            spr->x = scr.vx;
            spr->y = scr.vy;
            spr->scalex =
                (param->sx * PROJECTION_DISTANCE) / depth + 1;
            spr->scaley =
                (param->sy * PROJECTION_DISTANCE) / depth + 1;

            switch (param->mode)
            {
            case SPLASH_MODE_SPAWN:
                param->count = 0;
                param->mode++;
                {
                    VECTOR pos = {
                        .vx = param->px,
                        .vy = param->py,
                        .vz = param->pz
                    };
                    SVECTOR direction = {
                        .vx = 0,
                        .vy = SPLASH_DROPLET_SPEED_Y,
                        .vz = 0
                    };

                    SetBleedsDir(
                        &pos, &direction, SPLASH_DROPLET_SPREAD,
                        SPLASH_DROPLET_COUNT, SPLASH_DROPLET_LIFETIME,
                        SPLASH_DROPLET_COLOR);
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
                s32 ordering_depth;

                ordering_depth = (s16)(u16)scr.vz >> 2;
                if (ordering_depth >= 0)
                {
                    priority = DEPTH_LIMIT - 1;
                    if (ordering_depth < DEPTH_LIMIT)
                    {
                        priority = ordering_depth;
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


static void DrawFrame(TEffectSlot *ef)
{
    enum
    {
        FRAME_FLASH_LEVEL = 0x80,
        FRAME_SORT_DEPTH_BIAS = 50
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
        t = scr.vz - FRAME_SORT_DEPTH_BIAS;
        t = t >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(spr, OTablePt, (u16)pri);
    }
}

void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n, int time, long col)
{
    int btime;

    for (; ; n--)
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
                TEffectSlot *slot;
                BleedType *param;
                u8 r;

                slot = GetFreeEffectSlot();
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
    }
}

enum
{
    GORE_BLEED_JITTER_RADIUS = 60,
    GORE_COLOR = RGB24(127, 16, 23)
};

static __inline__ s32 JitterGoreBleedCoordinate(const s32 *coordinate)
{
    s32 random;
    s32 base;

    random = rand();
    base = *coordinate - GORE_BLEED_JITTER_RADIUS;
    return base + random % (GORE_BLEED_JITTER_RADIUS * 2);
}

static void DrawGore(TEffectSlot *ef)
{
    BloodType *param;
    GsSPRITE *spr;
    GsSPRITE *spr2;
    SVECTOR vector;
    VECTOR position;

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
        u16 previous_time;

        previous_time = param->time--;
        if ((s16)previous_time <= 0)
        {
            param->time = 0x80;
            param->mode++;
        }
        break;
    }

    case GORE_MODE_SPREAD:
    {
        u16 previous_time;

        param->scale += rand() % FIXED_ONE;
        previous_time = param->time--;
        if ((s16)previous_time <= 0)
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
        SVECTOR *velocity;
        long color;
        long green;
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
                param->scale = scale_random % BLOOD_SPREAD_SCALE_RANGE +
                               BLOOD_SPREAD_SCALE_MIN;
            }
            param->mode = GORE_MODE_SPREAD;
            param->time = rand() % 10;
            SoundEx((VECTOR *)&param->px, SE_BLOOD_SPLATTER);
        }
        else
        {
            u16 previous_time;

            previous_time = param->time--;
            if ((s16)previous_time <= 0)
            {
                ef->proc = 0;
            }
        }

        position = (VECTOR){
            .vx = JitterGoreBleedCoordinate(&param->px),
            .vy = JitterGoreBleedCoordinate(&param->py),
            .vz = JitterGoreBleedCoordinate(&param->pz)
        };
        vector = (SVECTOR){
            .vx = param->vx / 2,
            .vy = param->vy / 2,
            .vz = param->vz / 2
        };
        velocity = &vector;
        color = GORE_COLOR;

        slot = GetFreeEffectSlot();
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
    long transform_flag;
    u32 impact_phase;

    GsGetLw(coord, &local_to_world);
    GsSetLsMatrix(&local_to_world);
    RotTrans(local_position, &world_position, &transform_flag);

    {
        TEffectSlot *gore_slot;
        BloodType *gore;

        gore_slot = GetFreeEffectSlot();
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
        TEffectSlot *impact_slot;
        ImpactType *impact;
        long impact_pz;
        long start_color;
        long end_color;

        start_color = COLOR_GRAY;
        end_color = COLOR_GRAY;
        impact_position.vx = local_position->vx;
        impact_position.vy = local_position->vy;
        impact_position.vz = local_position->vz;
        impact_slot = GetFreeEffectSlot();
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


/* Originally static in EFFECT.C; global here because SetHinoko is split into
 * a separate translation unit and stores this function's address. */


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

int SetFlyWire(VECTOR *start, VECTOR *end)
{
    enum
    {
        FLYWIRE_DISTANCE_RESCALE_SHIFT = 8,
        FLYWIRE_TRAVEL_SPEED = 1000,
        FLYWIRE_BEND_DIVISOR = 16,
        FLYWIRE_STRAIGHTEN_FRAMES = 5,
    };
    TEffectSlot *slot;
    FlyWireType *param;
    int dist;
    int result;

    slot = GetFreeEffectSlot();
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

        big = abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
              abs(dz) > FIXED_ONE;
        if (big)
        {
            dx /= 1 << FLYWIRE_DISTANCE_RESCALE_SHIFT;
            dy /= 1 << FLYWIRE_DISTANCE_RESCALE_SHIFT;
            dz /= 1 << FLYWIRE_DISTANCE_RESCALE_SHIFT;
            root = SquareRoot0(dx * dx + dy * dy + dz * dz)
                   << FLYWIRE_DISTANCE_RESCALE_SHIFT;
        }
        else
        {
            root = SquareRoot0(dx * dx + dy * dy + dz * dz);
        }
        dist = root;

        param->NCenter.vx = (param->start.vx + param->end.vx) / 2;
        param->NCenter.vy = (param->start.vy + param->end.vy) / 2;
        param->NCenter.vz = (param->start.vz + param->end.vz) / 2;
        param->time = dist / FLYWIRE_TRAVEL_SPEED;

        dist /= FLYWIRE_BEND_DIVISOR;

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
        result = param->time + FLYWIRE_STRAIGHTEN_FRAMES;
    }
    else
    {
        result = 0;
    }
    return result;
}

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
    enum
    {
        WIRE_COLOR = 0x504838,
        WIRE_DISTANCE_RESCALE_SHIFT = 8,
    };
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
    line.r = WIRE_COLOR >> 16;
    line.g = WIRE_COLOR >> 8;
    line.b = WIRE_COLOR;

    {
        VECTOR *v1;
        VECTOR *v2;
        long dx, dy, dz;
        int rescale_distance;

        v1 = start;
        v2 = end;
        dx = v1->vx - v2->vx;
        dy = v1->vy - v2->vy;
        dz = v1->vz - v2->vz;
        rescale_distance = abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
                           abs(dz) > FIXED_ONE;
        if (rescale_distance)
        {
            dx /= 1 << WIRE_DISTANCE_RESCALE_SHIFT;
            dy /= 1 << WIRE_DISTANCE_RESCALE_SHIFT;
            dz /= 1 << WIRE_DISTANCE_RESCALE_SHIFT;
            distance = SquareRoot0(dx * dx + dy * dy + dz * dz)
                       << WIRE_DISTANCE_RESCALE_SHIFT;
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
    for (i = 0; ; i++)
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


static inline void CreateFrameEffect(VECTOR *pos, short size, short time,
                                     GsCOORDINATE2 *super)
{
    long z;
    TEffectSlot *slot;
    FrameType *fp;

    slot = GetFreeEffectSlot();
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


static inline void SpawnNapalmDamage(Humanoid *human)
{
    PARAM_ITEM_LAUNCH launch;
    s32 vx;
    s32 vz;

    launch.type = ITEM_NAPALM;
    launch.user = human;
    copyVector(&launch.start, MODEL_POSITION(human->model));
    launch.start.vy -= 100;
    copyVector(&launch.end, &launch.start);
    vx = human->vector.vx;
    launch.end.vx += vx;
    vz = human->vector.vz;
    launch.end.vy -= 15;
    launch.end.vz += vz;
    ReqItemUse(&launch);
}


void spawn_damage_effect_(Humanoid *human, DamageEffectKind kind)
{
    if (kind != DAMAGE_EFFECT_ATTACHED_FLASH)
    {
        SpawnNapalmDamage(human);
    }
    else
    {
        ModelType **objects;
        ModelType *model;
        short time;

        objects = human->model->object;
        if (human->model->n > 0)
            objects += rand() % human->model->n;
        model = *objects;

        {
            VECTOR pos = {
                .vx = rand() % 200 - 100,
                .vy = rand() % 200 - 100,
                .vz = rand() % 200 - 100
            };
            SVECTOR velocity = {.vx = 0, .vy = -60, .vz = 0};

            time = rand() % 60 + 60;
            CreateFrameEffect(&pos, 3 * FIXED_ONE, time, &model->locate);

            SetBleedsDir(GetAbsolutePosition(model, 0, 0, 0), &velocity,
                         100, 10, 30, RGB24(100, 100, 60));
            SoundEx(MODEL_POSITION(human->model), SE_LIGHTNING);
        }
    }
}

void spread_blood_pool_(Humanoid *human)
{
    enum
    {
        DAMAGE_FLOOR_EFFECT_PERIOD = 16,
        BLOOD_POOL_GROWTH_STEP = 0x88,
        BLOOD_POOL_SCALE_DIVISOR = FIXED_ONE / 4,
        BLOOD_POOL_SLOPE_PITCH = ANGLE_SIXTEENTH
    };
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
        chase[0] < 0)
    {
        chase[0] = 0;
        return;
    }

    if ((human->map.attrib & MAP_DAMAGE) != 0)
    {
        if ((GameClock & (DAMAGE_FLOOR_EFFECT_PERIOD - 1)) == 0)
        {
            spawn_damage_effect_(human, DAMAGE_EFFECT_ATTACHED_FLASH);
        }
        return;
    }

    timer = chase[0] + BLOOD_POOL_GROWTH_STEP;
    chase[0] = timer;
    if (timer > FIXED_ONE)
    {
        chase[0] = FIXED_ONE;
    }

    position = GetAbsolutePosition(human->model->object[MODEL_PART_WAIST], 0, 0, 0);
    height = human->model->rotate.pad;
    position->vy = human->model->locate.coord.t[1];
    BLOOD_POOL_MODEL_->locate.coord.t[0] = position->vx;
    BLOOD_POOL_MODEL_->locate.coord.t[1] = position->vy;
    BLOOD_POOL_MODEL_->locate.coord.t[2] = position->vz;

    scaled = chase[0] * -height / BLOOD_POOL_SCALE_DIVISOR;
    scale.vx = scale.vy = scale.vz =
        scaled - (human->map.height >> 1);

    if (human->map.angleH != 0)
    {
        BLOOD_POOL_MODEL_->rotate.vx = BLOOD_POOL_SLOPE_PITCH;
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

void DrawShadow(Humanoid *human)
{
    enum
    {
        WATER_SPLASH_PERIOD = 2,
        WATER_SPLASH_HEIGHT = 100,
        SWIM_SPLASH_RADIUS = 300,
        LAND_SPLASH_RADIUS = 100,
        WATER_SPLASH_SIZE = 2 * FIXED_ONE,
        WATER_SPLASH_FRAMES = 4,
        DAMAGE_NAPALM_PERIOD = 64,
        DAMAGE_NAPALM_PHASE = 1,
        DAMAGE_BLEED_PERIOD = 16,
        SHADOW_HEIGHT_SCALE = 4,
        SHADOW_SLOPE_PITCH = ANGLE_SIXTEENTH,
        SHADOW_OT_OFFSET = 2
    };
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
            human->map.height == 0 &&
            (GameClock & (WATER_SPLASH_PERIOD - 1)) != 0)
        {
            TEffectSlot *slot;
            SplashType *param;
            s32 splash_z;

            if (human->status == STAT_SWIM)
            {
                s32 r = rand();

                position->vy += WATER_SPLASH_HEIGHT;
                position->vx +=
                    r % (SWIM_SPLASH_RADIUS * 2) - SWIM_SPLASH_RADIUS;
                position->vz += rand() % (SWIM_SPLASH_RADIUS * 2) -
                                SWIM_SPLASH_RADIUS;
            }
            else
            {
                position->vx += rand() % (LAND_SPLASH_RADIUS * 2) -
                                LAND_SPLASH_RADIUS;
                position->vz += rand() % (LAND_SPLASH_RADIUS * 2) -
                                LAND_SPLASH_RADIUS;
            }

            slot = GetFreeEffectSlot();
            param = &slot->param.splash;
            param->px = position->vx;
            param->py = position->vy;
            splash_z = position->vz;
            param->sx = WATER_SPLASH_SIZE;
            param->sy = WATER_SPLASH_SIZE;
            param->speed = WATER_SPLASH_FRAMES;
            param->mode = SPLASH_MODE_SPAWN;
            param->pz = splash_z;
            slot->proc = DrawSplash;
        }
    }
    else if (human->map.attrib & MAP_DAMAGE)
    {
        if (human->map.height == 0)
        {
            if ((GameClock & (DAMAGE_NAPALM_PERIOD - 1)) ==
                DAMAGE_NAPALM_PHASE)
            {
                spawn_damage_effect_(human, DAMAGE_EFFECT_NAPALM);
            }
            else if ((GameClock & (DAMAGE_BLEED_PERIOD - 1)) == 0)
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

        scl.vx = scl.vy = scl.vz =
            height * SHADOW_HEIGHT_SCALE - (human->map.height >> 1);
        if (human->map.angleH != 0)
        {
            ShadowMdl->rotate.vx = SHADOW_SLOPE_PITCH;
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
            GsSortObject4(&ShadowMdl->object, OTablePt, SHADOW_OT_OFFSET,
                          (u_long *)TENCHU_SCRATCHPAD_ADDRESS);
        }
    }
}

short DrawAfterimage(AfterimageType *afi, short disp)
{
    enum
    {
        AFTERIMAGE_BRIGHTNESS_MAX = 127
    };
    GpuPolyGT4Packet *poly;
    MATRIX mat;
    short i;
    s32 otz;
    s16 tplv;
    s32 pri;
    long tmp1, tmp2;

    tplv = AFTERIMAGE_BRIGHTNESS_MAX;
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

        tplv = ((afi->n - i) * AFTERIMAGE_BRIGHTNESS_MAX) / afi->n;
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
    if (time != 0)
    {
        DrawPrim((u8 *)&ply.ply);
        DrawPrim((u8 *)&ply.tpage);
        DrawSync(0);
        VSync(0);
        time--;
        goto loop;
    }
    PutDrawEnv(&o_draw);
}

void draw_shade_quad_(void *ot, s32 r, s32 g, s32 b)
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

void DrawXG4(POLY_XG4 *ply)
{
    DrawPrim(&ply->ply);
    DrawPrim(&ply->tpage);
}

void AddXG4(void *ot, POLY_XG4 *ply)
{
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}

void SetPolyXG4(POLY_XG4 *ply, short attrib)
{
    setPolyG4(&ply->ply);
    setSemiTrans(&ply->ply, 1);
    setlen(&ply->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply->tpage.code[0] = GPU_DRAWMODE_BLEND(attrib) | GPU_DRAWMODE_DITHER;
}

void DrawXF4(POLY_XF4 *ply)
{
    DrawPrim(&ply->ply);
    DrawPrim(&ply->tpage);
}

void AddXF4(void *ot, POLY_XF4 *ply)
{
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}

void SetPolyXF4(POLY_XF4 *ply, short attrib)
{
    setPolyF4(&ply->ply);
    setSemiTrans(&ply->ply, 1);
    setlen(&ply->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply->tpage.code[0] = GPU_DRAWMODE_BLEND(attrib) | GPU_DRAWMODE_DITHER;
}

void DisposeAfterimage(AfterimageType *afi)
{
    if (afi == 0)
    {
        return;
    }

    vfree(afi->p1);
    vfree(afi->p2);
    vfree(afi);
}

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


/* These public APIs are retained as empty functions in the original TRIAL. */
void SetLightning(VECTOR *start, VECTOR *end, short r, short g, short b)
{
}

void SetHinoko(VECTOR *pos, SVECTOR *power, int n)
{
}

void SetExplosion(VECTOR *pos, SVECTOR *vect)
{
}

void set_fade_(u8 r, u8 g, u8 b, long priority)
{
}

void SetFrame(VECTOR *pos, short size, short time, GsCOORDINATE2 *super)
{
    CreateFrameEffect(pos, size, time, super);
}

void SetSnow(VECTOR *pos, SVECTOR *velocity, s32 size, u8 sprite)
{
}

void SetSplash(VECTOR *pos, short sx, short sy, int speed)
{
    long z;
    TEffectSlot *slot;
    SplashType *fp;

    slot = GetFreeEffectSlot();
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

void SetBleed(VECTOR *pos, SVECTOR *vec, int time, long col)
{
    TEffectSlot *slot;
    BleedType *param;
    u8 r;

    slot = GetFreeEffectSlot();
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
    TEffectSlot *slot;
    ImpactType *param;
    long pz;
    u16 stored_rotation = rotate;
    u16 stored_rotate_speed = rotate_speed;
    u16 stored_time = time;
    u16 stored_type = type;

    slot = GetFreeEffectSlot();
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

void SetupTexScroll(GsIMAGE *img, short vx, short vy)
{
}

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

void PrepareGetScreenPositionS(void)
{
    MATRIX *m = (MATRIX *)TENCHU_SCRATCHPAD_ADDRESS;

    m->t[0] = 0;
    m->t[1] = 0;
    m->t[2] = 0;
    SetTransMatrix(m);
    SetRotMatrix(&GsWSMATRIX);
}

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

int GetVectorDistance(VECTOR *v1, VECTOR *v2)
{
    enum
    {
        DISTANCE_RESCALE = 256
    };
    long dx, dy, dz;
    long len;
    int rescale_distance;

    dx = v1->vx - v2->vx;
    dy = v1->vy - v2->vy;
    dz = v1->vz - v2->vz;

    rescale_distance = abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
                       abs(dz) > FIXED_ONE;
    if (rescale_distance)
    {
        dx /= DISTANCE_RESCALE;
        dy /= DISTANCE_RESCALE;
        dz /= DISTANCE_RESCALE;
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
        len *= DISTANCE_RESCALE;
    }
    else
    {
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }
    return len;
}

long GetVectorLength(long dx, long dy, long dz)
{
    enum
    {
        LENGTH_RESCALE = 256
    };
    long len;
    int rescale_length;

    rescale_length = abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
                     abs(dz) > FIXED_ONE;
    if (rescale_length)
    {
        dx /= LENGTH_RESCALE;
        dy /= LENGTH_RESCALE;
        dz /= LENGTH_RESCALE;
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
        len *= LENGTH_RESCALE;
    }
    else
    {
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }
    return len;
}

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
        rawx = dx * t;
        tx = x + rawx / FIXED_ONE;
        rawy = dy * t;
        ty = y + rawy / FIXED_ONE;
        rawz = dz * t;
        tz = z + rawz / FIXED_ONE;
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

void UpdateTexScroll(TEffectSlot *ef)
{
}

