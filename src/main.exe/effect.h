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

union EffectParam /* size 72 (union EFFECT__180fake) */
{
    struct BloodType blood;
    struct BleedType bleed;
    struct SplashType splash;
    struct FrameType frame;
    struct ExplosionType explosion;
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
