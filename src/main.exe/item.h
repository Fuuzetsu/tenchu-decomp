#ifndef ITEM_H
#define ITEM_H

/* ModelType.attribute bit 15: ComputeAllConflict raises it on both models
 * when it records a collision result for the frame; every GetConflictResult
 * caller tests it before reading the result table. */
/* ModelType.attribute draw/cull configuration (the Draw* family's atr
 * tests, in test order) and the collision pair: */
/* Humanoid.wpatk packs the weapon's range class in the high nibble
 * (0-2 melee reach tiers, WPATK_CLASS_RANGED for bows/guns) and the
 * attack-pattern index in the low nibble. */
#define WPATK_CLASS(w) ((w) >> 4)
#define WPATK_CLASS_RANGED 3
#define N_WPATK_CLASSES (WPATK_CLASS_RANGED + 1)

/* ModelArchiveType.object[] roles proven by the humanoid callers.  Some
 * specialized skeletons reuse indices (the beast's first striking limb is
 * at the ordinary humanoid head index).  The weapon anchors vary with
 * wpatk; the ordinal hand names intentionally do not claim left or right. */
#define MODEL_PART_WAIST 0
#define MODEL_PART_HEAD 2
#define MODEL_PART_BEAST_HAND_0 2
#define MODEL_PART_BEAST_HAND_1 1
#define MODEL_PART_ONININ_HAND_0 8
#define MODEL_PART_ONININ_HAND_1 0x0B
#define MODEL_PART_WEAPON_HAND_0 0x0D
#define MODEL_PART_WEAPON_HAND_1 0x0E
#define N_NINJA_MODEL_PARTS (MODEL_PART_WEAPON_HAND_1 + 1)

/* The hand index shared by hand[], wepid[], and GetWeaponData's wpid. */
#define WEAPON_HAND_0 0
#define WEAPON_HAND_1 1
#define WEAPON_HAND_NONE (-1)

/* Humanoid.weapon[] holds the two active ornaments followed by their two
 * inactive alternatives. */
#define WEAPON_SLOT_ACTIVE_0 0
#define WEAPON_SLOT_ACTIVE_1 1
#define WEAPON_SLOT_INACTIVE_0 2
#define WEAPON_SLOT_INACTIVE_1 3
#define WEAPON_SLOT_COUNT 4
#define WEAPON_SLOT_NONE (-1)

/* Components of Humanoid.point (home/spawn point) and chase (AI scratch
 * point).  spread_blood_pool_ deliberately reuses chase[0] as a timer. */
#define HUMANOID_HOME_X 0
#define HUMANOID_HOME_Z 1
#define HUMANOID_CHASE_X 0
#define HUMANOID_CHASE_Z 1

/* Humanoid.pad_hold packs a virtual-pad latch: hold `button` for
 * `frames` frames (StateTransition's update_hint unpacks it as
 * pad_hold >> 16 and (u8)pad_hold). */
#define PAD_HOLD(button, frames) (((button) << 16) | (frames))

#define MODEL_ATTR_HIDDEN 0x0001      /* never draw (ActDEAD sinks drowned
                                         corpses with it) */
#define MODEL_ATTR_NOCULL 0x0002      /* skip the whole clip-point test */
#define MODEL_ATTR_CULL_BEHIND 0x0004 /* reject when the clip point is
                                         behind the camera (sz == 0) */
#define MODEL_ATTR_CULL_SCREEN 0x0008 /* reject outside the half-screen
                                         bounds (|x|>=0xf1 or |y|>=0xb5) */
#define MODEL_ATTR_CULL_FAR 0x0010    /* reject beyond the depth splice
                                         (sz > 0x4e2) */
#define MODEL_ATTR_COLLIDE 0x4000     /* conflict slot active (ActivateHumans
                                         raises it; the conflict queries gate
                                         on it) */
#define MODEL_ATTR_CONFLICT 0x8000

/*
 * Shared types + externs of the original item translation unit (ProcItem*,
 * ReqItem*). Layouts follow Ghidra's build-verified model; every offset here
 * is proven by a byte-matched function (see docs/matching-cookbook.md).
 * gp note: the original ITEM.C translation unit defines ic (gp-relative;
 * listed in Build.hs maspsxGpExterns for the files that touch it). Its
 * references to ActionHalt/EmergencyNotice are absolute (gp in
 * think's TU).
 */

typedef struct tag_TItem TItem;

struct AreaNodeType;
struct AfterimageType;

/* ITEM.C's original file-static initialization flag and item-pool cursor.
 * fInitial is qualified because this split decomp also exposes MISC.C's
 * distinct static with the same original name. */
extern u8 Item_fInitial;
extern s32 ic;

/* THINK_4.C's original short-returning dispatch tables. PSX.SYM supplies
 * both their element type and exact bounds; Humanoid stores one selected
 * function from each table.
 *
 * ThinkDB[] (retail data at 0x80089e40) names every think type the level
 * editor could assign, and the name says where it lives: the leading digit
 * is the table, the letter is the index in it, and the packed value puts
 * that index in the matching nibble — which is what SetupThinkFunction
 * unpacks. Read out, it is
 *
 *     Think1Func  3 1A-TRACE   4 1B-WATCH    5 1C-RANDOM
 *                 6 1D-NINJA   7 1E-SLEEP    8 1F-CHASE
 *     Think2Func  3 2A-CONFIRM 4 2B-CONTACT
 *     Think3Func  3 3A-CALLAID 4 3B-ATK-CHASE 5 3C-ATK-POINT
 *                 6 3D-ESCAPE  7 3E-ATK-AREA  8 3F-ATK-HITAWAY
 *     Think4Func  3 4A-ABANDON 4 4B-CONTACT   5 4C-CHASE
 *
 * plus the two whole-word player types 0x1111 and 0x2222 ("1 -PAD 1" and
 * "1 -PAD 2"). Our Think* function names already match these throughout.
 * Only the indices some caller spells literally are named here. */
typedef s16 (*ThinkFunc)(void);
extern ThinkFunc Think1Func[10];
extern ThinkFunc Think2Func[5];
extern ThinkFunc Think3Func[10];
extern ThinkFunc Think4Func[6];
extern ThinkFunc AttackFunc[N_WPATK_CLASSES];

/* Think1watch/Think1target act on the ticks where actcnt's low bits are
 * clear, so the character looks around once per this many idle ticks. */
#define THINK_IDLE_PERIOD 0x80

#define THINK1_WATCH 4     /* 1B-WATCH */
#define THINK2_CONTACT 4   /* 2B-CONTACT */
#define THINK3_ATK_CHASE 4 /* 3B-ATK-CHASE */
#define THINK4_CONTACT 4   /* 4B-CONTACT */

/* The henshin disguise's saved model state. PSX.SYM recovers the original
 * field names and its fifteen-part capacity; retail keeps the same layout. */
typedef struct HenshinModelPart HenshinModelPart;
struct HenshinModelPart
{
    u_long *tmd; /* 0x00 */
    s16 x;       /* 0x04 */
    s16 y;       /* 0x06 */
    s16 z;       /* 0x08 */
}; /* 0x0C */

typedef struct HenshinModelSnapshot HenshinModelSnapshot;
struct HenshinModelSnapshot
{
    s32 waist;                                /* 0x00 */
    HenshinModelPart p[N_NINJA_MODEL_PARTS]; /* 0x04 */
}; /* 0xB8 */

/* ITEM.C's original disguise snapshot. Retail adds a second snapshot for
 * the disguise target model; no original name for that addition is known. */
extern HenshinModelSnapshot Item_save;
extern HenshinModelSnapshot HenshinSnapshot;

/* Per-stage disguise character-type pair (what the henshin potion turns
 * you into). The original demo ITEM.C table had nine anonymous two-byte
 * rows; retail has one row per stage configuration followed by a zero pair.
 * Retail data: townsfolk on the early stages (JOCHU/MUSUME/CHONIN, rouban
 * guards), then the stage's own faction — Manji cultists, tengu, oni,
 * kabane, kerai, asigaru, sisi. */
#define N_HENSHIN_STAGE_ROWS (N_STAGE_CONFIGS + 1)
extern struct
{
    u8 type[N_PLAYABLE_CHARACTERS];
} HensinT[N_HENSHIN_STAGE_ROWS];

typedef struct Humanoid
{
    s16 type;                 /* 0x00 */
    s16 status;               /* 0x02 */
    s16 attribute; /* 0x04 (the ATTR_* bit word — see humanoid.h. The s16
                      declaration with per-site *(u16 *)& views is measured:
                      flipping the field to u16 changes the plain sites'
                      retail lh loads to lhu — the signed/unsigned mix is
                      the original's own per-site choice) */
    s16 turn;                 /* 0x06 */
    s16 life;                 /* 0x08 */
    s16 lifemax;              /* 0x0A (PSX.SYM's original signed maximum-life field) */
    s16 width;                /* 0x0C */
    s16 height;               /* 0x0E */
    PADtype pad;              /* 0x10 (DoInfoViewProc reads .data/.trig) */
    MapVector map;            /* 0x20 (retail adds area/index to the
                                 PSX.SYM-proven 0x10-byte demo record) */
    VECTOR *locate;           /* 0x38 */
    SVECTOR *rotate;          /* 0x3C (facing angles; MoveHumanoid reads .vy) */
    SVECTOR vector;           /* 0x40 (velocity; MoveHumanoid writes .vx/.vz) */
    VECTOR slocate;           /* 0x48 (ControlHumanoid snapshots *locate) */
    ModelArchiveType *model;  /* 0x58 */
    MotionManager *motion;    /* 0x5C */
    ThinkFunc think[4];       /* 0x60 (PSX.SYM: short (*think[4])(); retail
                                 shifts the demo's +0x58 by the eight-byte
                                 MapVector expansion) */
    TraceLine *trace;         /* 0x70 (SetupTraceLine/ControlTraceLine;
                                 Ghidra's own independently-built Humanoid
                                 also names this exact offset `trace`) */
    ModelType *target;        /* 0x74 (launch_lightning_bolt_
                                 reads target->locate.coord.t[1], the Y
                                 translation of the target's world matrix,
                                 for a lightning-bolt end point) */
    s32 point[2];             /* 0x78 (ground X/Z spawn position --
                                 BreedLife: point[0]=x via `sw a1,0x78(s0)`,
                                 point[1]=z via `sw s4,0x7C(s0)`; matches
                                 Ghidra's own independently-built Humanoid's
                                 `long point[2]` at this offset) */
    s32 chase[2];             /* 0x80: AI scratch — the chase/flank point
                                 (ChasetoTarget, Think*chase), reused as
                                 the blood-pool timer and death spot */
    u8 actmode;               /* 0x88 */
    u8 actflg;                /* 0x89 */
    /* Free-running idle counter for the Think1* wander states. It only
     * advances while the character is NOT acting: the act phase is the
     * ticks where the low bits are clear (0 and 0x80), during which the
     * counter holds while actscnt runs out, then one increment releases
     * it to count through the idle stretch again. So the mask below sets
     * how long the character waits between look-arounds. */
    u8 actcnt;                /* 0x8A */
    u8 actscnt;               /* 0x8B */
    s16 warid;                /* 0x8C */
    s16 wpatk;                /* 0x8E (PSX.SYM's original weapon-attack
                                 pattern field; retail keeps the signed
                                 short and shifts later fields by eight
                                 bytes with the expanded MapVector) */
    s16 wepid[2];             /* 0x90 (GetWeaponData: `human->wepid[wpid]
                                 = i;`, an `sh` store — proves this field;
                                 Ghidra's own independently-built Humanoid
                                 names it `wepid[2]` too; character_state:
                                 field58_0x90..field61_0x93) */
    OrnamentType *weapon[4];  /* 0x94 (equipped weapon ornaments — right/
                                 left active + right/left inactive per
                                 game_types.h's character_state sibling
                                 view of this same offset; AttackPQD
                                 swaps weapon[0] with weapon[2]/[3] to
                                 draw/holster) */
    void *illusion[2];        /* 0xA4 (PSX.SYM's original type; these
                                 opaque effect pointers are passed to the
                                 afterimage draw/dispose API) */
    s16 sound;                /* 0xAC (PSX.SYM name) SE-bank base: Sound()
                               * ORs category ids < 0x10 into it */
    s16 itmctl;               /* 0xAE (PSX.SYM's item-control field;
                                 retail shifts it eight bytes from +0xA6) */
    s32 pad_hold;             /* 0xB0 (packed AI pad command/duration;
                                 high half carries D-pad bits and the low
                                 byte counts remaining frames) */
    u8 item[N_ITEM_SLOTS];    /* 0xB4 (carry count per TItemType — ProcItemDrop;
                               * DoInfoViewProc's cursor wraps at ITEM_N) */
} Humanoid;

typedef struct PARAM_ITEM_LAUNCH
{
    TItemType type;  /* 0x00 */
    Humanoid *user;  /* 0x04 */
    VECTOR start;    /* 0x08 */
    VECTOR end;      /* 0x18 */
} PARAM_ITEM_LAUNCH; /* 0x28 */

/* PSX.SYM records both names for this request structure. */
typedef struct PARAM_ITEM_LAUNCH PARAM_ITEM_USE;

/* The thrown/placed-item request used by Makibishi and Jirai.  Retail adds
 * the owner pointer at +0x04 to the demo's PSX.SYM layout, leaving its
 * start/velocity roles intact and making it the same size as the launch
 * request. */
typedef struct PARAM_ITEM_DROP
{
    TItemType type; /* 0x00 */
    Humanoid *user; /* 0x04 (retail) */
    VECTOR start;   /* 0x08 */
    VECTOR vec;     /* 0x18 */
} PARAM_ITEM_DROP;  /* 0x28 (demo: 0x24, without user) */

/* A stationary/placed item's spawn params (AddItem2's ReqItemStay). */
typedef struct PARAM_ITEM_STAY
{
    TItemType type; /* 0x00 */
    VECTOR locate;  /* 0x04 */
} PARAM_ITEM_STAY;  /* 0x14 */

/* ITEM.C's saved item-layout record.  PackItemLayout and RestoreItemLayout
 * respectively produce and consume this original PSX.SYM layout. */
typedef struct TItemLayout
{
    s32 type;      /* 0x00 */
    VECTOR locate; /* 0x04 */
} TItemLayout;     /* 0x14 */

/* Rolling-item states from ITEM.C's anonymous enum. Retail adds state 5
 * (entered when CGetLevel reports the item left the map); its name is not
 * in the demo symbols, so KORO_OUT is our invention. */
enum
{
    KORO_NORMAL = 0,
    KORO_WATER = 1,
    KORO_GRAND = 2,
    KORO_WALL = 3,
    KORO_STAY = 4,
    KORO_OUT = 5
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
    struct AreaNodeType *hint; /* 0x0 */
    s16 vx;                    /* 0x4 */
    s16 vy;                    /* 0x6 */
    s16 vz;                    /* 0x8 */
    u8 status;                 /* 0xA */
} param_korogari;              /* 0xC */

/* ITEM.C's flying-item trajectory. The union allows a projectile to switch
 * from its three-point flight curve to the rolling-item state in place. */
struct tag_fly
{
    s32 sx;    /* 0x00 */
    s32 sy;    /* 0x04 */
    s32 sz;    /* 0x08 */
    s32 vx;    /* 0x0C */
    s32 vy;    /* 0x10 */
    s32 vz;    /* 0x14 */
    s32 rx;    /* 0x18 */
    s32 ry;    /* 0x1C */
    s32 rz;    /* 0x20 */
    u8 count;  /* 0x24 */
    u8 count2; /* 0x25 */
}; /* 0x28 */

typedef struct param_fly
{
    union
    {
        struct tag_fly fly;
        param_korogari koro;
    } p;     /* 0x00 */
    u8 mode; /* 0x28 */
} param_fly; /* 0x2C */

typedef struct param_arrow
{
    param_fly fly; /* 0x00 */
    u8 count;      /* 0x2C */
} param_arrow;     /* 0x30 */

typedef struct param_launch
{
    param_fly fly;                 /* 0x00 */
    struct AfterimageType *effect; /* 0x2C */
    u8 count;                      /* 0x30 */
} param_launch;                    /* 0x34 */

typedef struct param_drop
{
    param_korogari koro; /* 0x00 */
    u8 count;            /* 0x0C */
} param_drop;            /* 0x10 */

/* Kusuri used the shared drop parameters under its original source name. */
typedef struct param_drop param_kusuri;

typedef struct param_smoke
{
    param_korogari koro; /* 0x00 */
    u8 count;            /* 0x0C */
} param_smoke;           /* 0x10 */

typedef struct param_ningyo
{
    param_korogari koro; /* 0x00 */
    u8 count;            /* 0x0C */
    u8 hp;               /* 0x0D */
} param_ningyo;          /* 0x10 */

typedef struct param_ninken
{
    param_korogari koro; /* 0x00 */
    Humanoid *slave;     /* 0x0C */
    s16 count;           /* 0x10 */
} param_ninken;          /* 0x14 */

/* ITEM.C's remaining small parameter records.  These names and layouts are
 * taken directly from PSX.SYM; keeping them distinct reflects the original
 * source even where two item kinds happen to have the same representation. */
typedef struct param_goshikimai
{
    SVECTOR vec;    /* 0x00 */
} param_goshikimai; /* 0x08 */

typedef struct param_gosin
{
    /* Retail uses unsigned halfword loads; the demo declared this signed. */
    u16 count; /* 0x00 */
} param_gosin; /* 0x02 */

typedef struct param_gun
{
    VECTOR vec; /* 0x00 */
} param_gun;    /* 0x10 */

typedef struct param_henshin
{
    s16 count;   /* 0x00 */
    u8 lock;     /* 0x02 */
} param_henshin; /* 0x04 */

typedef struct param_kaengeki
{
    VECTOR start; /* 0x00 */
    VECTOR end;   /* 0x10 */
    u8 count;     /* 0x20 */
} param_kaengeki; /* 0x24 */

typedef struct param_lightningbolt
{
    VECTOR start;      /* 0x00 */
    SVECTOR rot;       /* 0x10 */
    u8 count;          /* 0x18 */
} param_lightningbolt; /* 0x1C */

typedef struct param_napalm
{
    SVECTOR vec; /* 0x00 */
    u8 count;    /* 0x08 */
} param_napalm;  /* 0x0A */

typedef struct param_shinsoku
{
    SVECTOR vec;  /* 0x00 */
    u8 count;     /* 0x08 */
} param_shinsoku; /* 0x0A */

/* ITEM.C's human-search scratch record (PSX.SYM's own struct TFindItemTarget,
 * reference/psxsym-types.h:3769 — field names are the authors' own). The
 * setup/search blocks in ProcItemSmoke/ProcItemDokudango view a shared stack
 * buffer through this. */
typedef struct TFindItemTarget
{
    Humanoid *find; /* 0x00 (the found target) */
    s32 dist;       /* 0x04 */
    s32 i;          /* 0x08 (scan resume index) */
    VECTOR pos;     /* 0x0C (search center) */
    s32 find_dist;  /* 0x1C (max/best distance) */
} TFindItemTarget;  /* 0x20 */

/* ProcItemDokudango's retail union view.  This uses the original embedded
 * rolling-item base; retail widens count to a halfword versus the demo's byte. */
typedef struct param_dokudango
{
    param_korogari koro; /* 0x00 */
    Humanoid *eater;     /* 0x0C */
    void *org_think;     /* 0x10 */
    u16 count;           /* 0x14 (retail accesses it with lhu/sh) */
} param_dokudango;       /* 0x18 */

struct tag_TItem
{
    Humanoid *owner;       /* 0x00 */
    ModelType *model;      /* 0x04 (official type; sprite-backed retail
                              items use the shared transform prefix) */
    TItemType type;        /* 0x08 */
    void (*proc)(TItem *); /* 0x0C */
    ModelType *locate;     /* 0x10 */
    struct
    {
        s32 mode;  /* 0x00 */
        s32 pause; /* 0x04 */
        s16 size;  /* 0x08 */
        s16 ofsY;  /* 0x0A */
    } collision;   /* 0x14, size 0x0C */
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
        param_kusuri kusuri;
        param_kaengeki kaengeki;
        param_henshin henshin;
        param_lightningbolt lightningbolt;
        param_gosin gosin;
        param_shinsoku shinsoku;
    } param; /* 0x20, size 0x34 */
    u8 mode; /* 0x54 */
}; /* sizeof = 0x58 (items[] stride) */

/* AttackCancelControl's independently selectable cleanup work. */
#define ATTACK_CANCEL_CONFLICTS 1
#define ATTACK_CANCEL_AFTERIMAGES 2
#define ATTACK_CANCEL_ALL \
    (ATTACK_CANCEL_CONFLICTS | ATTACK_CANCEL_AFTERIMAGES)

extern void AttackCancelControl(s16 mode);
/* Sets the motion globals and forwards to AttackCancelControl. */
extern void dispose_weapon_data_of_char_(Humanoid *h, int mode);
extern s16 UpdateMotion(MotionManager *m, short id);
extern short DrawSprite(Sprite3D *sprt);
extern VECTOR *GetAbsolutePosition(ModelType *model, short x, short y, short z);
extern int ReqItemDrop(PARAM_ITEM_LAUNCH *p);
extern int ReqItemStay(PARAM_ITEM_STAY *p);
extern void ReqItemDefault(Humanoid *user, TItemType item);
extern TItemType GetItemType(s32 conflict_id);
extern void AdtMessageBox(char *fmt, ...);
extern int rand(void);
extern void *memset(void *s, int c, u32 n);

/* "item dispose fail   id %d  mode %d" */
extern char msg_item_dispose_fail[]; /* "item dispose fail   id %d  mode %d" */
/* The global item pool. */
/* The item-teardown sequence ITEM.C pastes at every dispose site: run
 * the handler once in dispose mode, drop the collision entry, report a
 * handler that failed to clear its mode, and free the slot. Macro is
 * reconstruction shorthand for that copy-paste (expands to the
 * identical text). */
#define DISPOSE_ITEM(item)                                                    \
    item->mode = ITEM_MODE_DISPOSE;                                           \
    item->proc(item);                                                         \
    DeleteConflict(item->locate);                                             \
    if (item->mode != 0)                                                      \
    {                                                                         \
        AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);    \
    }                                                                         \
    item->owner = 0;                                                          \
    item->proc = 0;

#define MAX_ITEMS 30

/* Register an item's cubic conflict box and mirror it into the item's
 * own collision record — the block every armed item pastes after
 * InsertConflict. Macro is reconstruction shorthand (expands to the
 * identical text; register-pinned operands pass through unchanged). */
#define SET_ITEM_COLLISION(n, sz, owner, cmode)                               \
    ConflictObject[n].offset.vx = 0;                                          \
    ConflictObject[n].offset.vz = 0;                                          \
    ConflictObject[n].offset.vy = 0;                                          \
    ConflictObject[n].size.vz = sz;                                           \
    ConflictObject[n].size.vy = sz;                                           \
    ConflictObject[n].size.vx = sz;                                           \
    ConflictObject[n].common = owner;                                         \
    ConflictObject[n].size.pad = cmode;                                       \
    item->collision.size = sz;                                                \
    item->collision.ofsY = 0;                                                 \
    item->collision.mode = cmode;                                             \
    item->collision.pause = 0;

/* The launcher preamble every ReqItem* repeats: round-robin the pool
 * cursor `ic` to the next free slot, force-disposing the slot it lands
 * on when all 30 are live, leaving `item` set and `found:` planted.
 * Macro is reconstruction shorthand (expands to the identical text). */
#define TAKE_ITEM_SLOT()                                                      \
    i = 0;                                                                    \
    do                                                                        \
    {                                                                         \
        ic++;                                                                 \
        if (ic >= MAX_ITEMS)                                                  \
            ic = 0;                                                           \
        item = items + ic;                                                    \
        if (item->proc == 0)                                                  \
            goto found;                                                       \
        i++;                                                                  \
    } while (i < MAX_ITEMS - 1);                                              \
                                                                              \
    DISPOSE_ITEM(item);                                                       \
                                                                              \
found:
/* The same launcher preamble, in the cursor variant some ReqItem* use: the
 * scan keeps its recovered `ret` cursor and only publishes `item` once.
 * This gives cc1 two pseudos where TAKE_ITEM_SLOT() gives it one (adopting
 * the single-variable macro in those files costs 40 diff lines). Macro is
 * reconstruction shorthand for the copy-paste (expands to identical text). */
#define TAKE_ITEM_SLOT_VIA_CURSOR()                                           \
    i = 0;                                                                    \
    do                                                                        \
    {                                                                         \
        ic++;                                                                 \
        if (ic >= MAX_ITEMS)                                                  \
            ic = 0;                                                           \
        ret = items + ic;                                                     \
        if (ret->proc == 0)                                                   \
        {                                                                     \
            item = ret;                                                       \
            goto found;                                                       \
        }                                                                     \
        i++;                                                                  \
    } while (i < MAX_ITEMS - 1);                                              \
                                                                              \
    /* pool exhausted: force-dispose the slot the counter landed on */        \
    ret->mode = ITEM_MODE_DISPOSE;                                            \
    ret->proc(ret);                                                           \
    DeleteConflict(ret->locate);                                              \
    if (ret->mode != 0)                                                       \
    {                                                                         \
        AdtMessageBox(msg_item_dispose_fail, ret->type, (u32)ret->mode);      \
    }                                                                         \
    item = ret;                                                               \
    item->owner = 0;                                                          \
    item->proc = 0;                                                           \
                                                                              \
found:

extern TItem items[MAX_ITEMS];
/* ITEM.C's shared model and sprite resources. */
extern ModelType *SyurikenModel;
extern ModelType *ArrowModel;
extern ModelType *NingyoModel;
extern ModelType *HappouModel;
extern Sprite3D *sprNapalm;
extern Sprite3D *sprNapalm2;
/* ITEM.C's single shared lock-on/trajectory marker. */
extern GsSPRITE TargetSprite[1];

#endif
