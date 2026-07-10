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

/* HUMAN.C's trace-line (SetupTraceLine/ControlTraceLine/DestroyTraceLine):
 * a small ring of waypoints a Humanoid follows. Same layout as
 * DestroyTraceLine.c's independently (WORLD.C) redefined TraceLine/
 * TracePoint — reference/psxsym-types.h's struct TraceLine/TracePoint agree
 * (x/z long, range/pad short). Shared here since three item-TU-family
 * functions (SetupTraceLine/ControlTraceLine/KillHumanoid family) all touch
 * Humanoid.trace. */
typedef struct
{
    s32 x;       /* 0x0 */
    s32 z;       /* 0x4 */
    s16 range;   /* 0x8 */
    s16 pad;     /* 0xA */
} TracePoint;    /* 0xC */

typedef struct
{
    s16 index;        /* 0x0 */
    s16 count;        /* 0x2 */
    TracePoint *point; /* 0x4 */
} TraceLine;        /* 0x8 */

typedef struct tag_TItem tag_TItem;

/* game_types.h's `some_char_state_function` (a plain `s32(*)(void)`),
 * respelled locally so this header stays independent of game_types.h. The
 * Think1Func[]/Think2Func[]/Think3Func[]/Think4Func[] tables (SetupThinkFunction.c,
 * matched) are proven arrays of POINTERS TO this function-pointer type —
 * carried over here for Humanoid.think[4]'s element type. */
typedef s32 (*think_func_)(void);

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
    GsDOBJ2 object;               /* 0x64 (CreateCloneModel.c: valloc(sizeof(ModelType))
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

/* A single keyframe (Ghidra's own independently-built MotionElementType,
 * reference/ghidra_types.h:4861, cross-checked against psxsym-types.h:2649 —
 * both size 8, x/y/z/time all shorts; exercised by LoadMotion/HoldMotion/
 * GetSpline/ActiveMotion's mmp->motion->locate->x-style keyframe reads). */
typedef struct
{
    s16 x;                       /* 0x0 */
    s16 y;                       /* 0x2 */
    s16 z;                       /* 0x4 */
    s16 time;                    /* 0x6 */
} MotionElementType;               /* 0x8 */

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
                                    id/locate/rotate[1] after it) */
    s16 id;                       /* 0x6 (SearchMotion's `pMVar2->id == id` compare) */
    MotionElementType *locate;    /* 0x8 (LoadMotion's on-disk-offset ->
                                    absolute-pointer fixup; HoldMotion/
                                    ActiveMotion read ->x/->y/->z through it) */
    MotionElementType *rotate[1]; /* 0xC (per-bone keyframe list, indexed 0..n-1
                                    past the declared [1] — LoadMotion fixes up
                                    n entries, HoldMotion/ActiveMotion/SweepMotion
                                    read rotate[bone]->x/y/z) */
} MotionDataType;                  /* 0x10 */

/* GetMotionID's registry row (Ghidra's own independently-built type,
 * cross-checked: the `*8`-scaled index in GetMotionID's asm exactly matches
 * this struct's size). Sentinel-terminated array (mid == -1). */
typedef struct
{
    s16 mid;                      /* 0x0 */
    s16 id;                       /* 0x2 */
    MotionDataType *motion;       /* 0x4 */
} MotionRegistType;                /* 0x8 */

/* Spline interpolation state, one per animated bone (Ghidra's own
 * independently-built SplineControlType, reference/ghidra_types.h:5030,
 * cross-checked against psxsym-types.h:3604 — both size 0x18, matching
 * SetupMotionManager.c's own `(n + 1) * sizeof(SplineControlType), 0x18
 * bytes each` comment). key0/key1 bracket the current frame's keyframes;
 * dd0/ds1 are the precomputed per-frame deltas (GetSpline/
 * UpdateSplineControl). */
typedef struct
{
    MotionElementType *key0;      /* 0x0 */
    MotionElementType *key1;      /* 0x4 */
    SVECTOR dd0;                  /* 0x8 */
    SVECTOR ds1;                  /* 0x10 */
} SplineControlType;                /* 0x18 */

typedef struct
{
    s16 mid;                     /* 0x0 */
    s16 count;                   /* 0x2 */
    s16 loop;                    /* 0x4 */
    s16 n;                       /* 0x6 */
    s16 mask;                    /* 0x8 */
    s16 mode;                    /* 0xA */
    ModelArchiveType *model;     /* 0xC (HoldMotion/ActiveMotion: mmp->model->object) */
    MotionDataType *motion;      /* 0x10 */
    MotionRegistType *motreg;    /* 0x14 (GetMotionID's table) */
    SplineControlType *control;  /* 0x18 (freed by DisposeMotionManager;
                                    ActiveMotion indexes mmp->control + bone) */
} MotionManager;

/* On-disk motion pack: `n` pack-relative entries, each initially a byte
 * OFFSET from the pack base that LoadMotion fixes up in place into a real
 * MotionDataType pointer (Ghidra's own independently-built MotionPackType,
 * reference/ghidra_types.h:5206, cross-checked against psxsym-types.h:2669 —
 * both size 8). SearchMotion walks the fixed-up table by id. */
typedef struct
{
    s32 n;                        /* 0x0 */
    MotionDataType *motion[1];    /* 0x4 */
} MotionPackType;                  /* 0x8 */

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
    MapVector map;               /* 0x20 (game_types.h's MapVector, same
                                    character_state.map; GetNearestHumanoid
                                    confirms .height @0x24 via a plain
                                    `lw`/`== 0` test -- that access itself
                                    matches byte-for-byte, though the
                                    function as a whole is NON_MATCHING on
                                    an unrelated scheduling residual) */
    s16 attrib;                  /* 0x28 (StickonCheck: `& 0xc000`, read via
                                    `lh`; game_types.h's character_state
                                    twin at this same offset series proves
                                    the field u16 via a DIFFERENT TU's
                                    `lhu` — same per-TU load-width
                                    disagreement already established for
                                    lifemax) */
    u8 pad0b[0xE];               /* 0x2A (Ghidra: MapVector's degree/vector/
                                    direct/angleL/angleH, then 8 more
                                    unnamed bytes before `locate`; untouched
                                    by any matched function yet) */
    VECTOR *locate;              /* 0x38 */
    SVECTOR *rotate;             /* 0x3C (facing angles; MoveHumanoid reads .vy) */
    SVECTOR vector;              /* 0x40 (velocity; MoveHumanoid writes .vx/.vz) */
    u8 pad1[0x10];               /* 0x48 */
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
    ModelType *target;           /* 0x74 (AttackFire
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
    s16 warid;                   /* 0x8C (Ghidra's own independently-built
                                    Humanoid; actmode..warid untouched by any
                                    matched function yet, but the byte count
                                    to weapon_kind@0x8E is exact) */
    u16 weapon_kind;             /* 0x8E (EquipWeapon dispatches a weapon-
                                    swap by this field's value, read via
                                    lhu. Ghidra's own Humanoid names it
                                    `wpatk` (typed short there), but
                                    game_types.h's independently-proven
                                    character_state twin at this same
                                    cross-checked offset series (0x94/0xA4/
                                    0xAC/0xAE all agree with this struct)
                                    already types the field `u16
                                    weapon_kind` (enum weapon_kind, stored
                                    as 2 bytes) — kept unsigned per that
                                    proven sibling and the raw lhu width) */
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
extern s16 UpdateMotion(MotionManager *m, short id);
extern void MoveHumanoid(Humanoid *h, short a, short b);
extern void UpdateCoordinate(ModelType *m);
extern void DrawSprite(Sprite3D *s);
extern VECTOR *GetAbsolutePosition(ModelType *m, int x, int y, int z);
extern int ReqItemDrop(PARAM_ITEM_USE *p);
extern int ReqItemStay(PARAM_ITEM_STAY *p);
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
extern tag_TItem items[];

#endif

/* TENCHU_POSITIONAL_DATA_AREA_ is `struct { Sprite3D sprite; GsSPRITE gsp; } *[6]`.
 * Proof: SetupSprite's valloc(0x8C) == sizeof(Sprite3D) + sizeof(GsSPRITE) exactly,
 * and CVAsetup's fade bytes are the embedded GsSPRITE's .r/.g/.b. */
