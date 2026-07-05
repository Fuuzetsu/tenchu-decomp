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

/* Opaque — only ever handled by pointer (moved around Humanoid's weapon[4],
 * never dereferenced by an item-TU function). Fully defined locally, per
 * this repo's convention, in DrawOrnament.c/CreateCloneOrnament.c/
 * UpdateOrnament.c (each its own TU; no conflict with this forward decl). */
typedef struct OrnamentType OrnamentType;

typedef struct ModelType ModelType;
struct ModelType
{
    GsCOORDINATE2 locate;        /* 0x00 */
    SVECTOR rotate;              /* 0x50 */
    s16 id;                      /* 0x58 */
    s16 attribute;               /* 0x5A */
    SVECTOR clip;                /* 0x5C */
    GsDOBJ2 object;               /* 0x64 (FUN_8001851c.c: valloc(sizeof(ModelType))
                                    == 0x74 only with this field; attribute/coord2/tmd
                                    written directly at +0x64/+0x68/+0x6C, matching
                                    Ghidra's own independently-built struct exactly —
                                    reference/ghidra_types.h:5021) */
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
    s16 time;                    /* 0x4 (PlayMotion.c: mmp->count compared
                                    against mmp->motion->time; matches
                                    Ghidra's own independently-built
                                    MotionDataType exactly — reference/
                                    ghidra_types.h:4994, which also has
                                    id/locate/rotate[1] after it, not yet
                                    exercised by any matched function) */
} MotionDataType;

/* GetMotionID's registry row (Ghidra's own independently-built type,
 * cross-checked: the `*8`-scaled index in GetMotionID's asm exactly matches
 * this struct's size). Sentinel-terminated array (mid == -1). */
typedef struct
{
    s16 mid;                      /* 0x0 */
    s16 id;                       /* 0x2 */
    MotionDataType *motion;       /* 0x4 */
} MotionRegistType;                /* 0x8 */

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
    MotionRegistType *motreg;    /* 0x14 (GetMotionID's table) */
    void *control;               /* 0x18 (freed by DisposeMotionManager) */
} MotionManager;

/* Per-controller state embedded in Humanoid (Ghidra: PADtype). */
typedef struct
{
    u16 data;                    /* 0x0 (held buttons) */
    u16 sdata;                   /* 0x2 */
    u16 trig;                    /* 0x4 (newly pressed this frame) */
    s16 time;                    /* 0x6 */
    u16 stream[4];               /* 0x8 */
} PADtype;                       /* 0x10 */

typedef struct
{
    s16 type;                    /* 0x00 */
    s16 status;                  /* 0x02 */
    s16 attribute;               /* 0x04 */
    s16 turn;                    /* 0x06 */
    s16 life;                    /* 0x08 */
    u16 lifemax;                 /* 0x0A (lhu in ProcItemKusuri; DoInfoViewProc lh's it via a (s16) cast) */
    s16 width;                   /* 0x0C */
    s16 height;                  /* 0x0E */
    PADtype pad;                 /* 0x10 (DoInfoViewProc reads .data/.trig) */
    u8 pad0[0x18];               /* 0x20 */
    VECTOR *locate;              /* 0x38 */
    SVECTOR *rotate;             /* 0x3C (facing angles; MoveHumanoid reads .vy) */
    SVECTOR vector;              /* 0x40 (velocity; MoveHumanoid writes .vx/.vz) */
    u8 pad1[0x10];               /* 0x48 */
    ModelArchiveType *model;     /* 0x58 */
    MotionManager *motion;       /* 0x5C */
    u8 pad2a[0x14];              /* 0x60 */
    ModelType *target;           /* 0x74 (handle_char_state_attacking_SEVEN_
                                    reads target->locate.coord.t[1], the Y
                                    translation of the target's world matrix,
                                    for a lightning-bolt end point) */
    u8 pad2b[0x1C];              /* 0x78 */
    OrnamentType *weapon[4];     /* 0x94 (equipped weapon ornaments — right/
                                    left active + right/left inactive per
                                    game_types.h's character_state sibling
                                    view of this same offset; FUN_80027688
                                    swaps weapon[0] with weapon[2]/[3] to
                                    draw/holster) */
    void *illusion[2];           /* 0xA4 (Ghidra: `pointer illusion[2]` —
                                    opaque, matches character_state's
                                    some_afterimage_1/2_0xa4/0xa8; untouched
                                    by any matched function yet) */
    u16 sound;                   /* 0xAC (default sound id, OR'd into Sound()'s seid) */
    s16 active_item;             /* 0xAE (item_kind2 the character is using) */
    u8 pad3[0x4];                /* 0xB0 */
    u8 item[0x1A];               /* 0xB4 (carry count per item_kind2 — ProcItemDrop;
                                    DoInfoViewProc's cursor wraps at index 0x19) */
} Humanoid;

typedef struct
{
    s32 type;                    /* 0x00 */
    Humanoid *user;              /* 0x04 */
    VECTOR start;                /* 0x08 */
    VECTOR end;                  /* 0x18 */
} PARAM_ITEM_USE;                /* 0x28 */

/* A stationary/placed item's spawn params (DebugMenuItemSet's ReqItemStay;
 * Ghidra models .type as `enum TItemType`, but every proven sibling (
 * tag_TItem.type, PARAM_ITEM_USE.type) uses plain s32 instead — same here). */
typedef struct
{
    s32 type;                    /* 0x00 */
    VECTOR locate;               /* 0x04 */
} PARAM_ITEM_STAY;               /* 0x14 */

/* The dropped/rolling item's view of tag_TItem.param (Ghidra: param_korogari). */
typedef struct
{
    void *hint;                  /* 0x0 (AreaNodeType *) */
    s16 vx;                      /* 0x4 */
    s16 vy;                      /* 0x6 */
    s16 vz;                      /* 0x8 */
    u8 status;                   /* 0xA */
    u8 pad;                      /* 0xB */
    u8 count;                    /* 0xC (settle/pickup frame counter — ProcItemDrop) */
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
extern s16 UpdateMotion(MotionManager *m, int id);
extern void MoveHumanoid(Humanoid *h, short a, short b);
extern void UpdateCoordinate(ModelType *m);
extern void DrawSprite(Sprite3D *s);
extern VECTOR *GetAbsolutePosition(ModelType *m, int x, int y, int z);
extern int ReqItemDrop(PARAM_ITEM_USE *p);
extern int ReqItemStay(PARAM_ITEM_STAY *p);
extern void SetBleed(VECTOR *pos, SVECTOR *vel, int a, int col);
extern void SetSmoke(VECTOR *pos, SVECTOR *vel, short n, short time);
extern s16 SoundEx(VECTOR *loc, int id);
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
