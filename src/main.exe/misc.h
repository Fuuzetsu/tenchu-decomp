#ifndef MISC_H
#define MISC_H

/*
 * Shared types of the original MISC.C translation unit (the misc-object
 * spawner AddMisc.c and its dispatch table ProcMisc*, plus InitMisc/
 * DoMiscProc). Layouts follow the demo's PSX.SYM (reference/psxsym-types.h,
 * union MISC__181fake) and are proven by AddMisc.c/ResetAllMisc.c (matched)
 * plus this batch. AddMisc.c/ResetAllMisc.c predate this header and keep
 * their own local (duplicate) tag_TMisc — left as-is, not worth touching a
 * matched file to converge on this header.
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
typedef struct
{
    u8 type; /* 0x0 */
} TSprite;

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

/* Opaque here: InitMisc only moves ModelType pointers between LoadModel and
 * the DoorData/PitfallData tables, never dereferences a field (contrast
 * ProcMiscDoor/ProcMiscPitfall, unmatched, which do and would need the full
 * layout — reuse item.h's ModelType there instead of this stub). */
typedef struct ModelType ModelType;

/* Sprite3D extended with the embedded 2D GsSPRITE (item.h's Sprite3D is
 * truncated at 0x68, right where this `sprite` field starts — same
 * "local wrapper reuses item.h's Sprite3D instead of redefining it" idea as
 * PutItemIcon.c's ItemIconType, but this TU doesn't include item.h at all,
 * so the full struct is declared directly instead of composing). */
typedef struct Sprite3D Sprite3D;
struct Sprite3D
{
    GsCOORDINATE2 locate; /* 0x00 */
    SVECTOR rotate;       /* 0x50 */
    s16 id;               /* 0x58 */
    s16 attribute;        /* 0x5A */
    SVECTOR clip;         /* 0x5C */
    s32 scale;            /* 0x64 */
    GsSPRITE sprite;      /* 0x68 */
};                        /* 0x8C */

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
