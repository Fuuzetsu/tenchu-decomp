#ifndef EFFECT_H
#define EFFECT_H

/* EFFECT.C's effect-slot pool. PSX.SYM supplies the original effect records
 * and union members; retail redesigns and extends the impact record and adds
 * snow, screen-fade, and packed texture-scroll state. FlyWireType keeps the
 * union at its proven 72-byte size, making tag_EffectSlot's indexed pool
 * stride 76 bytes. */

struct AreaNodeType; /* opaque here: only ever stored, never dereferenced */
struct Humanoid;

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
typedef struct tag_EffectSlot TEffectSlot;
typedef void (*EffectProc)(TEffectSlot *effect);

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

/* Retail's five impact sprites. The first three identities are corroborated
 * by their TIM artwork and SetImpact callers; the last two are used only by
 * their namesake item effects. */
typedef u8 impact_sprite;
enum impact_sprite
{
    IMPACT_SPRITE_GUN = 0,
    IMPACT_SPRITE_FLASH = 1,
    IMPACT_SPRITE_HIT = 2,
    IMPACT_SPRITE_SHINSOKU = 3,
    IMPACT_SPRITE_GOSIN = 4
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
    impact_sprite type;      /* +0x20 */
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

/* SetupTexScroll copies a source TIM into any selected cells of a 2x2 VRAM
 * grid. Retail always selects all four cells; the demo exposed this mask as
 * the function's `mode` argument. The resulting image is scrolled in 1/16
 * texel units, while successive grids are packed into 0x40-pixel VRAM slots. */
enum texscroll_cell_mask
{
    TEXSCROLL_COPY_TOP_LEFT = 1 << 0,
    TEXSCROLL_COPY_TOP_RIGHT = 1 << 1,
    TEXSCROLL_COPY_BOTTOM_LEFT = 1 << 2,
    TEXSCROLL_COPY_BOTTOM_RIGHT = 1 << 3,
    TEXSCROLL_COPY_ALL = TEXSCROLL_COPY_TOP_LEFT |
                         TEXSCROLL_COPY_TOP_RIGHT |
                         TEXSCROLL_COPY_BOTTOM_LEFT |
                         TEXSCROLL_COPY_BOTTOM_RIGHT
};

enum texscroll_layout
{
    TEXSCROLL_GRID_COLUMNS = 2,
    TEXSCROLL_GRID_ROWS = 2,
    TEXSCROLL_SUBPIXEL_BITS = 4,
    TEXSCROLL_SUBPIXEL_SCALE = 1 << TEXSCROLL_SUBPIXEL_BITS,
    TEXSCROLL_VRAM_ORIGIN_X = 0x340,
    TEXSCROLL_VRAM_ORIGIN_Y = 0x100,
    TEXSCROLL_VRAM_Y_LIMIT = 0x200,
    TEXSCROLL_VRAM_SLOT_STRIDE = 0x40
};

/* BloodType.mode runs the same four phases as GoreType.mode: airborne
 * until it lands, then spread, linger, and fade out. */
typedef u8 blood_mode;
enum blood_mode
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
    blood_mode mode; /* +0x23 — retail draw phase */
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

/* SetBleeds and SetBleedsDir build a 16-byte position, then reuse the same
 * storage as two adjacent short vectors while preparing the particle. */
typedef union BleedSpawnVectors BleedSpawnVectors;
union BleedSpawnVectors
{
    VECTOR position;
    struct
    {
        SVECTOR velocity;
        SVECTOR temporary;
    } vector;
};

/* SplashType.mode: the first frame spawns the droplet burst, then the
 * column rises over `speed` frames and collapses again over another. */
typedef u8 splash_mode;
enum splash_mode
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
    splash_mode mode; /* +0x12 */
};

typedef u8 frame_mode;
enum frame_mode
{
    FRAME_MODE_FLASH = 0,
    FRAME_MODE_FADE = 1
};

struct FrameType /* size 24 */
{
    GsCOORDINATE2 *super; /* +0x0 */
    long px;              /* +0x4 */
    long py;              /* +0x8 */
    long pz;              /* +0xc */
    short size;           /* +0x10 */
    s16 count;            /* +0x12 (PSX.SYM's original field) */
    frame_mode mode;      /* +0x14 */
};

/* Damage floors alternate a large launched burst with a smaller flash and
 * bleed effect attached to one body part. */
typedef enum DamageEffectKind DamageEffectKind;
enum DamageEffectKind
{
    DAMAGE_EFFECT_ATTACHED_FLASH = 0,
    DAMAGE_EFFECT_NAPALM = 1
};

/* ExplosionType.mode: the flash frame and the fireball both grow, on two
 * different sprites; the last phase shrinks and alpha-fades out. */
typedef u8 explosion_mode;
enum explosion_mode
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
    explosion_mode mode; /* +0x21 */
};

/* GoreType.mode: SetGore launches a gob airborne, DrawGore walks it the
 * rest of the way -- on landing it spreads into a pool, sits for a while,
 * then fades its brightness to nothing and releases the slot. */
typedef u8 gore_mode;
enum gore_mode
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
    gore_mode mode; /* +0x1D */
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

typedef u8 flywire_mode;
enum flywire_mode
{
    FLYWIRE_MODE_EXTEND = 0,
    FLYWIRE_MODE_STRAIGHTEN = 1
};

struct FlyWireType /* fields through 0x44, naturally rounded to size 0x48;
                    * proven by DrawFlyWire/SetFlyWire */
{
    VECTOR start;   /* +0x00 */
    VECTOR end;     /* +0x10 */
    VECTOR center;  /* +0x20 */
    VECTOR NCenter; /* +0x30 */
    short count;    /* +0x40 */
    short time;     /* +0x42 */
    flywire_mode mode; /* +0x44 */
};

/* Retail-only full-screen fade state, proven jointly by set_fade_ and its
 * renderer draw_fade_. The renderer interpolates r/g/b between start_time
 * and end_time, advances mode through fade-in/hold/fade-out, and submits a
 * screen-sized POLY_XF4 at `OTablePt->org + priority`. This is effect state,
 * not PSX.SYM's standalone POLY_XF4 drawing helper. */
/* FadeType.mode: ramp the colour up over `duration`, hold it, then ramp
 * it back down and release the slot. */
typedef u8 fade_mode;
enum fade_mode
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
    fade_mode mode;  /* +0x10 */
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

struct tag_EffectSlot /* size 76 */
{
    EffectProc proc;
    union EffectParam param;
};

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
#define MaxImpacts (IMPACT_SPRITE_GOSIN + 1)
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
extern void spawn_damage_effect_(struct Humanoid *human, DamageEffectKind kind);
extern void UpdateTexScroll(TEffectSlot *ef);
extern void SetSnow(VECTOR *pos, SVECTOR *velocity, s32 size, u8 sprite);

#endif
