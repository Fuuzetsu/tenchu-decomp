#ifndef EFFECT_H
#define EFFECT_H

/* EFFECT.C's effect-slot pool. Struct/field names and offsets are the
 * authors' own (reference/psxsym-types.h); only the members our matched
 * functions actually touch are given real names, the rest is padding so
 * sizeof(union EffectParam) stays the proven 72 bytes (tag_EffectSlot's
 * pool stride, 76 bytes, must stay exact — EffectSlot is indexed). */

struct AreaNodeType; /* opaque here: only ever stored, never dereferenced */

typedef struct BloodType BloodType;
typedef struct BleedType BleedType;
typedef struct SplashType SplashType;
typedef struct FrameType FrameType;
typedef struct ExplosionType ExplosionType;
typedef struct XF4Type XF4Type;
typedef struct SmokeType SmokeType;
typedef struct ImpactType ImpactType;

typedef union ImpactColor ImpactColor;
union ImpactColor
{
    s32 word;
    struct
    {
        u8 b;
        u8 g;
        u8 r;
        u8 pad;
    } channel;
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

struct BloodType /* size 36 */
{
    struct AreaNodeType *hint; /* +0x0 */
    long px;                   /* +0x4 */
    long py;                   /* +0x8 */
    long pz;                   /* +0xc */
    long scale;                /* +0x10 */
    long rotate;                /* +0x14 */
    short time;                 /* +0x18 */
    short vx;                   /* +0x1a */
    short vy;                   /* +0x1c */
    short vz;                   /* +0x1e */
    u8 mode;                    /* +0x20 */
    u8 bright;                  /* +0x21 */
    u8 unk22;                   /* +0x22 — proven by FUN_8003944c (a real
                                  * field, not just alignment padding: it's
                                  * an explicit `sb` store there, distinct
                                  * from the struct's implicit round-up to
                                  * the union's 4-byte alignment). */
    u8 unk23;                   /* +0x23 -- proven by SetBlood: a genuine
                                  * `sb $zero, 0x23(...)` store, not just the
                                  * struct's own round-up to 36 bytes (which
                                  * would otherwise land silently on this same
                                  * byte). */
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

struct FlyWireType /* size 0x45, proven by DrawFlyWire/SetFlyWire's
                      * tools/access.py offsets */
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
    struct SplashType splash;
    struct FrameType frame;
    struct ExplosionType explosion;
    struct XF4Type xf4;
    struct SmokeType smoke;
    struct FlyWireType flywire;
    struct ImpactType impact;
    u8 pad[72];
};

typedef struct tag_EffectSlot /* size 76 */
{
    void (*proc)();
    union EffectParam param;
} TEffectSlot;

extern TEffectSlot EffectSlot[200];
extern int CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_; /* the pool's round-robin cursor */
extern TEffectSlot dmy; /* pool-full fallback write target, discarded */

#endif
