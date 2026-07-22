// Game data types for Tenchu main.exe — the canonical, build-verified type
// model (structs/enums/typedefs). Included by main.exe.h AFTER the PSY-Q
// SDK header and the base-int typedefs, so it may use GsIMAGE/VECTOR/u16/etc.

#include <psxsdk/libgpu.h>
//
// This file is the round-trip unit with Ghidra: `tools/sync_to_ghidra.py`
// pushes it into the Ghidra program; `tools/ghidra/ExportSymbolsTypes.java`
// exports Ghidra's version to reference/ghidra_types.h. `./Build check` is the
// arbiter — a type here is proven only if the build stays byte-identical.
//
// OWNER DIRECTIVE: always prefer the OFFICIAL recovered types over hand-guessed
// ones. When a type/field here duplicates a name recovered in
// reference/psxsym-types.h (the authors' own PSX.SYM) or item.h's official
// structs, migrate call sites to the official type/name and delete the guess,
// gating on byte-identical `./Build check`. Do not add new guessed duplicates
// of something the recovered symbols already name. (Completed example:
// the guessed `character_state` cluster WAS item.h's official Humanoid and
// has been fully retired — all Me_THINK_C derefs use `struct Humanoid` and
// its official fields, and the ~314-line cluster was deleted from this file.
// The offset-aligned field map is kept for reference:
// reference/character_state-to-humanoid.tsv.)

// One controller port's raw state — the official globals and PADCMD.C name
// this TPadPort (reference/psxsym-globals.h: `struct TPadPort PadPort[2][4]`).
// Retail inserted `active` at offset 6, making it 14 bytes vs the demo's 12
// (ComPad.c documents this). GetRealPad indexes the [port][slot] table and
// reads `button`.
typedef struct TPadPort TPadPort;
struct TPadPort
{
    u16 button;         /* 0x0 (held buttons) */
    s16 x;              /* 0x2 */
    s16 y;              /* 0x4 */
    u8 active;          /* 0x6 (retail-inserted) */
    u8 fAnalog;         /* 0x7 */
    u8 act1;            /* 0x8 */
    u8 act2;            /* 0x9 */
    u8 actbuf[2];       /* 0xA */
    u8 Send;            /* 0xC */
    u8 pad;             /* 0xD */
};

/* PADCMD.C's rumble attack/release envelope (anonymous in PSX.SYM). */
typedef struct PadArrangeType PadArrangeType;
struct PadArrangeType
{
    s32 pow;                           /* 0x00 */
    s32 time;                          /* 0x04 */
    s32 attack;                        /* 0x08 */
    s32 release;                       /* 0x0C */
};                                     /* 0x10 */

/* AdtSelect's menu row — the demo's own debug symbols name this TAdtSelect
 * (the PSX.SYM stack-variable records in FileOption/DoInfoViewProc/etc. call
 * these arrays `struct TAdtSelect [N]`). */
typedef struct TAdtSelect TAdtSelect;
struct TAdtSelect
{
    char *name;    /* 0x0 */
    u32 value;     /* 0x4 */
};

/* ADT's saved PSY-Q font settings. */
typedef struct AdtFntState AdtFntState;
struct AdtFntState
{
    s32 x;                           /* 0x00 */
    s32 y;                           /* 0x04 */
    s32 w;                           /* 0x08 */
    s32 h;                           /* 0x0C */
    s32 isbg;                        /* 0x10 */
    s32 n;                           /* 0x14 */
    s32 tx;                          /* 0x18 */
    s32 ty;                          /* 0x1C */
    s32 quiet;                       /* 0x20 */
};                                   /* 0x24 */

/* PSY-Q executable header, recovered verbatim in the demo's PSX.SYM. */
typedef struct EXEC EXEC;
struct EXEC
{
    u32 pc0;                       /* 0x00 */
    u32 gp0;                       /* 0x04 */
    u32 t_addr;                    /* 0x08 */
    u32 t_size;                    /* 0x0C */
    u32 d_addr;                    /* 0x10 */
    u32 d_size;                    /* 0x14 */
    u32 b_addr;                    /* 0x18 */
    u32 b_size;                    /* 0x1C */
    u32 s_addr;                    /* 0x20 */
    u32 s_size;                    /* 0x24 */
    u32 sp;                        /* 0x28 */
    u32 fp;                        /* 0x2C */
    u32 gp;                        /* 0x30 */
    u32 ret;                       /* 0x34 */
    u32 base;                      /* 0x38 */
};                                 /* 0x3C */

/* MEMCARD.C/INFOVIEW.C's PlayStation memory-card block header. */
typedef struct TCardHeader TCardHeader;
struct TCardHeader
{
    u8 Magic[2];                  /* 0x000 */
    u8 Type;                      /* 0x002 */
    u8 BlockEntry;                /* 0x003 */
    u8 Title[64];                 /* 0x004 */
    u8 reserve[28];               /* 0x044 */
    u8 Clut[32];                  /* 0x060 */
    u8 Icon[3][128];              /* 0x080 */
};                                /* 0x200 */

/* IMAGES.C's offset-table archive header. */
typedef struct ArcFile ArcFile;
struct ArcFile
{
    s16 count;                    /* 0x00 */
    s16 loaded;                   /* 0x02 */
    s32 entry[1];                 /* 0x04: offset before fixup, pointer after */
};                                /* 0x08 */

/* Retail's inventory-shop presentation and stock-limit record. */
typedef struct ShopItemDefault ShopItemDefault;
struct ShopItemDefault
{
    s16 x;                        /* 0x00: grid position */
    s16 y;                        /* 0x02 */
    s32 itemIndex;                /* 0x04 */
    u8 maxStock;                  /* 0x08 */
};                                /* 0x0C */

/* Retail's end-of-stage counters and calculated score components. Penalties
 * and totals are signed because they are negative before their clamps. */
typedef struct ScoreStats ScoreStats;
struct ScoreStats
{
    u8 stageBosses;               /* 0x00 */
    u8 stageEnemies;              /* 0x01 */
    u8 findEnemies;               /* 0x02 */
    u8 murders;                   /* 0x03 */
    u8 criticals;                 /* 0x04 */
    u8 friendHits;                /* 0x05 */
    s32 clock;                    /* 0x08 */
};                                /* 0x0C */

typedef struct ScoreResult ScoreResult;
struct ScoreResult
{
    u16 criticalScore;            /* 0x00 */
    u16 murderScore;              /* 0x02 */
    s16 friendPenalty;            /* 0x04 */
    s16 spottedScore;             /* 0x06 */
    s16 score;                    /* 0x08 */
    s16 grade;                    /* 0x0A */
};                                /* 0x0C */

/* CONFLICT.C's area-map cell. */
typedef struct AreaNodeType AreaNodeType;
struct AreaNodeType
{
    s16 y;                       /* 0x00 */
    s16 dy;                      /* 0x02 */
    s16 x1;                      /* 0x04 */
    s16 z1;                      /* 0x06 */
    s16 x2;                      /* 0x08 */
    s16 z2;                      /* 0x0A */
    s16 attribute;               /* 0x0C */
    s16 division;                /* 0x0E */
};                               /* 0x10 */

/* CONFLICT.C's area-map row index. */
typedef struct NodeIndexType NodeIndexType;
struct NodeIndexType
{
    s16 y;                       /* 0x00 */
    s16 n;                       /* 0x02 */
    s32 index;                   /* 0x04 */
    s16 x1;                      /* 0x08 */
    s16 z1;                      /* 0x0A */
    s16 x2;                      /* 0x0C */
    s16 z2;                      /* 0x0E */
};                               /* 0x10 */

/* LAYOUTENEMY.C's editable enemy placement. */
typedef struct TEnemyLayout TEnemyLayout;
struct TEnemyLayout
{
    s16 type;                    /* 0x00 */
    s16 ThinkType;               /* 0x02 */
    s16 nPath;                   /* 0x04 */
    s32 x;                       /* 0x08 */
    s32 y;                       /* 0x0C */
    s32 z;                       /* 0x10 */
    s16 r;                       /* 0x14 */
    s16 pad;                     /* 0x16 */
    VECTOR path[7];              /* 0x18 */
};                               /* 0x88 */

/* Area-map query result. PSX.SYM supplies the original first 16 bytes and
 * field names. Retail appends the last two cached pointers: GetAreaMapVector
 * writes them at +0x10/+0x14, and the corresponding eight-byte growth is
 * visible where Humanoid.locate moves from demo +0x30 to retail +0x38. */
typedef struct MapVector MapVector;
struct MapVector
{
    s32 level;                   /* 0x00 */
    s32 height;                  /* 0x04 */
    s16 attrib;                  /* 0x08 */
    s16 degree;                  /* 0x0A */
    u8 vector;                   /* 0x0C */
    u8 direct;                   /* 0x0D */
    u8 angleL;                   /* 0x0E */
    u8 angleH;                   /* 0x0F */
    struct AreaNodeType *area;   /* 0x10 (retail) */
    struct NodeIndexType *index; /* 0x14 (retail) */
};                               /* 0x18 */

/* Parent/child transform record embedded ahead of model and ornament data.
 * LoadModelArchive, LoadOrnamentArchive, and LoadConstruction all consume
 * the same PSX.SYM-defined table. */
typedef struct ParentingType ParentingType;
struct ParentingType
{
    s16 np;                      /* 0x00: parent number */
    s16 nc;                      /* 0x02: child number */
    s16 dx;                      /* 0x04 */
    s16 dy;                      /* 0x06 */
    s16 dz;                      /* 0x08 */
    u32 index;                   /* 0x0C */
};                               /* 0x10 */

/* WORLD.C/3DCTRL.C's shared model and ornament records. PSX.SYM supplies
 * each complete layout; these are used by items, characters, construction,
 * collision, effects, and the world object-slot manager. */
typedef struct ModelType ModelType;
struct ModelType
{
    GsCOORDINATE2 locate;         /* 0x00 */
    SVECTOR rotate;               /* 0x50 */
    s16 id;                       /* 0x58 */
    s16 attribute;                /* 0x5A */
    SVECTOR clip;                 /* 0x5C */
    GsDOBJ2 object;               /* 0x64 */
};                                /* 0x74 */

typedef struct ModelArchiveType ModelArchiveType;
struct ModelArchiveType
{
    GsCOORDINATE2 locate;         /* 0x00 */
    SVECTOR rotate;               /* 0x50 */
    s16 id;                       /* 0x58 */
    s16 attribute;                /* 0x5A */
    SVECTOR clip;                 /* 0x5C */
    s16 n;                        /* 0x64 */
    ModelType **object;           /* 0x68 */
};                                /* 0x6C */

typedef struct OrnamentType OrnamentType;
struct OrnamentType
{
    GsCOORDINATE2 locate;         /* 0x00 */
    GsDOBJ2 object;               /* 0x50 */
};                                /* 0x60 */

typedef struct OrnamentArchiveType OrnamentArchiveType;
struct OrnamentArchiveType
{
    GsCOORDINATE2 locate;         /* 0x00 */
    SVECTOR rotate;               /* 0x50 */
    s16 id;                       /* 0x58 */
    s16 attribute;                /* 0x5A */
    s16 n;                        /* 0x5C */
    OrnamentType **object;        /* 0x60 */
    u_long *data;                 /* 0x64 */
};                                /* 0x68 */

typedef struct tag_ObjectSlotType ObjectSlotType;
struct tag_ObjectSlotType
{
    ObjectSlotType *next;         /* 0x00 */
    OrnamentType *model;          /* 0x04 */
    s16 ModelSize;                /* 0x08 */
    s16 ShiftY;                   /* 0x0A */
};                                /* 0x0C */

typedef struct ObjectSlotManager ObjectSlotManager;
struct ObjectSlotManager
{
    ObjectSlotType *slot;         /* 0x00 */
    s32 n;                        /* 0x04 */
    s32 max;                      /* 0x08 */
};                                /* 0x0C */

typedef struct WorldType WorldType;
struct WorldType
{
    ObjectSlotType *top;          /* 0x00 */
};                                /* 0x04 */

/* CONFLICT.C's collision slot. PSX.SYM records result[64] and size 0x68 in
 * the demo. Retail raises the slot limit to 80 (InsertConflict), clears 0x50
 * result bytes, and reserves 0x2580 bytes for 80 slots, proving that the old
 * result array grew to 80 rather than gaining sixteen bytes of padding. */
typedef struct ConflictObjectType ConflictObjectType;
struct ConflictObjectType
{
    struct ModelType *model;     /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[80];               /* 0x28 */
};                               /* 0x78 */

/* 3DCTRL.C's textured sprite model. The demo PSX.SYM supplies the complete
 * layout and original member names; the retail users confirm the same 0x8C
 * size and offsets. */
typedef struct Sprite3D Sprite3D;
struct Sprite3D
{
    GsCOORDINATE2 locate;         /* 0x00 */
    SVECTOR rotate;               /* 0x50 */
    s16 id;                       /* 0x58 */
    s16 attribute;                /* 0x5A */
    SVECTOR clip;                 /* 0x5C */
    s32 scale;                    /* 0x64 */
    GsSPRITE sprite;              /* 0x68 */
};                                /* 0x8C */

/* 3DCTRL.C's tiled background. PSX.SYM supplies the complete layout and
 * original `hundle` spelling; retail confirms the same 0x48-byte record. */
typedef struct BackGround BackGround;
struct BackGround
{
    GsBG hundle;                  /* 0x00 */
    GsMAP map;                    /* 0x24 */
    GsCELL *cell;                 /* 0x34 */
    u32 *work;                    /* 0x38 */
    u16 *index;                   /* 0x3C */
    u16 sz;                       /* 0x40 */
    s16 id;                       /* 0x42 */
    s16 attribute;                /* 0x44 */
};                                /* 0x48 */

/* CHRANIM.C's character-animation event record. */
typedef struct CVAType CVAType;
struct CVAType
{
    s16 mode;                     /* 0x00 */
    s16 id;                       /* 0x02 */
    s16 x;                        /* 0x04 */
    s16 y;                        /* 0x06 */
    s16 z;                        /* 0x08 */
    s16 p;                        /* 0x0A */
};                                /* 0x0C */

/* CHRANIM.C's queued character-motion slot. */
typedef struct HumanAnimType HumanAnimType;
struct HumanAnimType
{
    struct Humanoid *human;       /* 0x00 */
    s16 loop;                     /* 0x04 */
    s16 motid;                    /* 0x06 */
};                                /* 0x08 */

/* CAMERA.C's camera modes. */
typedef enum TCameraMode TCameraMode;
enum TCameraMode
{
    CMODE_NORMAL = 0,
    CMODE_DIRECTION = 1,
    CMODE_SYSPARAM = 2,
    CMODE_SIGHT = 3,
    CMODE_CRITICAL_HIT = 4,
    CMODE_FIGHT = 5,
    CMODE_STICK_L = 6,
    CMODE_STICK_R = 7,
    CMODE_SWIM = 8,
    CMODE_PEEP_L = 9,
    CMODE_PEEP_R = 10,
    CMODE_CROUCH = 11,
    CMODE_RUN = 12,
    CMODE_LOCK = 13,
    CMODE_FALL = 14
};

/* CAMERA.C's global camera state. Retail rearranges the demo PSX.SYM
 * record: DirectionRX/DirectionRY move ahead of OldMode, and OldMode becomes
 * a byte beside the new CriticalHit flag. The resulting retail record is
 * 0x20 bytes; the demo's Valiation member at +0x20 is not part of it. */
typedef struct TCameraStatus TCameraStatus;
struct TCameraStatus
{
    VECTOR TargetVector;          /* 0x00 */
    struct Humanoid *Owner;       /* 0x10 */
    TCameraMode Mode;             /* 0x14 */
    s16 DirectionRX;              /* 0x18 */
    s16 DirectionRY;              /* 0x1A */
    u8 OldMode;                   /* 0x1C */
    u8 CriticalHit;               /* 0x1D */
};                                /* 0x20 */

/* Retail's four-vector camera preset (CAMERA_R1/R2/P1/P2). */
typedef struct CameraVectors CameraVectors;
struct CameraVectors
{
    SVECTOR r1;                    /* 0x00 */
    SVECTOR r2;                    /* 0x08 */
    SVECTOR p1;                    /* 0x10 */
    SVECTOR p2;                    /* 0x18 */
};                                 /* 0x20 */

/* CDPLAYER.C's playback state. Retail keeps the demo's original members but
 * rearranges the tail, adds the left/right volume bytes, and appends the
 * pending drive command. */
typedef struct TCdaStatus TCdaStatus;
struct TCdaStatus
{
    s32 StartPos;                 /* 0x00 */
    s32 CurPos;                   /* 0x04 */
    s32 EndPos;                   /* 0x08 */
    s16 mode;                     /* 0x0C */
    s16 CheckCount;               /* 0x0E */
    u8 status;                    /* 0x10 */
    u8 voll;                      /* 0x11 */
    u8 volr;                      /* 0x12 */
    u8 flag;                      /* 0x13 */
    u8 command;                   /* 0x14 */
};                                /* 0x18 */

/* CAMERA.C's smoothing history. */
typedef struct TMakeDifInfo TMakeDifInfo;
struct TMakeDifInfo
{
    s16 div;                      /* 0x00 */
    s16 spd;                      /* 0x02 */
    SVECTOR bef;                  /* 0x04 */
};                                /* 0x0C */

/* EVENT.C's stage-event descriptor. */
typedef struct EventSeqType EventSeqType;
struct EventSeqType
{
    u8 id;                        /* 0x00 */
    u8 event;                     /* 0x01 */
    u8 next1;                     /* 0x02 */
    u8 next2;                     /* 0x03 */
    u8 target;                    /* 0x04 */
    u8 mode;                      /* 0x05 */
    s16 status;                   /* 0x06 */
    s16 x[2];                     /* 0x08 */
    s16 y[2];                     /* 0x0C */
    s16 z[2];                     /* 0x10 */
};                                /* 0x14 */

/* APPEAR.C's weapon placement vectors. */
typedef struct WeaponType WeaponType;
struct WeaponType
{
    SVECTOR confp;                 /* 0x00 */
    SVECTOR ilup0;                 /* 0x08 */
    SVECTOR ilup1;                 /* 0x10 */
};                                 /* 0x18 */

/* APPEAR.C's weapon-model database row. */
typedef struct WeaponModelType WeaponModelType;
struct WeaponModelType
{
    u8 *name;                       /* 0x00 */
    s16 wid;                        /* 0x04 */
    u_long *model;                  /* 0x08 */
};                                  /* 0x0C */

/* APPEAR.C's character database row. */
struct MotionRegistType;
typedef struct HumanDataType HumanDataType;
struct HumanDataType
{
    s16 type;                       /* 0x00 */
    s16 wepid;                      /* 0x02 */
    s16 turn;                       /* 0x04 */
    s16 life;                       /* 0x06 */
    s16 width;                      /* 0x08 */
    s16 height;                     /* 0x0A */
    struct MotionRegistType *mtbl;  /* 0x0C */
    u8 *name;                       /* 0x10 */
    u_long *model;                  /* 0x14 */
};                                  /* 0x18 */

/* STAGE.C's per-stage character placement. */
typedef struct StageCharType StageCharType;
struct StageCharType
{
    s16 stage;                       /* 0x00 */
    s16 chrid;                       /* 0x02 */
    SVECTOR position;                /* 0x04 */
    s16 think;                       /* 0x0C */
};                                   /* 0x0E */

/* STAGE.C's stage metadata and starting transform. */
typedef struct TStageConfig TStageConfig;
struct TStageConfig
{
    u8 uid;                           /* 0x00 */
    u8 *name;                         /* 0x04 */
    u8 *path;                         /* 0x08 */
    s32 px;                           /* 0x0C */
    s32 py;                           /* 0x10 */
    s32 pz;                           /* 0x14 */
    s32 pr;                           /* 0x18 */
};                                    /* 0x1C */

/* AUDIO.C's loaded VAB handle. */
struct VabHdr;
typedef struct SoundEffect SoundEffect;
struct SoundEffect
{
    s16 VABid;                        /* 0x00 */
    s16 program;                      /* 0x02 */
    struct VabHdr *VABhead;           /* 0x04 */
};                                    /* 0x08 */

/* INFOVIEW.C's active life-bar slot (anonymous in PSX.SYM). */
typedef struct LifeBarEntry LifeBarEntry;
struct LifeBarEntry
{
    struct Humanoid *target;           /* 0x00 */
    s32 life;                          /* 0x04 */
    s32 max;                           /* 0x08 */
    s32 count;                         /* 0x0C */
    s32 style;                         /* 0x10 */
};                                     /* 0x14 */

/* Retail's redesigned INFOVIEW.C life-bar style. */
typedef struct TLifeBarStyle TLifeBarStyle;
struct TLifeBarStyle
{
    u16 base;                           /* 0x00 */
    s16 scale;                          /* 0x02 */
    s16 dx;                             /* 0x04 */
    s16 dy;                             /* 0x06 */
    GsSPRITE frame;                     /* 0x08 */
    GsSPRITE fill;                      /* 0x2C */
};                                      /* 0x50 */

/* HUMAN.C/WORLD.C's waypoint path. */
typedef struct TracePoint TracePoint;
struct TracePoint
{
    s32 x;                            /* 0x00 */
    s32 z;                            /* 0x04 */
    s16 range;                        /* 0x08 */
    s16 pad;                          /* 0x0A */
};                                    /* 0x0C */

typedef struct TraceLine TraceLine;
struct TraceLine
{
    s16 index;                        /* 0x00 */
    s16 count;                        /* 0x02 */
    TracePoint *point;                /* 0x04 */
};                                    /* 0x08 */

/* EFFECT.C's draw-mode-plus-flat-quad primitive. */
typedef struct POLY_XF4 POLY_XF4;
struct POLY_XF4
{
    DR_TPAGE tpage;                    /* 0x00 */
    POLY_F4 ply;                       /* 0x08 */
};                                     /* 0x20 */

typedef enum weapon_kind weapon_kind;

enum weapon_kind
{
    NO_WEAPON = 0x00,
    KUMA_WEAPON = 0x01,
    NINJA_0_1_WEAPON = 0x02,
    RAT_CAT_DOG_WEAPON = 0x03,
    KODATI = 0x04,
    JYUTE = 0x05,
    JYUTEB = 0x06,
    EN = 0x07,
    ENB = 0x08,
    ANDON = 0x09,
    JYURUR = 0x0a,
    JYURUL = 0x0b,
    KOZUKA = 0x0c,
    IKARI = 0x10,
    BOU = 0x11,
    SABRE = 0x12,
    NINJA = 0x13,
    KEITOU = 0x14,
    KEITOUB = 0x15,
    KATANA_0 = 0x16,
    SAYA_0 = 0x17,
    TUKAH_0 = 0x18,
    HOUTOU = 0x19,
    SAYA_1 = 0x1a,
    TUKAH_1 = 0x1b,
    KATANA_1 = 0x1c,
    SAYAN = 0x1d,
    TUKAN = 0x1e,
    KATANA_2 = 0x1f,
    CROWR = 0x20,
    CROWL = 0x21,
    YARI = 0x22,
    KABUTUTI = 0x23,
    SASUMATA = 0x24,
    HALBERT = 0x25,
    KON = 0x26,
    NAGI = 0x27,
    ENGETU = 0x28,
    SEVEN = 0x29,
    KATANAL = 0x2a,
    SAYAL = 0x2b,
    TUKAL = 0x2c,
    TUKAANI = 0x2d,
    TEPPO = 0x30,
    GUN = 0x31,
    YUMI = 0x32,
    YAB_0 = 0x33,
    YAZUTU_0 = 0x34,
    KATAYUMI = 0x35,
    YAB_1 = 0x36,
    YAZUTU_1 = 0x37,
    END_OF_WEAPON_KIND_MARKER = 0xffff,
};

typedef enum character_kind character_kind;
enum character_kind
{
    RIKIMARU_0 = 0x00,
    AYAME_0 = 0x01,
    RIKIMARU_1 = 0x02,
    AYAME_1 = 0x03,
    ROUJYU = 0x04,
    HIME = 0x05,
    TONO = 0x06,
    KERAI_KATANA = 0x07,
    KERAI_YARI = 0x08,
    KERAI_YUMI = 0x09,
    ROUNIN_KATANA = 0x10,
    ROUNIN_1YARI = 0x11,
    ROUNIN_YUMI = 0x12,
    ROUBAN_KATANA = 0x13,
    ROUBAN_SASUMATA = 0x14,
    ROUBAN_YUMI = 0x15,
    ASIGARU_KATANA = 0x16,
    ASIGARU_YARI = 0x17,
    ASIGARU_YUMI = 0x18,
    SISI_KATANA = 0x19,
    SISI_YARI = 0x1a,
    SISI_YUMI = 0x1b,
    NINJAA = 0x20,
    NINJAB = 0x21,
    KUNOITI = 0x22,
    MANJI = 0x30,
    MANJI5_KEITOU = 0x31,
    MANJI5_ENGETU = 0x32,
    MANJI5_YUMI = 0x33,
    PIRATEA_HALBERT = 0x40,
    PIRATEA_TEPPO = 0x41,
    PIRATEB = 0x42,
    TENGU_JYUTE = 0x50,
    TENGU_KON = 0x51,
    TENGU_YUMI = 0x52,
    KIMEN13 = 0x60,
    ONIKERAI = 0x61,
    ONIKUNO_EN = 0x62,
    ONIKUNO_CROWR = 0x63,
    KABANE_HOUTOU = 0x70,
    KABANE_KABUTUTI = 0x71,
    KABANE_YUMI = 0x72,
    MOURYO_0 = 0x73,
    MOURYO_1 = 0x74,
    ECHIGOYA = 0x80,
    HANBE = 0x81,
    TUZI = 0x82,
    GOO = 0x83,
    ON = 0x84,
    BALMA = 0x85,
    KUMA_0 = 0x86,
    NINJA_0 = 0x87,
    MEIOU = 0x88,
    KUMA_1 = 0x89,
    NINJA_1 = 0x8a,
    ANI = 0x8b,
    TAZ = 0x8c,
    HIKONE = 0x8d,
    KATAOKA_KATAYUMI = 0x8e,
    KATAOKA_KOZUKA = 0x8f,
    CHONIN = 0x90,
    JOCHU = 0x91,
    MUSUME = 0x92,
    ZAININ = 0x93,
    BIZENYA = 0x94,
    NAKAI = 0x95,
    MEKAKE = 0x96,
    RAT = 0xa0,
    CAT = 0xa1,
    DOG_0 = 0xa2,
    DOG_1 = 0xa3,
    WOLF = 0xa4,
    FIREDOG = 0xa5,
    S1 = 0xa6,
    S2 = 0xa7,
    ARROW = 0xa8,
    NINKEN = 0xa9,
    END_OF_CHARACTER_KIND_MARKER = 0xffff,
};

typedef enum character_status character_status;
enum character_status
{
    DEFAULT_STATE = 0x00,
    POSING_OR_PLAYING_SOME_CUTSCENE_ANIMATION_THING = 0x01,
    SLOW_WALKING = 0x02,
    SWIMMING = 0x03,
    AIMING_KAGINAWA = 0x04,
    IDLING = 0x05,
    MOVING = 0x06,
    ATTACKING = 0x07,
    FALLING_OR_PICKING_OR_DROPPING_ITEM = 0x08,
    JUMPING = 0x09,
    HANGING = 0x0a,
    CROUCHING = 0x0b,
    PRESSED_AGAINST_WALL = 0x0c,
    AIMING_WEAPON = 0x0e,
    USING_ITEM = 0x0f,
    RECOVERY_ANIMATION = 0x10,
    DEAD = 0x11,
};

typedef enum stage_rank stage_rank;
enum stage_rank
{
    RANK_THUG = 0x00,
    RANK_NOVICE = 0x01,
    RANK_NINJA = 0x02,
    RANK_MASTER_NINJA = 0x03,
    RANK_GRAND_MASTER = 0x04,
};

/* Difficulty: the persistent-state byte at 0x80010058 (symbol gNannido).
 * Official name from the demo PSX.SYM: WORLD.C's cross-exe config struct
 * (typedef TLinkInfo) carries `unsigned char Nannido` at +0x5 and the demo
 * exe keeps a standalone `unsigned char gNannido` (0x80098090); "nannido"
 * is Japanese for difficulty. Retail moved it into the 0x80010000 blob;
 * the neighbouring bytes keep TLinkInfo's field run (0x5A SoundLevel ->
 * CD volume in FUN_8004f68c, 0x5B SELevel -> PlaySE scale, 0x5D Anakon ->
 * PadShock gate). The demo has NO enum for the values -- the original does
 * arithmetic on the raw byte (`EngageLevel = 3 - pt[0x58]` in
 * SetupAppearance, `rand() % 4 - 2 >= gNannido` in StateTransition) -- so
 * these member names are ours. 0=easy proven by FileOption pairing
 * gNannido=0 with EngageLevel=3 (heaviest attack veto, no NPC auto-guard)
 * and gNannido=2 with EngageLevel=1 (no veto, densest attack cadence,
 * 600-frame alerts). */
typedef enum game_difficulty game_difficulty;
enum game_difficulty
{
    DIFFICULTY_EASY = 0x00,
    DIFFICULTY_NORMAL = 0x01,
    DIFFICULTY_HARD = 0x02,
};

/* ITEM.C's item-kind enum, using the original PSX.SYM name and labels with
 * the retail ordering.  Retail swaps goshikimai/nemuri; its inserted ARMOUR
 * shifts TELEPORT to 24, replacing the demo's SYSFLAG at that value. */
typedef enum TItemType TItemType;
enum TItemType
{
    ITEM_KAGINAWA = 0x00,
    ITEM_SHURIKEN = 0x01,
    ITEM_MAKIBISHI = 0x02,
    ITEM_KUSURI = 0x03,
    ITEM_FIRE = 0x04,
    ITEM_SMOKE = 0x05,
    ITEM_JIRAI = 0x06,
    ITEM_DOKUDANGO = 0x07,
    ITEM_GOSHIKIMAI = 0x08,
    ITEM_NEMURI = 0x09,
    ITEM_KAWARIMI = 0x0a,
    ITEM_HENSHIN = 0x0b,
    ITEM_GOSIN = 0x0c,
    ITEM_SHINSOKU = 0x0d,
    ITEM_NINGYO = 0x0e,
    ITEM_HAPPOU = 0x0f,
    ITEM_NINKEN = 0x10,
    ITEM_KAENGEKI = 0x11,
    ITEM_MANEBUE = 0x12,
    ITEM_ARMOUR = 0x13,
    ITEM_GUN = 0x14,
    ITEM_ARROW = 0x15,
    ITEM_NAPALM = 0x16,
    ITEM_LIGHTNINGBOLT = 0x17,
    ITEM_TELEPORT = 0x18,
    ITEM_N = 0x19,
};
// s32 AdtSelect(char *screen_header, TAdtSelect *choices, char *param_3);

// The persistent game state blob at 0x80010000 (below the exe image; survives
// across screens). TLinkInfo is the OFFICIAL typedef from the demo PSX.SYM
// (WORLD.C's cross-exe config struct — it "links" state between the separate
// executables); the full demo definition is reference/psxsym-types.h (216
// bytes, CharType short at +0, options at +0x5..+0xE). Retail rearranged and
// extended the layout (0xE70 memset by InitPersistentState, magic 0x19981110
// at +0, options run moved to +0x58, language/shop stock added), so the demo
// definition CANNOT be adopted verbatim — byte-identity pins the retail
// offsets below. Field naming: TitleCase = official demo field name carried
// over (correspondence proven by retail consumers); lowercase = repo-named,
// no demo counterpart proven (candidates: counts ~ selItem?, backup ~
// saveItem? — demo arrays are [30], retail [0x14], unverified).
// The 0x80010000 instance keeps splat's descriptive symbol name
// PersistentState — the demo names only the type, not the retail instance.
// Offsets proven by BriefingAndInventorySelectionScreen.
// Splat also names some fields as standalone globals (CHOSEN_CHARACTER = +4,
// CHOSEN_STAGE = +5, STAGE_LAYOUT_NUMBER = +6, CHOSEN_LANGUAGE = +0x5E,
// SHOP_STOCK_STATE_BY_CHAR = +0x40C); the original source mixed direct global
// accesses with pointer-based ones, so both views coexist on purpose.
typedef struct
{
    u8 field_0x0[4];        /* 0x000 magic 0x19981110 (InitPersistentState) */
    u8 CharType;            /* 0x004 CHOSEN_CHARACTER (stock matrix row;
                             *       demo +0x0, short) */
    u8 StageNo;             /* 0x005 CHOSEN_STAGE (demo +0x2) */
    u8 layout;              /* 0x006 STAGE_LAYOUT_NUMBER */
    u8 counts[0x14];        /* 0x007 selected count per item 0..0x13;
                             *       [0x12]/[0x13] double as flags */
    u8 field_0x1b[0xC];     /* 0x01B */
    u8 backup[0x14];        /* 0x027 loadout backup (restore on abort) */
    u8 field_0x3b[0xD];     /* 0x03B */
    u8 flags48;             /* 0x048 bit0: item screen already initialised */
    u8 field_0x49[0xF];     /* 0x049 */
    u8 Nannido;             /* 0x058 gNannido: game_difficulty (demo +0x5) */
    u8 Stereo;              /* 0x059 gSound: 1 = stereo, 0 = mono
                             *       (InitSoundEffect/InitPersistentState
                             *       -> SsSetStereo/SsSetMono; demo +0x7) */
    u8 SoundLevel;          /* 0x05A gSoundLevel: music/CD volume 0..0x7F
                             *       (FUN_8004f68c, _PlayMusic; demo +0x8) */
    u8 SELevel;             /* 0x05B gSELevel: SE volume 0..0x7F
                             *       (PlaySE, PlayVoice; demo +0x9) */
    u8 fMemory;             /* 0x05C gfMemory: post-mission memory-card
                             *       save flow enabled (StageEndScreen /
                             *       mission_score_screen -> FUN_800514d8
                             *       save UI; demo +0xC) */
    u8 Anakon;              /* 0x05D analog pad / rumble enabled (PadShock
                             *       gate, PadProc; demo +0xE; default 1) */
    u8 language;            /* 0x05E CHOSEN_LANGUAGE (retail-only) */
    u8 field_0x5f[0x3AD];   /* 0x05F */
    u8 stock[0x100];        /* 0x40C SHOP_STOCK_STATE_BY_CHAR[chr*0x20+item];
                             *       0xFE = locked, 0xFF = infinite;
                             *       [chr*0x20+0x13] = stage bonus item flag */
} TLinkInfo;
