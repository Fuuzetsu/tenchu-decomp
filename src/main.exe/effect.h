#ifndef EFFECT_H
#define EFFECT_H

/* EFFECT.C's effect-slot pool. PSX.SYM supplies the original effect records
 * and union members; retail redesigns and extends the impact record and adds
 * snow, screen-fade, and packed texture-scroll state. FlyWireType keeps the
 * union at its proven 72-byte size, making tag_EffectSlot's indexed pool
 * stride 76 bytes. */

struct AreaNodeType; /* opaque here: only ever stored, never dereferenced */

typedef struct BloodType BloodType;
typedef struct BleedType BleedType;
typedef struct SplashType SplashType;
typedef struct FrameType FrameType;
typedef struct ExplosionType ExplosionType;
typedef struct ExplosionType HinokoType;
typedef struct GoreType GoreType;
typedef struct FadeType FadeType;
typedef struct SmokeType SmokeType;
typedef struct ImpactType ImpactType;
typedef struct SnowParticleType SnowParticleType;
typedef struct TexScroll TexScroll;

typedef union ImpactColor ImpactColor;
union ImpactColor
{
    s32 word;
    struct
    {
        u8 b;
        u8 g;
        u8 r;
    } channel;
};

/* EFFECT.C's original animated-frame sprite count. */
enum
{
    MaxFrames = 4
};

/* Retail's redesigned and extended version of PSX.SYM's ImpactType. */
struct ImpactType /* size 36 */
{
    s32 px;                  /* +0x00 */
    s32 py;                  /* +0x04 */
    s32 pz;                  /* +0x08 */
    GsCOORDINATE2 *super;    /* +0x0C */
    ImpactColor start_color; /* +0x10 */
    ImpactColor end_color;   /* +0x14 */
    s16 start_size;          /* +0x18 */
    s16 end_size;            /* +0x1A */
    s16 rotate;              /* +0x1C */
    s16 rotate_speed;        /* +0x1E */
    u8 type;                 /* +0x20 */
    u8 count;                /* +0x21 */
    u8 time;                 /* +0x22 */
};

/* Retail-only snowfall particle, proven jointly by SetSnow and DrawSnow. */
struct SnowParticleType /* size 32 */
{
    s32 x;           /* +0x00 */
    s32 y;           /* +0x04 */
    s32 z;           /* +0x08 */
    s32 ground;      /* +0x0C */
    s32 sample_y;    /* +0x10 */
    s32 size;        /* +0x14 */
    s16 velocity[3]; /* +0x18 */
    u8 sprite;       /* +0x1E */
};

/* Retail embeds a shortened form of PSX.SYM's TexScroll in an EffectSlot.
 * The px/py accumulators and vx/vy deltas keep their recovered identities;
 * only the demo's time/count pair is absent. */
struct TexScroll /* size 24 */
{
    s16 px;     /* +0x00 */
    s16 py;     /* +0x02 */
    s16 vx;     /* +0x04 */
    s16 vy;     /* +0x06 */
    s16 x;      /* +0x08 */
    s16 y;      /* +0x0A */
    s16 sx;     /* +0x0C */
    s16 sy;     /* +0x0E */
    RECT image; /* +0x10 */
};

/* BloodType.mode runs the same four phases as GoreType.mode: airborne
 * until it lands, then spread, linger, and fade out. */
enum
{
    BLOOD_MODE_AIRBORNE = 0,
    BLOOD_MODE_SPREAD = 1,
    BLOOD_MODE_LINGER = 2,
    BLOOD_MODE_FADE = 3
};

struct BloodType /* size 36 */
{
    struct AreaNodeType *hint; /* +0x0 */
    long px;                   /* +0x4 */
    long py;                   /* +0x8 */
    long pz;                   /* +0xc */
    long scale;                /* +0x10 */
    long rotate;               /* +0x14 */
    short time;                /* +0x18 */
    short vx;                  /* +0x1a */
    short vy;                  /* +0x1c */
    short vz;                  /* +0x1e */
    /* Retail redesigns the demo's two-byte mode/bright tail: the renderers
     * consume a halfword fade plus separate sprite and phase bytes. */
    u16 brightness; /* +0x20 — retail halfword fade/brightness */
    u8 sprite;      /* +0x22 — sprBlood/sprBloodStay selection */
    u8 mode;        /* +0x23 — retail draw phase */
};

struct BleedType /* size 32 */
{
    VECTOR pos;  /* +0x0 */
    SVECTOR vec; /* +0x10 */
    u8 r;        /* +0x18 */
    u8 g;        /* +0x19 */
    u8 b;        /* +0x1a */
    u8 time;     /* +0x1b */
    u8 mode;     /* +0x1c */
};

/* SplashType.mode: the first frame spawns the droplet burst, then the
 * column rises over `speed` frames and collapses again over another. */
enum
{
    SPLASH_MODE_SPAWN = 0,
    SPLASH_MODE_RISE = 1,
    SPLASH_MODE_FALL = 2
};

struct SplashType /* size 20 */
{
    long px;  /* +0x0 */
    long py;  /* +0x4 */
    long pz;  /* +0x8 */
    short sx; /* +0xc */
    short sy; /* +0xe */
    u8 speed; /* +0x10 */
    u8 count; /* +0x11 */
    u8 mode;  /* +0x12 */
};

struct FrameType /* size 24 */
{
    GsCOORDINATE2 *super; /* +0x0 */
    long px;              /* +0x4 */
    long py;              /* +0x8 */
    long pz;              /* +0xc */
    short size;           /* +0x10 */
    short count;          /* +0x12 */
    u8 mode;              /* +0x14 */
};

/* ExplosionType.mode: the flash frame and the fireball both grow, on two
 * different sprites; the last phase shrinks and alpha-fades out. */
enum
{
    EXPLOSION_MODE_FLASH = 0,
    EXPLOSION_MODE_EXPAND = 1,
    EXPLOSION_MODE_FADE = 2
};

struct ExplosionType /* size 36 (aka HinokoType — reference/psxsym-types.h
                        aliases the same struct twice); DrawHinoko.c's own
                        param view, offsets proven from its raw .s (a
                        DIFFERENT layout from BloodType even though it
                        overlaps the same union bytes) */
{
    SVECTOR vec; /* +0x0 */
    VECTOR pos;  /* +0x8 */
    long rotate; /* +0x18 */
    long scale;  /* +0x1c */
    u8 time;     /* +0x20 */
    u8 mode;     /* +0x21 */
};

/* GoreType.mode: SetGore launches a gob airborne, DrawGore walks it the
 * rest of the way -- on landing it spreads into a pool, sits for a while,
 * then fades its brightness to nothing and releases the slot. */
enum
{
    GORE_MODE_AIRBORNE = 0,
    GORE_MODE_SPREAD = 1,
    GORE_MODE_LINGER = 2,
    GORE_MODE_FADE = 3
};

struct GoreType /* size 32 */
{
    VECTOR pos;  /* +0x00 */
    SVECTOR vec; /* +0x10 */
    long col;    /* +0x18 */
    u8 time;     /* +0x1C */
    u8 mode;     /* +0x1D */
};

struct SmokeType /* size 0x24; PSX.SYM supplies every demo member/name, and
                  * retail appends the sprite selector at +0x22 */
{
    SVECTOR vec; /* +0x0 */
    VECTOR pos;  /* +0x8 */
    long rotate; /* +0x18 */
    long scale;  /* +0x1c */
    u8 time;     /* +0x20 */
    u8 evtime;   /* +0x21 */
    u8 sprite;   /* +0x22 (retail) */
};

typedef struct FlyWireType FlyWireType;

struct FlyWireType /* fields through 0x44, naturally rounded to size 0x48;
                    * proven by DrawFlyWire/SetFlyWire */
{
    VECTOR start;   /* +0x00 */
    VECTOR end;     /* +0x10 */
    VECTOR center;  /* +0x20 */
    VECTOR NCenter; /* +0x30 */
    short count;    /* +0x40 */
    short time;     /* +0x42 */
    u8 mode;        /* +0x44 */
};

/* Retail-only full-screen fade state, proven jointly by set_fade_ and its
 * renderer draw_fade_. The renderer interpolates r/g/b between start_time
 * and end_time, advances mode through fade-in/hold/fade-out, and submits a
 * screen-sized POLY_XF4 at `OTablePt->org + priority`. This is effect state,
 * not PSX.SYM's standalone POLY_XF4 drawing helper. */
/* FadeType.mode: ramp the colour up over `duration`, hold it, then ramp
 * it back down and release the slot. */
enum
{
    FADE_MODE_IN = 0,
    FADE_MODE_HOLD = 1,
    FADE_MODE_OUT = 2
};

struct FadeType /* size 20 */
{
    u8 r;            /* +0x00 */
    u8 g;            /* +0x01 */
    u8 b;            /* +0x02 */
    long priority;   /* +0x04 */
    long start_time; /* +0x08 */
    long end_time;   /* +0x0C */
    u8 mode;         /* +0x10 */
};

union EffectParam /* size 72 (union EFFECT__180fake) */
{
    struct BloodType blood;
    struct BleedType bleed;
    struct SmokeType smoke;
    struct ExplosionType explosion;
    struct ExplosionType hinoko;
    struct GoreType gore;
    struct FlyWireType flywire;
    struct SplashType splash;
    struct ImpactType impact;
    struct FrameType frame;
    struct FadeType fade;
    struct SnowParticleType snow;
    struct TexScroll texscroll;
};

typedef struct tag_EffectSlot /* size 76 */
{
    void (*proc)();
    union EffectParam param;
} TEffectSlot;

extern TEffectSlot EffectSlot[N_EFFECT_SLOTS];
extern int EFFECT_CURSOR_; /* the pool's round-robin cursor */
extern TEffectSlot dmy;                                     /* pool-full fallback write target, discarded */

/* Retail's sprite selector banks initialized together in InitEffect. */
enum
{
    N_AIRBORNE_BLOOD_SPRITES = 2,
    N_BLOOD_SPRITES = N_AIRBORNE_BLOOD_SPRITES * 2,
    SMOKE_SPRITE_NORMAL = 0,
    SMOKE_SPRITE_ALT = 1,
    N_SMOKE_SPRITES = SMOKE_SPRITE_ALT + 1,
    BOMB_SPRITE_FLASH = 0,
    BOMB_SPRITE_EXPANDED = 1,
    BOMB_SPRITE_HINOKO = 2,
    N_EXPLOSION_SPRITES = BOMB_SPRITE_HINOKO + 1,
    SNOW_SPRITE_DEFAULT = 0,
    N_SNOW_SPRITES = SNOW_SPRITE_DEFAULT + 1
};

/* Retail expands the demo's singleton blood sprites into four variants. */
extern GsSPRITE sprBlood[N_BLOOD_SPRITES];
extern GsSPRITE sprBloodStay[N_BLOOD_SPRITES];
extern GsSPRITE sprFrame[MaxFrames];
extern GsSPRITE sprSplash;
/* Retail replaces the demo's three Sprite3D pointers with five GsSPRITEs. */
/* Impact-flash sprite count — official demo name (EFFECT.C's enum,
 * demo value 3); retail extends the family to 5. */
#define MaxImpacts 5
extern GsSPRITE sprImpact[MaxImpacts];
extern POLY_F4 plyBleed;
/* Retail stores two smoke sprites before the next global. */
extern Sprite3D *sprSmoke[N_SMOKE_SPRITES];
extern Sprite3D *sprBomb[N_EXPLOSION_SPRITES];
/* Retail keeps the original SpriteSnow name as a one-entry selector table. */
extern Sprite3D *SpriteSnow[N_SNOW_SPRITES];
extern ModelType *ModelHook;
extern ModelType *ShadowMdl;
extern void DrawGore(TEffectSlot *ef);
extern void SetGore(GsCOORDINATE2 *coord, SVECTOR *position, SVECTOR *vector);
extern void UpdateTexScroll(TEffectSlot *ef);
extern void SetSnow(VECTOR *pos, SVECTOR *velocity, s32 size, u8 sprite);

#endif
