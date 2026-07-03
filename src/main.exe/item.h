#ifndef ITEM_H
#define ITEM_H

/*
 * Shared types + externs of the original item translation unit (ProcItem*,
 * ReqItem*). Layouts follow Ghidra's build-verified model; every offset here
 * is proven by a byte-matched function (see docs/matching-cookbook.md).
 * gp note: this TU defines COUNTER_FOR_ITEM_ARRAY_ (gp-relative; listed in
 * Build.hs maspsxGpExterns for the files that touch it) but only *references*
 * ActionHalt/FRAMES_UNTIL_END_OF_ALERT (absolute here, gp in think's TU).
 */

typedef struct tag_TItem tag_TItem;

typedef struct ModelType ModelType;
struct ModelType
{
    GsCOORDINATE2 locate;        /* 0x00 */
    SVECTOR rotate;              /* 0x50 */
    s16 id;                      /* 0x58 */
    s16 attribute;               /* 0x5A */
    SVECTOR clip;                /* 0x5C */
};

typedef struct
{
    GsCOORDINATE2 locate;        /* 0x00 */
    SVECTOR rotate;              /* 0x50 */
    s16 id;                      /* 0x58 */
    s16 attribute;               /* 0x5A */
    SVECTOR clip;                /* 0x5C */
    s16 n;                       /* 0x64 */
    ModelType **object;          /* 0x68 */
} ModelArchiveType;

typedef struct
{
    GsCOORDINATE2 locate;        /* 0x00 */
    SVECTOR rotate;              /* 0x50 */
    s16 id;                      /* 0x58 */
    s16 attribute;               /* 0x5A */
    SVECTOR clip;                /* 0x5C */
    s32 scale;                   /* 0x64 */
} Sprite3D;

typedef struct
{
    u8 n;                        /* 0x0 */
    u8 sweep;                    /* 0x1 */
    u8 orderspd;                 /* 0x2 */
    u8 sidespd;                  /* 0x3 */
} MotionDataType;

typedef struct
{
    s16 mid;                     /* 0x0 */
    s16 count;                   /* 0x2 */
    s16 loop;                    /* 0x4 */
    s16 n;                       /* 0x6 */
    s16 mask;                    /* 0x8 */
    s16 mode;                    /* 0xA */
    void *model;                 /* 0xC */
    MotionDataType *motion;      /* 0x10 */
} MotionManager;

typedef struct
{
    s16 type;                    /* 0x00 */
    s16 status;                  /* 0x02 */
    s16 attribute;               /* 0x04 */
    s16 turn;                    /* 0x06 */
    s16 life;                    /* 0x08 */
    u16 lifemax;                 /* 0x0A */
    u8 pad0[0x2C];               /* 0x0C */
    VECTOR *locate;              /* 0x38 */
    u8 pad1[0x1C];               /* 0x3C */
    ModelArchiveType *model;     /* 0x58 */
    MotionManager *motion;       /* 0x5C */
    u8 pad2[0x4E];               /* 0x60 */
    s16 active_item;             /* 0xAE (item_kind2 the character is using) */
} Humanoid;

typedef struct
{
    s32 type;                    /* 0x00 */
    Humanoid *user;              /* 0x04 */
    VECTOR start;                /* 0x08 */
    VECTOR end;                  /* 0x18 */
} PARAM_ITEM_USE;                /* 0x28 */

/* The dropped/rolling item's view of tag_TItem.param (Ghidra: param_korogari). */
typedef struct
{
    void *hint;                  /* 0x0 (AreaNodeType *) */
    s16 vx;                      /* 0x4 */
    s16 vy;                      /* 0x6 */
    s16 vz;                      /* 0x8 */
    u8 status;                   /* 0xA */
} param_korogari;

struct tag_TItem
{
    Humanoid *owner;             /* 0x00 */
    Sprite3D *model;             /* 0x04 */
    s32 type;                    /* 0x08 */
    void (*proc)(tag_TItem *);   /* 0x0C */
    ModelType *locate;           /* 0x10 */
    s32 coll_mode;               /* 0x14 */
    s32 coll_pause;              /* 0x18 */
    s16 coll_size;               /* 0x1C */
    s16 coll_ofsY;               /* 0x1E */
    u8 param[0x34];              /* 0x20 (a union; view via casts, e.g. param_korogari) */
    u8 mode;                     /* 0x54 */
};                               /* sizeof = 0x58 (items[] stride) */

extern void dispose_weapon_data_of_char_(Humanoid *h, int a);
extern void UpdateMotion(MotionManager *m, int id);
extern void MoveHumanoid(Humanoid *h, int a, int b);
extern void UpdateCoordinate(ModelType *m);
extern void DrawSprite(Sprite3D *s);
extern VECTOR *GetAbsolutePosition(ModelType *m, int x, int y, int z);
extern int ReqItemDrop(PARAM_ITEM_USE *p);
extern void SetBleed(VECTOR *pos, SVECTOR *vel, int a, int col);
extern void SoundEx(VECTOR *loc, int id);
extern void DeleteConflict(ModelType *m);
extern void AdtMessageBox(char *fmt, ...);
extern int rand(void);
extern void *memset(void *s, int c, u32 n);

/* Absolute in this TU (defined by think's TU, gp there — see the gp note). */
extern s16 ActionHalt;
/* "item dispose fail   id %d  mode %d" */
extern char D_800121CC[];
/* The global item pool. */
extern tag_TItem items[];

#endif
