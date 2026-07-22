#ifndef EFFECT_H
#define EFFECT_H

/* EFFECT.C's effect-slot pool. PSX.SYM supplies the original effect records
 * and union members; retail expands the impact record and adds snow,
 * flat-quad, and packed texture-scroll state. FlyWireType keeps the union at
 * its proven 72-byte size, making tag_EffectSlot's indexed pool stride 76
 * bytes. */

struct AreaNodeType; /* opaque here: only ever stored, never dereferenced */

typedef struct BloodType BloodType;
typedef struct BleedType BleedType;
typedef struct SplashType SplashType;
typedef struct FrameType FrameType;
typedef struct ExplosionType ExplosionType;
typedef struct ExplosionType HinokoType;
typedef struct GoreType GoreType;
typedef struct XF4Type XF4Type;
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

/* Retail's expanded version of PSX.SYM's ImpactType. */
struct ImpactType /* size 36 */
{
    s32 px;                    /* +0x00 */
    s32 py;                    /* +0x04 */
    s32 pz;                    /* +0x08 */
    GsCOORDINATE2 *super;      /* +0x0C */
    ImpactColor start_color;   /* +0x10 */
    ImpactColor end_color;     /* +0x14 */
    s16 start_size;            /* +0x18 */
    s16 end_size;              /* +0x1A */
    s16 rotate;                /* +0x1C */
    s16 rotate_speed;          /* +0x1E */
    u8 type;                   /* +0x20 */
    u8 count;                  /* +0x21 */
    u8 time;                   /* +0x22 */
};

/* Retail-only snowfall particle, proven jointly by SetSnow and DrawSnow. */
struct SnowParticleType /* size 32 */
{
    s32 x;                     /* +0x00 */
    s32 y;                     /* +0x04 */
    s32 z;                     /* +0x08 */
    s32 ground;                /* +0x0C */
    s32 sample_y;              /* +0x10 */
    s32 size;                  /* +0x14 */
    s16 velocity[3];           /* +0x18 */
    u8 sprite;                 /* +0x1E */
};

/* Retail embeds a shortened form of PSX.SYM's TexScroll in an EffectSlot.
 * The px/py accumulators and vx/vy deltas keep their recovered identities;
 * only the demo's time/count pair is absent. */
struct TexScroll /* size 24 */
{
    s16 px;      /* +0x00 */
    s16 py;      /* +0x02 */
    s16 vx;      /* +0x04 */
    s16 vy;      /* +0x06 */
    s16 x;       /* +0x08 */
    s16 y;       /* +0x0A */
    s16 sx;      /* +0x0C */
    s16 sy;      /* +0x0E */
    RECT image;  /* +0x10 */
};

struct BloodType /* size 36 */
{
    struct AreaNodeType *hint; /* +0x0 */
    long px;                   /* +0x4 */
    long py;                   /* +0x8 */
    long pz;                   /* +0xc */
    long scale;                /* +0x10 */
    long rotate;               /* +0x14 */
    short time;                 /* +0x18 */
    short vx;                   /* +0x1a */
    short vy;                   /* +0x1c */
    short vz;                   /* +0x1e */
    /* Retail redesigns the demo's two-byte mode/bright tail: the renderers
     * consume a halfword fade plus separate sprite and phase bytes. */
    u16 brightness;             /* +0x20 — retail halfword fade/brightness */
    u8 sprite;                  /* +0x22 — sprBlood/sprBloodStay selection */
    u8 mode;                    /* +0x23 — retail draw phase */
};

struct BleedType /* size 32 */
{
    VECTOR pos;   /* +0x0 */
    SVECTOR vec;  /* +0x10 */
    u8 r;         /* +0x18 */
    u8 g;         /* +0x19 */
    u8 b;         /* +0x1a */
    u8 time;      /* +0x1b */
    u8 mode;      /* +0x1c */
};

struct SplashType /* size 20 */
{
    long px;    /* +0x0 */
    long py;    /* +0x4 */
    long pz;    /* +0x8 */
    short sx;   /* +0xc */
    short sy;   /* +0xe */
    u8 speed;   /* +0x10 */
    u8 count;   /* +0x11 */
    u8 mode;    /* +0x12 */
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

struct ExplosionType /* size 36 (aka HinokoType — reference/psxsym-types.h
                        aliases the same struct twice); DrawHinoko.c's own
                        param view, offsets proven from its raw .s (a
                        DIFFERENT layout from BloodType even though it
                        overlaps the same union bytes) */
{
    SVECTOR vec;  /* +0x0 */
    VECTOR pos;   /* +0x8 */
    long rotate;  /* +0x18 */
    long scale;   /* +0x1c */
    u8 time;      /* +0x20 */
    u8 mode;      /* +0x21 */
};

struct GoreType /* size 32 */
{
    VECTOR pos;   /* +0x00 */
    SVECTOR vec;  /* +0x10 */
    long col;     /* +0x18 */
    u8 time;      /* +0x1C */
    u8 mode;      /* +0x1D */
};

struct SmokeType /* size 0x24; PSX.SYM supplies every demo member/name, and
                    * retail appends the sprite selector at +0x22 */
{
    SVECTOR vec;  /* +0x0 */
    VECTOR pos;   /* +0x8 */
    long rotate;  /* +0x18 */
    long scale;   /* +0x1c */
    u8 time;      /* +0x20 */
    u8 evtime;    /* +0x21 */
    u8 sprite;    /* +0x22 (retail) */
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

/* Offsets proven by FUN_80038fdc (tools/access.py): +0/+1/+2 one-byte
 * fields, +0x4/+0x8/+0xc a long triple sharing BloodType's px/py/pz layout,
 * +0x10 one more byte. Distinct from BloodType at +0x0 (BloodType.hint is a
 * 4-byte pointer; here it's three separate 1-byte stores) — the "divergent
 * union member" case in docs/matching-cookbook.md. Tentatively named XF4:
 * FUN_80038fdc's `proc` callback (FUN_80036284) calls AddXF4/SetPolyXF4,
 * and reference/psxsym-unplaced.tsv has an un-located demo `DrawXF4`
 * (EFFECT.C:1785) — this family always pairs SetX/DrawX (SetBlood/
 * DrawBlood, SetSplash/DrawSplash, ...), so FUN_80038fdc/FUN_80036284 are
 * good SetXF4/DrawXF4 candidates. Not corroborated by callmatch.py yet —
 * do not treat the name as confirmed. */
struct XF4Type /* size >= 0x11 */
{
    u8 unk0;   /* +0x0 */
    u8 unk1;   /* +0x1 */
    u8 unk2;   /* +0x2 */
    long px;   /* +0x4 */
    long py;   /* +0x8 */
    long pz;   /* +0xc */
    u8 unk10;  /* +0x10 */
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
    struct XF4Type xf4;
    struct SnowParticleType snow;
    struct TexScroll texscroll;
};

typedef struct tag_EffectSlot /* size 76 */
{
    void (*proc)();
    union EffectParam param;
} TEffectSlot;

extern TEffectSlot EffectSlot[200];
extern int CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_; /* the pool's round-robin cursor */
extern TEffectSlot dmy; /* pool-full fallback write target, discarded */
extern GsSPRITE sprFrame[MaxFrames];
extern GsSPRITE sprSplash;
extern POLY_F4 plyBleed;
/* Retail stores two smoke sprites before the next global. */
extern Sprite3D *sprSmoke[2];
extern Sprite3D *sprBomb[3];
extern ModelType *ModelHook;
extern ModelType *ShadowMdl;
extern void UpdateTexScroll(TEffectSlot *ef);
extern void SetSnow(VECTOR *pos, SVECTOR *velocity, s32 size, u8 sprite);

#endif
