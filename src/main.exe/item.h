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

typedef struct tag_TItem TItem;

struct AreaNodeType;
struct AfterimageType;

/* game_types.h's `some_char_state_function` (a plain `s32(*)(void)`),
 * respelled locally so this header stays independent of game_types.h. The
 * Think1Func[]/Think2Func[]/Think3Func[]/Think4Func[] tables (SetupThinkFunction.c,
 * matched) are proven arrays of POINTERS TO this function-pointer type —
 * carried over here for Humanoid.think[4]'s element type. */
typedef s32 (*think_func_)(void);

/* The henshin disguise's saved model state. PSX.SYM recovers the original
 * field names and its fifteen-part capacity; retail keeps the same layout. */
typedef struct HenshinModelPart HenshinModelPart;
struct HenshinModelPart
{
    u_long *tmd;                  /* 0x00 */
    s16 x;                        /* 0x04 */
    s16 y;                        /* 0x06 */
    s16 z;                        /* 0x08 */
};                                /* 0x0C */

typedef struct HenshinModelSnapshot HenshinModelSnapshot;
struct HenshinModelSnapshot
{
    s32 waist;                    /* 0x00 */
    HenshinModelPart p[15];       /* 0x04 */
};                                /* 0xB8 */

typedef struct Humanoid
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
    MapVector map;               /* 0x20 (retail adds area/index to the
                                    PSX.SYM-proven 0x10-byte demo record) */
    VECTOR *locate;              /* 0x38 */
    SVECTOR *rotate;             /* 0x3C (facing angles; MoveHumanoid reads .vy) */
    SVECTOR vector;              /* 0x40 (velocity; MoveHumanoid writes .vx/.vz) */
    VECTOR slocate;              /* 0x48 (ControlHumanoid snapshots *locate) */
    ModelArchiveType *model;     /* 0x58 */
    MotionManager *motion;       /* 0x5C */
    think_func_ *think[4];       /* 0x60 (per-think-level dispatch function
                                    pointers, game_types.h's character_state
                                    twin: think_setting0..3 @0x60/0x64/0x68/
                                    0x6C, same `some_char_state_function *`
                                    type — Think3callaid.c proves this offset
                                    series with four whole-word stores of
                                    Think1Func[4]/Think2Func[4]/Think3Func[4]/
                                    Think4Func[4]). Matches Ghidra's own
                                    independently-built Humanoid's
                                    `pointer *think[4]` here too. */
    TraceLine *trace;            /* 0x70 (SetupTraceLine/ControlTraceLine;
                                    Ghidra's own independently-built Humanoid
                                    also names this exact offset `trace`) */
    ModelType *target;           /* 0x74 (handle_char_state_attacking_SEVEN_
                                    reads target->locate.coord.t[1], the Y
                                    translation of the target's world matrix,
                                    for a lightning-bolt end point) */
    s32 point[2];                /* 0x78 (ground X/Z spawn position --
                                    BreedLife: point[0]=x via `sw a1,0x78(s0)`,
                                    point[1]=z via `sw s4,0x7C(s0)`; matches
                                    Ghidra's own independently-built Humanoid's
                                    `long point[2]` at this offset) */
    s32 chase[2];                /* 0x80 (Ghidra's own independently-built
                                    Humanoid; untouched by any matched
                                    function yet) */
    u8 actmode;                  /* 0x88 */
    u8 actflg;                   /* 0x89 */
    u8 actcnt;                   /* 0x8A */
    u8 actscnt;                  /* 0x8B */
    s16 warid;                   /* 0x8C */
    s16 wpatk;                   /* 0x8E (PSX.SYM's original weapon-attack
                                    pattern field; retail keeps the signed
                                    short and shifts later fields by eight
                                    bytes with the expanded MapVector) */
    s16 wepid[2];                /* 0x90 (GetWeaponData: `human->wepid[wpid]
                                    = i;`, an `sh` store — proves this field;
                                    Ghidra's own independently-built Humanoid
                                    names it `wepid[2]` too; character_state:
                                    field58_0x90..field61_0x93) */
    OrnamentType *weapon[4];     /* 0x94 (equipped weapon ornaments — right/
                                    left active + right/left inactive per
                                    game_types.h's character_state sibling
                                    view of this same offset; AttackPQD
                                    swaps weapon[0] with weapon[2]/[3] to
                                    draw/holster) */
    void *illusion[2];           /* 0xA4 (Ghidra: `pointer illusion[2]` —
                                    opaque, matches character_state's
                                    some_afterimage_1/2_0xa4/0xa8; untouched
                                    by any matched function yet) */
    u16 sound;                   /* 0xAC (default sound id, OR'd into Sound()'s seid) */
    s16 active_item;             /* 0xAE (TItemType the character is using) */
    s32 field76_0xb0;            /* 0xB0 (packed movement hint/count word;
                                    StateTransition and the character_state
                                    twin both read/write it as a whole word) */
    u8 item[0x1A];               /* 0xB4 (carry count per TItemType — ProcItemDrop;
                                    DoInfoViewProc's cursor wraps at index 0x19) */
} Humanoid;

typedef struct PARAM_ITEM_LAUNCH
{
    TItemType type;              /* 0x00 */
    Humanoid *user;              /* 0x04 */
    VECTOR start;                /* 0x08 */
    VECTOR end;                  /* 0x18 */
} PARAM_ITEM_LAUNCH;             /* 0x28 */

/* PSX.SYM records both names for this request structure. */
typedef struct PARAM_ITEM_LAUNCH PARAM_ITEM_USE;

/* A stationary/placed item's spawn params (AddItem2's ReqItemStay). */
typedef struct PARAM_ITEM_STAY
{
    TItemType type;              /* 0x00 */
    VECTOR locate;               /* 0x04 */
} PARAM_ITEM_STAY;               /* 0x14 */

/* Rolling-item states from ITEM.C's anonymous enum. Retail adds state 5,
 * whose name is not present in the demo symbols. */
enum
{
    KORO_NORMAL = 0,
    KORO_WATER = 1,
    KORO_GRAND = 2,
    KORO_WALL = 3,
    KORO_STAY = 4
};

/* Item-processor teardown sentinel from ITEM.C's anonymous enum. */
enum
{
    ITEM_MODE_DISPOSE = 0xff
};

/* ITEM.C's original rolling-item base.  Derived item parameters embed this
 * 12-byte record as their first member (param_drop, param_smoke,
 * param_ningyo, param_ninken, param_dokudango). */
typedef struct param_korogari
{
    struct AreaNodeType *hint;   /* 0x0 */
    s16 vx;                      /* 0x4 */
    s16 vy;                      /* 0x6 */
    s16 vz;                      /* 0x8 */
    u8 status;                   /* 0xA */
} param_korogari;                /* 0xC */

/* ITEM.C's flying-item trajectory. The union allows a projectile to switch
 * from its three-point flight curve to the rolling-item state in place. */
struct tag_fly
{
    s32 sx;                       /* 0x00 */
    s32 sy;                       /* 0x04 */
    s32 sz;                       /* 0x08 */
    s32 vx;                       /* 0x0C */
    s32 vy;                       /* 0x10 */
    s32 vz;                       /* 0x14 */
    s32 rx;                       /* 0x18 */
    s32 ry;                       /* 0x1C */
    s32 rz;                       /* 0x20 */
    u8 count;                     /* 0x24 */
    u8 count2;                    /* 0x25 */
};                                /* 0x28 */

typedef struct param_fly
{
    union
    {
        struct tag_fly fly;
        param_korogari koro;
    } p;                          /* 0x00 */
    u8 mode;                      /* 0x28 */
} param_fly;                      /* 0x2C */

typedef struct param_arrow
{
    param_fly fly;                /* 0x00 */
    u8 count;                     /* 0x2C */
} param_arrow;                    /* 0x30 */

typedef struct param_launch
{
    param_fly fly;                /* 0x00 */
    struct AfterimageType *effect; /* 0x2C */
    u8 count;                     /* 0x30 */
} param_launch;                   /* 0x34 */

typedef struct param_drop
{
    param_korogari koro;         /* 0x00 */
    u8 count;                    /* 0x0C */
} param_drop;                    /* 0x10 */

typedef struct param_smoke
{
    param_korogari koro;         /* 0x00 */
    u8 count;                    /* 0x0C */
} param_smoke;                   /* 0x10 */

typedef struct param_ningyo
{
    param_korogari koro;         /* 0x00 */
    u8 count;                    /* 0x0C */
    u8 hp;                       /* 0x0D */
} param_ningyo;                  /* 0x10 */

typedef struct param_ninken
{
    param_korogari koro;         /* 0x00 */
    Humanoid *slave;             /* 0x0C */
    s16 count;                   /* 0x10 */
} param_ninken;                  /* 0x14 */

/* ITEM.C's remaining small parameter records.  These names and layouts are
 * taken directly from PSX.SYM; keeping them distinct reflects the original
 * source even where two item kinds happen to have the same representation. */
typedef struct param_goshikimai
{
    SVECTOR vec;                  /* 0x00 */
} param_goshikimai;               /* 0x08 */

typedef struct param_gosin
{
    s16 count;                    /* 0x00 */
} param_gosin;                    /* 0x02 */

typedef struct param_gun
{
    VECTOR vec;                   /* 0x00 */
} param_gun;                      /* 0x10 */

typedef struct param_henshin
{
    s16 count;                    /* 0x00 */
    u8 lock;                      /* 0x02 */
} param_henshin;                  /* 0x04 */

typedef struct param_kaengeki
{
    VECTOR start;                 /* 0x00 */
    VECTOR end;                   /* 0x10 */
    u8 count;                     /* 0x20 */
} param_kaengeki;                 /* 0x24 */

typedef struct param_lightningbolt
{
    VECTOR start;                 /* 0x00 */
    SVECTOR rot;                  /* 0x10 */
    u8 count;                     /* 0x18 */
} param_lightningbolt;            /* 0x1C */

typedef struct param_napalm
{
    SVECTOR vec;                  /* 0x00 */
    u8 count;                     /* 0x08 */
} param_napalm;                   /* 0x0A */

typedef struct param_shinsoku
{
    SVECTOR vec;                  /* 0x00 */
    u8 count;                     /* 0x08 */
} param_shinsoku;                 /* 0x0A */

/* ITEM.C's human-search scratch record (PSX.SYM's own struct TFindItemTarget,
 * reference/psxsym-types.h:3769 — field names are the authors' own). The
 * setup/search blocks in ProcItemSmoke/ProcItemDokudango view a shared stack
 * buffer through this. */
typedef struct TFindItemTarget
{
    Humanoid *find;              /* 0x00 (the found target) */
    s32 dist;                    /* 0x04 */
    s32 i;                       /* 0x08 (scan resume index) */
    VECTOR pos;                  /* 0x0C (search center) */
    s32 find_dist;               /* 0x1C (max/best distance) */
} TFindItemTarget;               /* 0x20 */

/* ProcItemDokudango's retail union view.  This uses the original embedded
 * rolling-item base; retail widens count to a halfword versus the demo's byte. */
typedef struct param_dokudango
{
    param_korogari koro;         /* 0x00 */
    Humanoid *eater;             /* 0x0C */
    void *org_think;             /* 0x10 */
    u16 count;                   /* 0x14 (retail accesses it with lhu/sh) */
} param_dokudango;               /* 0x18 */

struct tag_TItem
{
    Humanoid *owner;             /* 0x00 */
    ModelType *model;            /* 0x04 (official type; sprite-backed retail
                                    items use the shared transform prefix) */
    TItemType type;              /* 0x08 */
    void (*proc)(TItem *);       /* 0x0C */
    ModelType *locate;           /* 0x10 */
    struct
    {
        s32 mode;                /* 0x00 */
        s32 pause;               /* 0x04 */
        s16 size;                /* 0x08 */
        s16 ofsY;                /* 0x0A */
    } collision;                 /* 0x14, size 0x0C */
    union
    {
        param_launch launch;
        param_smoke smoke;
        param_drop drop;
        param_gun gun;
        param_arrow arrow;
        param_napalm napalm;
        param_ningyo ningyo;
        param_goshikimai goshikimai;
        param_ninken ninken;
        param_dokudango dokudango;
        param_drop kusuri;
        param_kaengeki kaengeki;
        param_henshin henshin;
        param_lightningbolt lightningbolt;
        param_gosin gosin;
        param_shinsoku shinsoku;
    } param;                     /* 0x20, size 0x34 */
    u8 mode;                     /* 0x54 */
};                               /* sizeof = 0x58 (items[] stride) */

extern void dispose_weapon_data_of_char_(Humanoid *h, int a);
extern s16 UpdateMotion(MotionManager *m, short id);
extern void MoveHumanoid(Humanoid *h, short a, short b);
extern void UpdateCoordinate(ModelType *m);
extern void DrawSprite(Sprite3D *s);
extern VECTOR *GetAbsolutePosition(ModelType *m, int x, int y, int z);
extern int ReqItemDrop(PARAM_ITEM_LAUNCH *p);
extern int ReqItemStay(PARAM_ITEM_STAY *p);
extern void ReqItemDefault(Humanoid *user, TItemType item);
extern TItemType GetItemType(s32 conflict_id);
extern void SetBleed(VECTOR *pos, SVECTOR *vel, int a, int col);
extern void SetSmoke(VECTOR *pos, SVECTOR *vel, short n, short time);
extern s16 SoundEx(VECTOR *loc, short id);
extern void DeleteConflict(ModelType *m);
extern void AdtMessageBox(char *fmt, ...);
extern int rand(void);
extern void *memset(void *s, int c, u32 n);

/* Absolute in this TU (defined by think's TU, gp there — see the gp note). */
extern s16 ActionHalt;
/* "item dispose fail   id %d  mode %d" */
extern char D_800121CC[];
/* The global item pool. */
extern TItem items[];

#endif
