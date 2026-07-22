#ifndef MISC_H
#define MISC_H

/*
 * Shared types of the original MISC.C translation unit (the misc-object
 * spawner AddMisc.c and its dispatch table ProcMisc*, plus InitMisc/
 * DoMiscProc). Layouts follow the demo's PSX.SYM (reference/psxsym-types.h,
 * union MISC__181fake) and are proven by the matched AddMisc.c and
 * ResetAllMisc.c implementations as well as the processors that share them.
 */

typedef struct tag_TMisc tag_TMisc;

typedef struct
{
    s32 a; /* 0x0 */
    s32 b; /* 0x4 */
    s32 c; /* 0x8 */
} TMiscInit;

/* The MISC_SPRITE variant of the param union (MISC__181fake's `sprite`
 * member, union MISC__181fake in reference/psxsym-types.h) — a single byte
 * at the union's base offset, reused after CREATE clamps/narrows the raw
 * TMiscInit.a read down to a valid sprite-table index. */
typedef struct TSprite
{
    u8 type; /* 0x0 */
} TSprite;

/* The MISC_DOOR variant of the parameter union (PSX.SYM's TDoor). */
typedef struct TDoor
{
    ModelType *locate; /* 0x0 */
    s16 r;             /* 0x4 */
    s16 dr;            /* 0x6 */
    u8 type;           /* 0x8 */
    u8 pad[3];         /* 0x9 */
} TDoor;               /* 0xC */

/* The MISC_SNOWFALL variant of the param union (MISC__181fake's `snowfall`
 * member, reference/psxsym-types.h). */
typedef struct TSnowfall
{
    s32 w;          /* 0x0 */
    s32 h;          /* 0x4 */
    SVECTOR *snows; /* 0x8 */
} TSnowfall;

/* The MISC_PITFALL variant of the parameter union. */
typedef struct TPitfall
{
    ModelType *locate; /* 0x0 */
    s16 r;             /* 0x4 */
    u8 type;           /* 0x6 */
    u8 pad;            /* 0x7 */
} TPitfall;

struct tag_TMisc
{
    void (*proc)(tag_TMisc *, s32); /* 0x00 */
    s32 x;                          /* 0x04 */
    s32 y;                          /* 0x08 */
    s32 z;                          /* 0x0C */
    s32 count;                      /* 0x10 */
    u8 pause;                       /* 0x14 */
    u8 mode;                        /* 0x15 */
    union
    {
        TMiscInit init;
        TDoor door;
        TPitfall pitfall;
        TSnowfall snowfall;
        TSprite sprite;
    } param;                        /* 0x18 */
};                                  /* 0x24 */

enum TMiscMessage
{
    MM_CREATE = 0,
    MM_DESTROY = 1,
    MM_PAUSE = 2,
    MM_RESUME = 3
};

typedef struct
{
    ModelType *Model[2]; /* 0x0 */
    s16 HitSize;         /* 0x8 */
} DoorDataType;            /* 0xC, MISC__183fake */

typedef struct
{
    ModelType *Model[2]; /* 0x0 */
    s16 HitSize;         /* 0x8 */
} PitfallDataType;         /* 0xC, MISC__184fake */

typedef struct
{
    Sprite3D *spr; /* 0x0 */
    s32 scale;     /* 0x4 */
} SpriteDataType;           /* 0x8, MISC__185fake */

extern tag_TMisc misc[200];
extern DoorDataType DoorData[11];
extern PitfallDataType PitfallData[2];
extern SpriteDataType SpriteData[2];

/* This TU defines it (gp-relative; listed in Build.hs maspsxGpExterns for
 * InitMisc.c/DoMiscProc.c). Set by InitMisc's tail, checked by DoMiscProc. */
extern u8 EFFECT_SPAWNERS_INITIALISED;

extern void AdtMessageBox(char *fmt, ...);
extern int rand(void);

#endif
