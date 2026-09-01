// Game data types for Tenchu main.exe — the canonical, build-verified type
// model (structs/enums/typedefs). Included by main.exe.h AFTER the PSY-Q
// SDK header and the base-int typedefs, so it may use GsIMAGE/VECTOR/u16/etc.

#include <psxsdk/libcd.h>
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

/* libpad port ids encode the physical port in their high nibble and the
 * multitap slot in their low two bits. */
#define PAD_PORT_COUNT 2
#define PAD_SLOTS_PER_PORT 4
#define PAD_PORT_INDEX_SHIFT 4
#define PAD_SLOT_INDEX_MASK (PAD_SLOTS_PER_PORT - 1)

enum pad_controller_index
{
    PAD_CONTROLLER_1 = 0,
    PAD_CONTROLLER_2 = 1
};

enum pad_port_id
{
    PAD_PORT_1 = 0x00,
    PAD_PORT_2 = 0x10
};

/* PadInitDirect fills one 34-byte report per physical port.  A multitap
 * report has a two-byte header followed by four eight-byte slot reports. */
enum pad_report_format
{
    PAD_REPORT_STATUS = 0,
    PAD_REPORT_ID = 1,
    PAD_REPORT_BUTTON_HIGH = 2,
    PAD_REPORT_BUTTON_LOW = 3,
    PAD_REPORT_HEADER_SIZE = 2,
    PAD_SLOT_REPORT_SIZE = 8,
    PAD_REPORT_BUFFER_SIZE =
        PAD_REPORT_HEADER_SIZE + PAD_SLOTS_PER_PORT * PAD_SLOT_REPORT_SIZE,
    PAD_REPORT_TYPE_SHIFT = 4,
    PAD_REPORT_STATUS_OK = 0
};

enum pad_report_type
{
    PAD_REPORT_TYPE_ANALOG = 7,
    PAD_REPORT_TYPE_MULTITAP = 8
};

#define PAD_REPORT_TYPE(report) \
    ((report)[PAD_REPORT_ID] >> PAD_REPORT_TYPE_SHIFT)

/* Names from libpad's PadInfoMode and PadGetState contracts. */
enum pad_info_mode
{
    PAD_INFO_CURRENT_ID = 1,
    PAD_INFO_CURRENT_EXTENDED_ID = 2,
    PAD_INFO_CURRENT_OFFSET = 3,
    PAD_INFO_ID_TABLE = 4,
    PAD_INFO_EXTENDED = 5
};

enum pad_connection_state
{
    PAD_STATE_DISCONNECTED = 0,
    PAD_STATE_FIND_PAD = 1,
    PAD_STATE_FIND_CTP1 = 2,
    PAD_STATE_FIND_CTP2 = 3,
    PAD_STATE_REQUEST_INFO = 4,
    PAD_STATE_EXECUTE_COMMAND = 5,
    PAD_STATE_STABLE = 6,
    PAD_STATE_ERROR = 7
};

enum pad_main_mode
{
    PAD_MAIN_MODE_DIGITAL = 0,
    PAD_MAIN_MODE_ANALOG = 1
};

enum pad_main_mode_lock
{
    PAD_MAIN_MODE_KEEP_LOCK = 0,
    PAD_MAIN_MODE_UNLOCK = 2,
    PAD_MAIN_MODE_LOCK = 3
};

enum pad_protocol_constant
{
    PAD_ACTUATOR_COUNT = 2,
    PAD_DIGITAL_AXIS_MAGNITUDE = 45,
    PAD_ANALOG_MODE_SWITCH_DELAY = 15
};

/* Button subsets shared by player and AI-pad code.  The signed spelling is
 * intentional: a few original think handlers materialize 0xffffa000 rather
 * than using an immediate AND with the equivalent unsigned 0xa000. */
#define PAD_DIRECTION_BUTTONS \
    (PADLleft | PADLdown | PADLright | PADLup)
#define PAD_TURN_BUTTONS (PADLleft | PADLright)
#define PAD_TURN_BUTTONS_SIGNED ((s16)PAD_TURN_BUTTONS)

// One controller port's raw state — the official globals and PADCMD.C name
// this TPadPort (reference/psxsym-globals.h: `struct TPadPort PadPort[2][4]`).
// Retail inserted `active` at offset 6, making it 14 bytes vs the demo's 12
// (ComPad.c documents this). GetRealPad indexes the [port][slot] table and
// reads `button`.
typedef struct TPadPort TPadPort;
struct TPadPort
{
    u16 button;   /* 0x0 (held buttons) */
    s16 x;        /* 0x2 */
    s16 y;        /* 0x4 */
    u8 active;    /* 0x6 (retail-inserted) */
    u8 fAnalog;   /* 0x7 */
    u8 act1;      /* 0x8 */
    u8 act2;      /* 0x9 */
    u8 actbuf[PAD_ACTUATOR_COUNT]; /* 0xA */
    u8 Send;      /* 0xC */
};

/* PADCMD.C's rumble attack/release envelope (anonymous in PSX.SYM). */
typedef struct PadArrangeType PadArrangeType;
struct PadArrangeType
{
    s32 pow;     /* 0x00 */
    s32 time;    /* 0x04 */
    s32 attack;  /* 0x08 */
    s32 release; /* 0x0C */
}; /* 0x10 */

/* PADCMD.C's command table. COMMAND is the recovered unsigned storage word;
 * pad_command is the signed runtime id returned through dtCMD. Retail has
 * thirteen sequences plus the null table terminator (the demo had fewer). */
typedef unsigned short COMMAND;
typedef s16 pad_command;

enum pad_command_value
{
    CMD_NONE = 0,
    CMD_DASH_FORWARD = 0x01,
    CMD_DASH_BACKWARD = 0x02,
    CMD_DASH_LEFT = 0x03,
    CMD_DASH_RIGHT = 0x04,
    CMD_ROLL_FORWARD = 0x11,
    CMD_ROLL_BACKWARD = 0x12,
    CMD_ROLL_LEFT = 0x13,
    CMD_ROLL_RIGHT = 0x14,
    CMD_LUNGE = 0x21,
    CMD_LUNGE_BACK = 0x22,
    CMD_FLIP = 0x31
};

enum
{
    PAD_COMMAND_END = 0xFFFF,
    PAD_COMMAND_STREAM_LENGTH = 4,
    N_PAD_COMMAND_SEQUENCES = 13,
    N_PAD_COMMAND_TABLE_ENTRIES = N_PAD_COMMAND_SEQUENCES + 1
};

/* Command state embedded in each Humanoid. */
typedef struct PADtype PADtype;
struct PADtype
{
    u16 data;      /* 0x00 */
    u16 sdata;     /* 0x02 */
    u16 trig;      /* 0x04 */
    s16 time;      /* 0x06 */
    u16 stream[PAD_COMMAND_STREAM_LENGTH]; /* 0x08 */
}; /* 0x10 */

/* AdtSelect's menu row — the demo's own debug symbols supply this name and
 * the unsigned label-pointer type (the stack-variable records in
 * FileOption/DoInfoViewProc/etc. call these `struct TAdtSelect [N]`). */
typedef struct TAdtSelect TAdtSelect;
struct TAdtSelect
{
    u8 *name;     /* 0x0 */
    u_long value; /* 0x4 */
};

/* Menu rows use this value for their explicit cancel entry. */
#define ADT_SELECT_CANCEL (-1)

/* ADT's original quiet-state names, recovered from the demo's PSX.SYM. */
typedef enum AdtQuietMode AdtQuietMode;
enum AdtQuietMode
{
    ADT_NORMAL = 0,
    ADT_QUIET = 1
};

/* ADT's saved PSY-Q font settings. */
typedef struct AdtFntState AdtFntState;
struct AdtFntState
{
    s32 x;              /* 0x00 */
    s32 y;              /* 0x04 */
    s32 w;              /* 0x08 */
    s32 h;              /* 0x0C */
    s32 isbg;           /* 0x10 */
    s32 n;              /* 0x14 */
    s32 tx;             /* 0x18 */
    s32 ty;             /* 0x1C */
    AdtQuietMode quiet; /* 0x20 */
}; /* 0x24 */

/* PSY-Q executable header, recovered verbatim in the demo's PSX.SYM. */
typedef struct EXEC EXEC;
struct EXEC
{
    u32 pc0;    /* 0x00 */
    u32 gp0;    /* 0x04 */
    u32 t_addr; /* 0x08 */
    u32 t_size; /* 0x0C */
    u32 d_addr; /* 0x10 */
    u32 d_size; /* 0x14 */
    u32 b_addr; /* 0x18 */
    u32 b_size; /* 0x1C */
    u32 s_addr; /* 0x20 */
    u32 s_size; /* 0x24 */
    u32 sp;     /* 0x28 */
    u32 fp;     /* 0x2C */
    u32 gp;     /* 0x30 */
    u32 ret;    /* 0x34 */
    u32 base;   /* 0x38 */
}; /* 0x3C */

enum
{
    CARD_ICON_TYPE_FLAG = 0x10,
    CARD_ICON_FRAME_COUNT = 3,
    CARD_ICON_CLUT_COLORS = 16,
    CARD_ICON_BITMAP_SIZE = 128
};

/* TCardHeader.Type combines the icon flag with its animation-frame count. */
enum card_icon_display
{
    SAVE_ICON_1_FRAME = CARD_ICON_TYPE_FLAG | 1,
    SAVE_ICON_2_FRAMES = CARD_ICON_TYPE_FLAG | 2,
    SAVE_ICON_3_FRAMES = CARD_ICON_TYPE_FLAG | CARD_ICON_FRAME_COUNT
};

typedef struct TCardHeader TCardHeader;
struct TCardHeader
{
    u8 Magic[2];     /* 0x000 */
    u8 Type;         /* 0x002 */
    u8 BlockEntry;   /* 0x003 */
    u8 Title[64];    /* 0x004 */
    u8 reserve[28];  /* 0x044 */
    u8 Clut[CARD_ICON_CLUT_COLORS * sizeof(u16)]; /* 0x060 */
    u8 Icon[CARD_ICON_FRAME_COUNT][CARD_ICON_BITMAP_SIZE]; /* 0x080 */
}; /* 0x200 */

typedef union ArcEntry ArcEntry;
union ArcEntry
{
    s32 offset;   /* relative to ArcFile.entry before relocation */
    u_long *data; /* absolute resource pointer after relocation */
}; /* 0x04 */

/* ArcFile.loaded is the tag for every ArcEntry union in the archive. */
typedef s16 arc_relocation_state;
enum arc_relocation_state
{
    ARC_ENTRIES_RELATIVE = 0,
    ARC_ENTRIES_ABSOLUTE = 1
};

/* IMAGES.C's relocatable offset-table archive header. */
typedef struct ArcFile ArcFile;
struct ArcFile
{
    s16 count;         /* 0x00 */
    arc_relocation_state loaded; /* 0x02: selects ArcEntry.offset/data */
    ArcEntry entry[1]; /* 0x04 */
}; /* 0x08 */

#define ARC_ENTRY_TABLE_OFFSET ((s32)&((ArcFile *)0)->entry)

/* Retail's inventory-shop presentation and stock-limit record. */
typedef struct ShopItemDefault ShopItemDefault;
struct ShopItemDefault
{
    s16 x;         /* 0x00: grid position */
    s16 y;         /* 0x02 */
    s32 itemIndex; /* 0x04 */
    u8 maxStock;   /* 0x08 */
}; /* 0x0C */

/* Retail's end-of-stage counters and calculated score components. Penalties
 * and totals are signed because they are negative before their clamps. */
typedef struct ScoreStats ScoreStats;
struct ScoreStats
{
    u8 stageBosses;  /* 0x00 */
    u8 stageEnemies; /* 0x01 */
    u8 findEnemies;  /* 0x02 */
    u8 murders;      /* 0x03 */
    u8 criticals;    /* 0x04 */
    u8 friendHits;   /* 0x05 */
    s32 clock;       /* 0x08 */
}; /* 0x0C */

typedef struct ScoreResult ScoreResult;
/* ScoreResult stores the named stage-rank domain in a signed halfword. */
typedef s16 stage_rank;
typedef u8 compact_stage_rank;
typedef s16 stage_award_tier;
struct ScoreResult
{
    u16 criticalScore; /* 0x00 */
    u16 murderScore;   /* 0x02 */
    s16 friendPenalty; /* 0x04 */
    s16 spottedScore;  /* 0x06 */
    s16 score;         /* 0x08 */
    stage_rank grade;  /* 0x0A */
}; /* 0x0C */

/* CONFLICT.C's raw area-map word type, recovered from PSX.SYM. */
typedef unsigned long AreaMapType;

/* Terrain attributes are combined 16-bit flags, not an enum-typed field.
 * The compiler stores enums as four bytes, while the recovered nodes and
 * query result both carry a signed short. */
typedef s16 MapAttribute;

/* AreaNodeType.division is a 4x4 cell-enable mask. -1 enables all cells. */
typedef s16 AreaDivisionMask;
#define AREA_DIVISION_ALL (-1)

/* Each signed grid cell selects a node subset; -1 means no subset. */
typedef s16 area_node_index;
#define AREA_NODE_INDEX_NONE (-1)

/* CONFLICT.C's area-map cell. */
typedef struct AreaNodeType AreaNodeType;
struct AreaNodeType
{
    s16 y;         /* 0x00 */
    s16 dy;        /* 0x02 */
    s16 x1;        /* 0x04 */
    s16 z1;        /* 0x06 */
    s16 x2;        /* 0x08 */
    s16 z2;        /* 0x0A */
    MapAttribute attribute; /* 0x0C */
    AreaDivisionMask division; /* 0x0E */
}; /* 0x10 */

/* Area/terrain attribute bits — shared vocabulary of
 * AreaMapNodeType.attribute, the FieldAttrib mirror, and each character's
 * MapVector.attrib copy:
 *   MAP_WATER   0x0004 — water surface (SwimCheck's gate; the grapple and
 *                        damage checks test it)
 *   MAP_DEATH   0x0200 — kill floor: standing on it at height 0 forces the
 *                        MOT_DEAD motion (DefaultActionHumanoid)
 *   MAP_SLOPE_X 0x4000 / MAP_SLOPE_Z 0x8000 — the node's dy interpolates
 *                        along x resp. z (camera_terrain_pitch_;
 *                        StickonCheck rejects wall-stick on slopes)
 *   MAP_DAMAGE  0x0100 — damaging floor: gates the periodic
 *                        spawn_damage_effect_ tick (DrawShadow,
 *                        spread_blood_pool_, StateTransition)
 *   MAP_WOOD    0x0008 — wooden planking, identified from the stage ACM
 *                        data itself (data.vol): the training stage's one
 *                        pond-deck node and CAVE2's 17 mine walkways carry
 *                        it, and ActCHASE switches to the hollow footstep
 *                        (sound 0x14) on it. DrawShadow overloads the same
 *                        bit on the humanoid's map COPY as an airborne
 *                        marker (level above the model → no ground shadow)
 * 0x0001 and MAP_BUOYANT are the two base ground materials in the ACM
 * data — 0x0001 dominant everywhere with no code reader found, 0x0002
 * clustered in the caves, which is consistent with its one reader: it
 * drives DefaultActionHumanoid's clamp (kill upward velocity, force
 * height 1), and a bit that did that everywhere would forbid jumping.
 * MAP_RESULT_FINAL marks a node result that ends the current leaf-list scan;
 * the authored maps commonly set it on every entry except the last, where
 * reaching the list end already has the same effect.
 * MAP_MATERIAL_MASK is the low block DefaultActionHumanoid wipes when a
 * character comes to rest on top of a conflict object. */
#define MAP_MATERIAL_MASK 0x007f
#define MAP_ATTRIBUTE_UNRESOLVED ((MapAttribute)0x0081)
/* CGetLevel/GetAreaMapLevel/ComputeAreaLevel return this when the probe
 * point is outside the area map ("no floor here"). */
#define LEVEL_NONE ((s32)0x80000000)

enum map_attribute_flag
{
    MAP_BUOYANT = 0x0002, /* the surface holds you up: see ATTR_BUOYANT */
    MAP_WATER = 0x0004,
    MAP_WOOD = 0x0008,
    MAP_DAMAGE = 0x0100,
    MAP_DEATH = 0x0200,
    MAP_RESULT_FINAL = 0x2000,
    MAP_SLOPE_X = 0x4000,
    MAP_SLOPE_Z = 0x8000
};

typedef struct IndexArrayType IndexArrayType;

/* ACM link words are relative byte offsets on disk. LoadAreaMap relocates
 * them in place into either a node-list or subdivision-table pointer. */
typedef union AreaMapReference AreaMapReference;
union AreaMapReference
{
    long address;
    AreaNodeType *nodes;
    IndexArrayType *subdivision;
}; /* 0x04 */

/* CONFLICT.C's area-map row index. */
typedef struct NodeIndexType NodeIndexType;
struct NodeIndexType
{
    s16 y;                  /* 0x00 */
    s16 n;                  /* 0x02 */
    AreaMapReference index; /* 0x04 */
    s16 x1;                 /* 0x08 */
    s16 z1;                 /* 0x0A */
    s16 x2;                 /* 0x0C */
    s16 z2;                 /* 0x0E */
}; /* 0x10 */

/* GetAreaMapLevel's load-bearing cursor is based at NodeIndexType.index.
 * These accessors retain that address shape while naming the surrounding
 * halfword fields. */
#define NODE_INDEX_BYTE_OFFSET(member) \
    ((s32)&((NodeIndexType *)0)->member)
#define NODE_INDEX_ROW_S16_OFFSET(member)                              \
    ((NODE_INDEX_BYTE_OFFSET(member) - NODE_INDEX_BYTE_OFFSET(index)) / \
     (s32)sizeof(s16))
#define NODE_INDEX_ROW_FIELD(row, member) \
    (((s16 *)(row))[NODE_INDEX_ROW_S16_OFFSET(member)])
#define NODE_INDEX_ROW_WORDS (sizeof(NodeIndexType) / sizeof(s32))

/* CONFLICT.C's lookup table for a subdivided area-node list. The leading
 * word is an offset on disk and an AreaNodeType pointer after relocation. */
#define AREA_INDEX_AXIS_SIZE 4
struct IndexArrayType
{
    AreaMapReference index;      /* 0x00 */
    area_node_index array[AREA_INDEX_AXIS_SIZE][AREA_INDEX_AXIS_SIZE]; /* 0x04 */
}; /* 0x24 */

/* GetAreaMapVector packs its four horizontal neighbour probes into a mask;
 * the reflection tables cover every possible mask value. */
#define N_MAP_PROBE_DIRECTIONS 4
#define MAP_PROBE_ALL ((1 << N_MAP_PROBE_DIRECTIONS) - 1)
#define N_MAP_PROBE_MASKS (MAP_PROBE_ALL + 1)

/* A bit for each horizontal direction sampled by GetAreaMapVector. */
typedef u8 MapProbeMask;

/* WORLD.C's packed four-stage think-function selector. Each nibble indexes
 * one of the four Think*Func tables. */
typedef short TThinkType;

/* The first three entries are shared by all four dispatch tables. */
enum think_basic_program
{
    THINK_BASIC_NONE = 0,
    THINK_BASIC_PAD1 = 1,
    THINK_BASIC_PAD2 = 2
};

/* Remaining entries in the individual dispatch tables. ThinkDB supplies
 * the editor-facing names; the two non-editor entries retain their recovered
 * function names. */
enum think1_program
{
    THINK1_TRACE = 3,
    THINK1_WATCH = 4,
    THINK1_RANDOM = 5,
    THINK1_NINJA = 6,
    THINK1_SLEEP = 7,
    THINK1_CHASE = 8,
    THINK1_TARGET = 9,
    N_THINK1_PROGRAMS = THINK1_TARGET + 1
};

enum think2_program
{
    THINK2_CONFIRM = 3,
    THINK2_CONTACT = 4,
    N_THINK2_PROGRAMS = THINK2_CONTACT + 1
};

enum think3_program
{
    THINK3_CALLAID = 3,
    THINK3_ATK_CHASE = 4,
    THINK3_ATK_POINT = 5,
    THINK3_ESCAPE = 6,
    THINK3_ATK_AREA = 7,
    THINK3_ATK_HITAWAY = 8,
    THINK3_FIRST_ATTACK = 9,
    N_THINK3_PROGRAMS = THINK3_FIRST_ATTACK + 1
};

enum think4_program
{
    THINK4_ABANDON = 3,
    THINK4_CONTACT = 4,
    THINK4_CHASE = 5,
    N_THINK4_PROGRAMS = THINK4_CHASE + 1
};

enum
{
    THINK_PROGRAM_BITS = 4,
    THINK_PROGRAM_MASK = (1 << THINK_PROGRAM_BITS) - 1,
    THINK2_PROGRAM_SHIFT = THINK_PROGRAM_BITS,
    THINK3_PROGRAM_SHIFT = THINK_PROGRAM_BITS * 2,
    THINK4_PROGRAM_SHIFT = THINK_PROGRAM_BITS * 3
};

#define THINK_MIX(think1, think2, think3, think4)                         \
    ((think1) | ((think2) << THINK2_PROGRAM_SHIFT) |                      \
     ((think3) << THINK3_PROGRAM_SHIFT) | ((think4) << THINK4_PROGRAM_SHIFT))

#define THINK1_FROM_MIX(type) ((type) & THINK_PROGRAM_MASK)
#define THINK2_FROM_MIX(type) \
    (((type) >> THINK2_PROGRAM_SHIFT) & THINK_PROGRAM_MASK)
#define THINK3_FROM_MIX(type) \
    (((type) >> THINK3_PROGRAM_SHIFT) & THINK_PROGRAM_MASK)
#define THINK4_FROM_MIX(type) \
    (((type) >> THINK4_PROGRAM_SHIFT) & THINK_PROGRAM_MASK)

#define THINK_MIX_NONE                                                    \
    THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE, THINK_BASIC_NONE,        \
              THINK_BASIC_NONE)
#define THINK_MIX_PLAYER                                                  \
    THINK_MIX(THINK_BASIC_PAD1, THINK_BASIC_PAD1, THINK_BASIC_PAD1,        \
              THINK_BASIC_PAD1)
#define THINK_MIX_PAD2                                                    \
    THINK_MIX(THINK_BASIC_PAD2, THINK_BASIC_PAD2, THINK_BASIC_PAD2,        \
              THINK_BASIC_PAD2)
#define THINK_MIX_NINKEN                                                  \
    THINK_MIX(THINK1_TARGET, THINK2_CONTACT, THINK3_ATK_CHASE,             \
              THINK4_CHASE)

/* THINK.C's editor database row, recovered from PSX.SYM. */
typedef struct ThinkDBtype ThinkDBtype;
struct ThinkDBtype
{
    u8 *name;
    TThinkType value;
};

/* Character ids are stored as signed halfwords so tables can use -1 as
 * their end marker; enum character_kind below supplies the named values. */
typedef s16 character_kind;
typedef u8 compact_character_kind;

/* WORLD.C's editable enemy placement. */
#define MAX_ENEMY_PATH_POINTS 7
typedef s32 enemy_layout_index;
#define ENEMY_LAYOUT_NONE (-1)

typedef struct TEnemyLayout TEnemyLayout;
struct TEnemyLayout
{
    character_kind type;  /* 0x00 */
    TThinkType ThinkType; /* 0x02 */
    s16 nPath;            /* 0x04 */
    s32 x;                /* 0x08 */
    s32 y;                /* 0x0C */
    s32 z;                /* 0x10 */
    s16 r;                /* 0x14 */
    s16 pad;              /* 0x16 */
    VECTOR path[MAX_ENEMY_PATH_POINTS]; /* 0x18 */
}; /* 0x88 */

/* Area-map query result. PSX.SYM supplies the original first 16 bytes and
 * field names. Retail appends the last two cached pointers: GetAreaMapVector
 * writes them at +0x10/+0x14, and the corresponding eight-byte growth is
 * visible where Humanoid.locate moves from demo +0x30 to retail +0x38. */
typedef struct MapVector MapVector;
struct MapVector
{
    s32 level;                   /* 0x00 */
    s32 height;                  /* 0x04 */
    MapAttribute attrib;         /* 0x08 */
    s16 degree;                  /* 0x0A */
    MapProbeMask vector;         /* 0x0C: blocked directions */
    u8 direct;                   /* 0x0D */
    MapProbeMask angleL;         /* 0x0E: neighbouring floor is higher */
    MapProbeMask angleH;         /* 0x0F: neighbouring floor is lower */
    struct AreaNodeType *area;   /* 0x10 (retail) */
    struct NodeIndexType *index; /* 0x14 (retail) */
}; /* 0x18 */

/* Parent/child transform record embedded ahead of model and ornament data.
 * LoadModelArchive, LoadOrnamentArchive, and LoadConstruction all consume
 * the same PSX.SYM-defined table. */
typedef struct ParentingType ParentingType;
struct ParentingType
{
    s16 np;    /* 0x00: parent number */
    s16 nc;    /* 0x02: child number */
    s16 dx;    /* 0x04 */
    s16 dy;    /* 0x06 */
    s16 dz;    /* 0x08 */
    u32 index; /* 0x0C */
}; /* 0x10 */

/* On-disk .MAD archive header. The model and ornament loaders use different
 * load widths for the same count halfword, so retain both views explicitly. */
typedef union ModelArchiveCount ModelArchiveCount;
union ModelArchiveCount
{
    s16 signed_count;
    u16 unsigned_count;
};

typedef struct ModelArchiveFile ModelArchiveFile;
struct ModelArchiveFile
{
    u32 signature;             /* 0x00 */
    ModelArchiveCount count;   /* 0x04 */
    u16 reserved;              /* 0x06 */
    ParentingType parenting[1]; /* 0x08, followed by linked TMD files */
}; /* 0x18 + variable data */

#define MODEL_ARCHIVE_BYTE_OFFSET(member) \
    ((s32)&((ModelArchiveFile *)0)->member)
#define MODEL_ARCHIVE_CURSOR_ADVANCE(cursor, from, to)                      \
    ((u_long *)((s32)(cursor) + MODEL_ARCHIVE_BYTE_OFFSET(to) -             \
                MODEL_ARCHIVE_BYTE_OFFSET(from)))
#define MODEL_ARCHIVE_SIGNED_COUNT(cursor) \
    (((ModelArchiveCount *)(cursor))->signed_count)
#define MODEL_ARCHIVE_UNSIGNED_COUNT(cursor) \
    (((ModelArchiveCount *)(cursor))->unsigned_count)
#define MODEL_ARCHIVE_PARENTING(file)                                      \
    ((ParentingType *)((s32)(file) + MODEL_ARCHIVE_BYTE_OFFSET(parenting)))

/* Model and humanoid attributes are signed 16-bit flag words. Keep their
 * storage types separate from the four-byte enums that name their bits. */
typedef s16 ModelAttribute;
typedef s16 HumanoidAttribute;

typedef s16 humanoid_life;
#define HUMANOID_LIFE_INACTIVE (-1)

typedef s16 character_status;

typedef s16 action_halt_state;
enum action_halt_state
{
    /* Stage completion is a terminal halt that CVAsequence must not clear. */
    ACTION_HALT_STAGE_END = -1,
    ACTION_HALT_NONE = 0,
    /* A cinematic or paired critical-hit animation owns actor control. */
    ACTION_HALT_ACTIVE = 1
};

typedef s16 search_result;

/* A model's signed-halfword index in ConflictObject; -1 is unregistered. */
typedef s16 conflict_id;

enum model_attribute_flag
{
    MODEL_ATTR_HIDDEN = 0x0001, /* never draw */
    MODEL_ATTR_NOCULL = 0x0002, /* skip the whole clip-point test */
    MODEL_ATTR_CULL_BEHIND = 0x0004, /* reject when the clip point is behind */
    MODEL_ATTR_CULL_SCREEN = 0x0008, /* reject outside the screen bounds */
    MODEL_ATTR_CULL_FAR = 0x0010, /* reject beyond the depth splice */
    MODEL_ATTR_COLLIDE = 0x4000, /* conflict slot active */
    MODEL_ATTR_CONFLICT = 0x8000
};

/* Packed libgs GsDOBJ2.attribute fields consumed by Tenchu's linked-TMD
 * decoders. */
enum
{
    GS_DOBJ_LMODE_SHIFT = 3,
    GS_DOBJ_LMODE_MASK = 3,
    GS_DOBJ_LIGNR_SHIFT = 5,
    GS_DOBJ_LIOFF_SHIFT = 6,
    GS_DOBJ_DIVISION_DEPTH_SHIFT = 9,
    GS_DOBJ_DIVISION_DEPTH_MASK = 7,
    GS_DOBJ_TON_SHIFT = 30,
    GS_DOBJ_FLAG_MASK = 1
};

#define GS_DOBJ_LMODE(attribute) \
    (((attribute) >> GS_DOBJ_LMODE_SHIFT) & GS_DOBJ_LMODE_MASK)
#define GS_DOBJ_LIGNR(attribute) \
    (((attribute) >> GS_DOBJ_LIGNR_SHIFT) & GS_DOBJ_FLAG_MASK)
#define GS_DOBJ_LIOFF(attribute) \
    (((attribute) >> GS_DOBJ_LIOFF_SHIFT) & GS_DOBJ_FLAG_MASK)
#define GS_DOBJ_DIVISION_DEPTH(attribute)                               \
    (((attribute) >> GS_DOBJ_DIVISION_DEPTH_SHIFT) &                    \
     GS_DOBJ_DIVISION_DEPTH_MASK)
#define GS_DOBJ_DIVISION_DEPTH_BITS(depth) \
    ((depth) << GS_DOBJ_DIVISION_DEPTH_SHIFT)
#define GS_DOBJ_TON(attribute) \
    (((attribute) >> GS_DOBJ_TON_SHIFT) & GS_DOBJ_FLAG_MASK)

/* WORLD.C/3DCTRL.C's shared model and ornament records. PSX.SYM supplies
 * each complete layout; these are used by items, characters, construction,
 * collision, effects, and the world object-slot manager. */
typedef struct ModelType ModelType;
struct ModelType
{
    GsCOORDINATE2 locate; /* 0x00 */
    SVECTOR rotate;       /* 0x50 */
    conflict_id id;       /* 0x58 */
    ModelAttribute attribute; /* 0x5A */
    SVECTOR clip;         /* 0x5C */
    GsDOBJ2 object;       /* 0x64 */
}; /* 0x74 */

/* A humanoid's articulated model. The skeleton's sub-object indices
 * the code pins: 0 is the waist/root, 2 the head (JAW's bite
 * hitbox; simple models' attach fallback), 8/0xb the barehanded
 * FIST fighter's striking-limb pair, and 0xd/0xe the two weapon-hand
 * anchors (arrows spawn at 0xd; the grapple hook fires and the medicine
 * bottle attaches at 0xe; armed attacks put hitboxes on both). Ninja
 * models carry 15 parts (HenshinModelSnapshot); NPC models fewer
 * (ProcItemKusuri's n > 0xe test picks the fallback). */
typedef struct ModelArchiveType ModelArchiveType;
struct ModelArchiveType
{
    GsCOORDINATE2 locate; /* 0x00 */
    SVECTOR rotate;       /* 0x50 */
    conflict_id id;       /* 0x58 */
    ModelAttribute attribute; /* 0x5A */
    SVECTOR clip;         /* 0x5C */
    s16 n;                /* 0x64 */
    ModelType **object;   /* 0x68 */
}; /* 0x6C */

typedef struct OrnamentType OrnamentType;
struct OrnamentType
{
    GsCOORDINATE2 locate; /* 0x00 */
    GsDOBJ2 object;       /* 0x50 */
}; /* 0x60 */

typedef struct OrnamentArchiveType OrnamentArchiveType;
struct OrnamentArchiveType
{
    GsCOORDINATE2 locate;  /* 0x00 */
    SVECTOR rotate;        /* 0x50 */
    conflict_id id;        /* 0x58 */
    ModelAttribute attribute; /* 0x5A */
    s16 n;                 /* 0x5C */
    OrnamentType **object; /* 0x60 */
    u_long *data;          /* 0x64 */
}; /* 0x68 */

typedef struct tag_ObjectSlotType ObjectSlotType;
struct tag_ObjectSlotType
{
    ObjectSlotType *next; /* 0x00 */
    OrnamentType *model;  /* 0x04 */
    s16 ModelSize;        /* 0x08 */
    s16 ShiftY;           /* 0x0A */
}; /* 0x0C */

typedef struct ObjectSlotManager ObjectSlotManager;
struct ObjectSlotManager
{
    ObjectSlotType *slot; /* 0x00 */
    s32 n;                /* 0x04 */
    s32 max;              /* 0x08 */
}; /* 0x0C */

/* The stage spatial hash has eight wrapping cells on each axis. */
#define WORLD_MAP_AXIS_SIZE 8
#define WORLD_MAP_AXIS_MASK (WORLD_MAP_AXIS_SIZE - 1)

/* One cell of the stage's 8x8x8 spatial hash (WorldMap):
 * LoadConstruction buckets every ornament object into its cell's
 * slot list, and DrawConstruction walks only the cells near the
 * camera. */
typedef struct WorldType WorldType;
struct WorldType
{
    ObjectSlotType *top; /* 0x00 */
}; /* 0x04 */

/* WORLD.C's scratchpad contract between DrawConstruction and IsVisible.
 * The producer caches the current camera at +0x38. Each visibility test
 * writes its relative point at +0x10 and leaves the rotated result at +0x00,
 * whose Z is then reused to choose a construction draw bucket. */
typedef struct ConstructionVisibilityWorkspace
    ConstructionVisibilityWorkspace;
struct ConstructionVisibilityWorkspace
{
    VECTOR view_space; /* 0x00: ApplyRotMatrix result */
    SVECTOR relative;  /* 0x10: world point minus camera position */
    u8 reserved[0x20]; /* 0x18 */
    GsRVIEW2 view;     /* 0x38: cached by DrawConstruction */
}; /* 0x58 */

/* EFFECT.C and several sprite helpers borrow this common scratchpad frame to
 * project one point: the GTE local-screen matrix, its input vector, and the
 * two auxiliary outputs written by RotTransPers. */
typedef struct ScreenProjectionWorkspace ScreenProjectionWorkspace;
struct ScreenProjectionWorkspace
{
    MATRIX local_screen; /* 0x00 */
    SVECTOR point;       /* 0x20 */
    s32 perspective;     /* 0x28: RotTransPers `p` output */
    s32 flag;            /* 0x2C: RotTransPers `flag` output */
}; /* 0x30 */

/* Motion IDs are stored in signed halfwords; -1 means no active motion and
 * terminates motion-registration and battle tables.  Their high byte is the
 * character-status / Act* handler family; the low byte selects a motion
 * within that family. */
typedef s16 motion_id;
#define MOTION_ID_NONE (-1)
#define MOTION_STATUS(mid) ((mid) >> 8)

/* AttackPQD's signed end-frame sentinel. */
#define MOTION_FRAME_ANY (-1)

typedef s16 motion_move_mode;
enum motion_move_mode
{
    /* No motion request is pending in the shared motID/motMODE mailbox. */
    MOTION_MOVE_UNSET = -1,
    /* Start the animation without applying its authored root speed. */
    MOTION_MOVE_NONE = 0,
    /* Apply orderspd/sidespd once when the animation is started. */
    MOTION_MOVE_APPLY = 1
};

/* MOTION.C's keyframe, motion, registry, spline, and playback records. */
typedef struct MotionElementType MotionElementType;
struct MotionElementType
{
    s16 x;    /* 0x00 */
    s16 y;    /* 0x02 */
    s16 z;    /* 0x04 */
    s16 time; /* 0x06 */
}; /* 0x08 */

typedef struct MotionDataType MotionDataType;

/* AMD links are byte offsets before LoadMotion relocates the archive and
 * typed pointers afterward.  MotionPack links are relative to the pack;
 * keyframe links are relative to their containing MotionDataType. */
typedef union MotionElementReference MotionElementReference;
union MotionElementReference
{
    s32 offset;
    MotionElementType *keyframes;
}; /* 0x04 */

typedef union MotionDataReference MotionDataReference;
union MotionDataReference
{
    s32 offset;
    MotionDataType *data;
}; /* 0x04 */

struct MotionDataType
{
    u8 n;                              /* 0x00 */
    u8 sweep;                          /* 0x01 */
    u8 orderspd;                       /* 0x02 */
    u8 sidespd;                        /* 0x03 */
    s16 time;                          /* 0x04 */
    s16 id;                            /* 0x06 */
    MotionElementReference locate;    /* 0x08: root keyframes */
    MotionElementReference rotate[1]; /* 0x0C: per-bone keyframes */
}; /* 0x10 */

typedef struct MotionRegistType MotionRegistType;
struct MotionRegistType
{
    motion_id mid;          /* 0x00 */
    s16 id;                 /* 0x02 */
    MotionDataType *motion; /* 0x04 */
}; /* 0x08 */

typedef struct SplineControlType SplineControlType;
struct SplineControlType
{
    MotionElementType *key0; /* 0x00 */
    MotionElementType *key1; /* 0x04 */
    SVECTOR dd0;             /* 0x08 */
    SVECTOR ds1;             /* 0x10 */
}; /* 0x18 */

/* Hermite interpolation uses a 5-bit fraction.  The lookup includes both
 * endpoints, so fraction 0..32 selects one complete SVECTOR basis row. */
enum
{
    SPLINE_FRACTION_SCALE = 32,
    N_SPLINE_BASIS_ROWS = SPLINE_FRACTION_SCALE + 1
};

/* MotionManager.mask selects which skeleton parts a motion drives.  Bit zero
 * owns the root translation and rotation; the remaining bits map directly to
 * ModelArchiveType.object[] indices. */
typedef s16 motion_part_mask;
enum motion_part_mask
{
    MOTION_MASK_ROOT = 1,
    MOTION_MASK_ALL = 0x7FFF, /* all 15 ninja model parts */
    MOTION_MASK_EVERY_PART = -1,
    MOTION_MASK_NOROOT = -2 /* pose bones without root translation */
};

#define MOTION_PART_ENABLED(mask, index) \
    (((mask) >> (index)) & MOTION_MASK_ROOT)

/* MotionManager.loop counts completed repeats. Negative values disable
 * normal playback; handlers may decrement them further as post-motion timers.
 * The CVA sequencer parks the counter at s16 max for an endless motion. */
typedef s16 motion_loop_count;
enum motion_loop_value
{
    MOTION_LOOP_FROZEN = -2,
    MOTION_LOOP_DISABLED = -1,
    MOTION_LOOP_FOREVER = 0x7FFF
};

/* The only observed MotionManager.mode bit is toggled on every ledge climb;
 * one phase starts the climb pose at frame 13. */
typedef s16 motion_manager_mode;
enum motion_manager_mode
{
    MOTION_MODE_DEFAULT = 0,
    MOTION_MODE_CLIMB_ALTERNATE = 1
};

enum motion_sweep_encoding
{
    MOTION_SWEEP_NEGATIVE_BIT = 0x80,
    MOTION_SWEEP_BYTE_RANGE = 0x100
};

typedef struct MotionManager MotionManager;
struct MotionManager
{
    motion_id mid;              /* 0x00 */
    s16 count;                  /* 0x02 */
    motion_loop_count loop;     /* 0x04 */
    s16 n;                      /* 0x06 */
    motion_part_mask mask;      /* 0x08 per-bone animation mask */
    motion_manager_mode mode;   /* 0x0A */
    ModelArchiveType *model;    /* 0x0C */
    MotionDataType *motion;     /* 0x10 */
    MotionRegistType *motreg;   /* 0x14 */
    SplineControlType *control; /* 0x18 */
}; /* 0x1C */

typedef struct MotionPackType MotionPackType;
struct MotionPackType
{
    s32 n;                         /* 0x00 */
    MotionDataReference motion[1]; /* 0x04 */
}; /* 0x08 */

/* MOTION.C's per-attack tuning row (BattleDB, indexed by warid; official
 * field names). Frame fields compare against the attack motion's
 * rising dtM->count (retail data: atks < atke) in ActATTACK:
 *   mid          the attack motion's id in GetMotionID's space
 *                (GetAttackDBID linear-searches it; -1 terminates)
 *   power        damage (DamageControl; parry stun derives from it)
 *   atks/atke    the frames where the weapon hitbox switches on/off
 *   contfrm      latest frame at which the next combo input is accepted
 *   revise       while count < this, the swing still steers toward the
 *                target (early aim-correction window)
 *   ilus/ilue    afterimage trail start/stop frames */
typedef struct BattleType BattleType;
struct BattleType
{
    motion_id mid; /* 0x00 */
    s16 power;   /* 0x02 */
    s16 atks;    /* 0x04 */
    s16 atke;    /* 0x06 */
    s16 contfrm; /* 0x08 */
    s16 revise;  /* 0x0A */
    s16 ilus;    /* 0x0C */
    s16 ilue;    /* 0x0E */
}; /* 0x10 */

/* CONFLICT.C's collision slot. PSX.SYM records result[64] and size 0x68 in
 * the demo. Retail raises the slot limit to 80 (InsertConflict), clears 0x50
 * result bytes, and reserves 0x2580 bytes for 80 slots, proving that the old
 * result array grew to 80 rather than gaining sixteen bytes of padding. */
/* ConflictObject slot class, stored in size.pad (and mirrored into
 * result[] entries together with the flags below when two slots overlap):
 *   CONFLICT_HIT   — a weapon/projectile hitbox (raises ATTR_HIT on touch)
 *   CONFLICT_STAND — the object's top can be stood on (resolver snaps the
 *                    character up and clears ATTR_PUSH | ATTR_FALL)
 *   CONFLICT_SOFT  — never pushes the character out (doors, sleep gas)
 * offset.pad doubles as GetConflictResult's per-wave hit budget.
 * result[] entries carry the partner's class bits plus:
 *   CONFLICT_LIVE     — overlap recorded this frame (ComputeAllConflict)
 *   CONFLICT_CONSUMED — already returned once by GetConflictResult */
/* ConflictObjectType.common holds either a Humanoid owner or one of the
 * ownerless item/door tags. */
typedef enum conflict_owner_tag ConflictOwnerTag;
enum conflict_owner_tag
{
    CONFLICT_OWNER_NONE = 0,
    CONFLICT_OWNER_ITEM = 1,
    CONFLICT_OWNER_DOOR = 2
};

typedef union ConflictOwner ConflictOwner;
union ConflictOwner
{
    void *raw; /* PSX.SYM's original common field view */
    struct Humanoid *human;
    ConflictOwnerTag tag;
}; /* 0x04 */

typedef enum conflict_class ConflictClass;
enum conflict_class
{
    CONFLICT_HIT = 1,
    CONFLICT_STAND = 4,
    CONFLICT_SOFT = 8
};

enum conflict_result_flag
{
    CONFLICT_CONSUMED = 0x40,
    CONFLICT_LIVE = 0x80
};
#define N_CONFLICT_OBJECTS 80

typedef struct ConflictObjectType ConflictObjectType;
struct ConflictObjectType
{
    struct ModelType *model; /* 0x00 */
    VECTOR position;         /* 0x04 */
    SVECTOR offset;          /* 0x14 */
    SVECTOR size;            /* 0x1C */
    ConflictOwner common;    /* 0x24 */
    u8 result[N_CONFLICT_OBJECTS]; /* 0x28 */
}; /* 0x78 */

/* 3DCTRL.C's textured sprite model. The demo PSX.SYM supplies the complete
 * layout and original member names; the retail users confirm the same 0x8C
 * size and offsets. */
typedef struct Sprite3D Sprite3D;
struct Sprite3D
{
    GsCOORDINATE2 locate; /* 0x00 */
    SVECTOR rotate;       /* 0x50 */
    conflict_id id;       /* 0x58 */
    ModelAttribute attribute; /* 0x5A */
    SVECTOR clip;         /* 0x5C */
    s32 scale;            /* 0x64 */
    GsSPRITE sprite;      /* 0x68 */
}; /* 0x8C */

/* 3DCTRL.C's tiled background. PSX.SYM supplies the complete layout and
 * original `hundle` spelling; retail confirms the same 0x48-byte record. */
typedef struct BackGround BackGround;
struct BackGround
{
    GsBG hundle;   /* 0x00 */
    GsMAP map;     /* 0x24 */
    GsCELL *cell;  /* 0x34 */
    u32 *work;     /* 0x38 */
    u16 *index;    /* 0x3C */
    u16 sz;        /* 0x40 */
    conflict_id id; /* 0x42 */
    ModelAttribute attribute; /* 0x44 */
}; /* 0x48 */

/* CHRANIM.C's 12-byte CVA cutscene-script grammar. A sequence opens with a
 * SEQUENCE header and then runs batches of command rows separated by WAIT
 * markers. A zero-frame wait ends the sequence; CVA_CMD_END terminates the
 * complete table. */
typedef s16 cva_command;
enum cva_command
{
    CVA_CMD_END = -1,
    CVA_CMD_SEQUENCE = 0,
    CVA_CMD_WAIT = 1,
    CVA_CMD_MOTION = 2,
    CVA_CMD_ACTOR = 3,
    CVA_CMD_CAMERA_CUT = 4,
    CVA_CMD_CAMERA_POSE = 5,
    CVA_CMD_CAMERA_PAN = 6,
    CVA_CMD_EFFECT = 7,
    CVA_CMD_TELOP = 8
};

/* Command-specific sentinels stored in the shared payload halfwords. */
enum
{
    CVA_MUSIC_STOP = -1,
    CVA_MOTION_NO_REPOSITION = -1,
    CVA_ACTOR_DESPAWN = -1,
    CVA_TELOP_CLEAR = -1
};

typedef s16 cva_camera_cut_kind;
enum cva_camera_cut_kind
{
    CVA_CAMERA_CUT_TARGET_RELATIVE_BASE = 0,
    CVA_CAMERA_CUT_TARGET_RELATIVE_QUARTER_TURN = 1,
    CVA_CAMERA_CUT_TARGET_RELATIVE_HALF_TURN = 2,
    CVA_CAMERA_CUT_TARGET_RELATIVE_THREE_QUARTER_TURN = 3,
    CVA_CAMERA_CUT_FIXED_POSITION = 4,
    CVA_CAMERA_CUT_HUMANOID_POSITION = 5
};

typedef s16 cva_camera_pose_kind;
enum cva_camera_pose_kind
{
    CVA_CAMERA_POSE_FIXED_REFERENCE = 0,
    CVA_CAMERA_POSE_HUMANOID_REFERENCE = 1
};

typedef s16 camera_pan_mode;
enum camera_pan_mode
{
    CAMERA_PAN_DISABLED = 0,
    CAMERA_PAN_NORMAL_CAMERA = 1,
    CAMERA_PAN_ORBIT_ANGLE_INCREASE = 2,
    CAMERA_PAN_ORBIT_ANGLE_DECREASE = 3,
    CAMERA_PAN_UP = 4,
    CAMERA_PAN_DOWN = 5,
    CAMERA_PAN_ZOOM_IN = 6,
    CAMERA_PAN_ZOOM_OUT = 7,
    CAMERA_PAN_TRACK_TARGET = 8,
    CAMERA_PAN_REFRESH_VIEW = 20
};

typedef s16 cva_effect_kind;
enum cva_effect_kind
{
    CVA_EFFECT_BLOOD = 1,
    CVA_EFFECT_FADE = 3
};

enum cva_format_constant
{
    CVA_WORLD_POSITION_SCALE = 1000,
    CVA_CAMERA_POSITION_SCALE = 100,
    CVA_EFFECT_POSITION_SCALE = 10,
    CVA_CAMERA_DEFAULT_ORBIT_DISTANCE = 3000,
    CVA_CAMERA_DEFAULT_PAN_SPEED = 20,
    CVA_CAMERA_TARGET_HEIGHT_OFFSET = 300,
    CVA_BLOOD_DURATION = 30
};

typedef struct CVACoordinates CVACoordinates;
struct CVACoordinates
{
    s16 x;
    s16 y;
    s16 z;
}; /* 0x06 */

/* The raw member names are the original CVAType fields recorded by PSX.SYM.
 * The other views name the same five payload halfwords according to the row
 * tag that owns them. */
typedef struct CVARawPayload CVARawPayload;
struct CVARawPayload
{
    s16 id; /* 0x00 */
    s16 x;  /* 0x02 */
    s16 y;  /* 0x04 */
    s16 z;  /* 0x06 */
    s16 p;  /* 0x08 */
}; /* 0x0A */

typedef struct CVASequencePayload CVASequencePayload;
struct CVASequencePayload
{
    s16 id;
    s16 reserved[3];
    s16 music;
}; /* 0x0A */

typedef struct CVAWaitPayload CVAWaitPayload;
struct CVAWaitPayload
{
    s16 frames;
    s16 reserved[4];
}; /* 0x0A */

typedef struct CVAMotionPayload CVAMotionPayload;
struct CVAMotionPayload
{
    character_kind actor;
    CVACoordinates position;
    s16 facing;
}; /* 0x0A */

typedef struct CVAActorPayload CVAActorPayload;
struct CVAActorPayload
{
    character_kind actor;
    motion_id motion;
    motion_loop_count loop;
    motion_id next_motion;
    s16 reserved;
}; /* 0x0A */

typedef struct CVACameraOrbitParameters CVACameraOrbitParameters;
struct CVACameraOrbitParameters
{
    s16 reserved[3];
    s16 distance;
}; /* 0x08 */

typedef struct CVACameraFixedParameters CVACameraFixedParameters;
struct CVACameraFixedParameters
{
    CVACoordinates position;
    s16 reserved;
}; /* 0x08 */

typedef struct CVACameraActorParameters CVACameraActorParameters;
struct CVACameraActorParameters
{
    s16 reserved[3];
    character_kind actor;
}; /* 0x08 */

typedef union CVACameraParameters CVACameraParameters;
union CVACameraParameters
{
    CVACameraOrbitParameters orbit;
    CVACameraFixedParameters fixed;
    CVACameraActorParameters humanoid;
}; /* 0x08 */

typedef struct CVACameraCutPayload CVACameraCutPayload;
struct CVACameraCutPayload
{
    cva_camera_cut_kind kind;
    CVACameraParameters parameters;
}; /* 0x0A */

typedef struct CVACameraPosePayload CVACameraPosePayload;
struct CVACameraPosePayload
{
    cva_camera_pose_kind kind;
    CVACameraParameters reference;
}; /* 0x0A */

typedef struct CVACameraPanPayload CVACameraPanPayload;
struct CVACameraPanPayload
{
    camera_pan_mode mode;
    s16 reserved[3];
    s16 speed;
}; /* 0x0A */

typedef struct CVAEffectRawParameters CVAEffectRawParameters;
struct CVAEffectRawParameters
{
    s16 x;
    s16 y;
    s16 z;
    s16 p;
}; /* 0x08 */

typedef struct CVABloodParameters CVABloodParameters;
struct CVABloodParameters
{
    CVACoordinates position;
    s16 count;
}; /* 0x08 */

typedef struct CVAFadeParameters CVAFadeParameters;
struct CVAFadeParameters
{
    s16 red;
    s16 green;
    s16 blue;
    s16 frames;
}; /* 0x08 */

typedef union CVAEffectParameters CVAEffectParameters;
union CVAEffectParameters
{
    CVAEffectRawParameters raw;
    CVABloodParameters blood;
    CVAFadeParameters fade;
}; /* 0x08 */

typedef struct CVAEffectPayload CVAEffectPayload;
struct CVAEffectPayload
{
    cva_effect_kind kind;
    CVAEffectParameters parameters;
}; /* 0x0A */

typedef struct CVATelopPayload CVATelopPayload;
struct CVATelopPayload
{
    s16 text_offset;
    s16 reserved[4];
}; /* 0x0A */

typedef union CVACommandPayload CVACommandPayload;
union CVACommandPayload
{
    CVARawPayload raw;
    CVASequencePayload sequence;
    CVAWaitPayload wait;
    CVAMotionPayload motion;
    CVAActorPayload actor;
    CVACameraCutPayload camera_cut;
    CVACameraPosePayload camera_pose;
    CVACameraPanPayload camera_pan;
    CVAEffectPayload effect;
    CVATelopPayload telop;
}; /* 0x0A */

typedef struct CVAType CVAType;
struct CVAType
{
    cva_command mode;          /* 0x00 */
    CVACommandPayload payload; /* 0x02 */
}; /* 0x0C */

/* CHRANIM.C's queued character-motion slot. */
typedef struct HumanAnimType HumanAnimType;
struct HumanAnimType
{
    struct Humanoid *human; /* 0x00 */
    motion_loop_count loop; /* 0x04 */
    motion_id motid;        /* 0x06 */
}; /* 0x08 */

#define N_CVA_HUMANS 5
#define N_TANKA_SPRITES 6

/* Motion-id families (motID / MotionManager.mid): the high byte indexes the
 * Act* handler table at 0x80086b24 (ActNORMAL..ActDEAD, the demo's own
 * function names), the low byte selects the move within the family.  A
 * named-constant overlay — PSX.SYM records no enum for these; the bases are
 * proven by the dispatch table order. */
enum motion_family
{
    MOT_NORMAL = 0x000,
    MOT_ACTION = 0x100,
    MOT_MOVE = 0x200,
    MOT_SWIM = 0x300,
    MOT_KAGI = 0x400,
    MOT_ENGAGE = 0x500,
    MOT_CHASE = 0x600,
    MOT_ATTACK = 0x700,
    MOT_STATE = 0x800,
    MOT_JUMP = 0x900,
    MOT_HANG = 0xA00,
    MOT_SQUAT = 0xB00,
    MOT_STICKON = 0xC00,
    MOT_CEILHANG = 0xD00,
    MOT_SYURI = 0xE00,
    /* MOT_ITEM sub-ids: +0 scatter makibishi, +1 drink/eat (kusuri,
     * dokudango), +2 throw (fire/smoke/nemuri), +3 plant (jirai,
     * goshikimai), +4 kaengeki flame, +5 shinsoku cast (exempt from
     * ActivateHumans' think budget while playing). */
    MOT_ITEM = 0xF00,
    MOT_DAMAGE = 0x1000,
    MOT_DEAD = 0x1100
};

/* Specific motion ids (family | index). Invented names — none appear in
 * the demo symbols; each derived from the id's setter and handler arm
 * (see the Act* files). Family roots (0x100, 0x200, ...) keep their
 * MOT_* family names above. */
enum
{
    MOT_NORMAL_TURN_R = 0x001,    /* idle pivot (ActNORMAL) */
    MOT_NORMAL_TURN_L = 0x002,
    MOT_ACTION_LOOP = 0x101,      /* AI scripted idle, loops until pad */
    MOT_ACTION_GESTURE = 0x102,   /* AI scripted one-shot */
    /* Character-specific registration-table variants whose behavior is only
     * visible in their family handler; main.exe never requests them directly. */
    MOT_ACTION_VARIANT_3 = 0x103,
    MOT_ACTION_FIDGET_A = 0x104,  /* random standing fidget (coin flip) */
    MOT_ACTION_FIDGET_B = 0x105,
    MOT_ACTION_NOTICE = 0x106,    /* guard spots the player */
    MOT_MOVE_BACK = 0x201,
    MOT_MOVE_DASH_FWD = 0x202,
    MOT_MOVE_DASH_BACK = 0x203,
    MOT_MOVE_DASH_RIGHT = 0x204,
    MOT_MOVE_DASH_LEFT = 0x205,
    MOT_SWIM_EXIT = 0x301,
    MOT_SWIM_STROKE = 0x302,
    MOT_KAGI_FLY = 0x401,         /* hook in flight */
    MOT_KAGI_PULL = 0x402,        /* reel-in to the wall */
    MOT_ENGAGE_STANCE = 0x501,    /* weapon-drawn alert idle */
    MOT_ENGAGE_VARIANT_2 = 0x502,
    MOT_ENGAGE_SHEATHE = 0x503,   /* stance exit into MOT_STATE_SHEATHE */
    MOT_ENGAGE_TURN_R = 0x504,
    MOT_ENGAGE_TURN_L = 0x505,
    MOT_CHASE_BACK = 0x602,
    MOT_CHASE_DASH_BACK = 0x604,
    MOT_CHASE_DASH_RIGHT = 0x605,
    MOT_CHASE_DASH_LEFT = 0x606,
    MOT_CHASE_DASH_FWD = 0x607,   /* cancellable into lunge/roll */
    MOT_ATTACK_SLASH2 = 0x701,
    MOT_ATTACK_SLASH2_RIGHT = 0x702,
    MOT_ATTACK_SLASH2_LEFT = 0x703,
    MOT_ATTACK_SLASH3 = 0x704,
    MOT_ATTACK_SLASH4 = 0x705,
    MOT_ATTACK_RIGHT1 = 0x706,
    MOT_ATTACK_RIGHT2 = 0x707,
    MOT_ATTACK_RIGHT2_LEFT = 0x708,
    MOT_ATTACK_LEFT1 = 0x709,
    MOT_ATTACK_LEFT2 = 0x70a,
    MOT_ATTACK_LEFT2_RIGHT = 0x70b,
    MOT_ATTACK_CROUCH = 0x70c,
    MOT_ATTACK_LUNGE = 0x70d,
    MOT_ATTACK_DIVE = 0x70f,
    MOT_ATTACK_DIVE_LAND = 0x710,
    MOT_ATTACK_BACK = 0x711,
    MOT_ATTACK_LUNGE_BACK = 0x712,
    MOT_ATTACK_TAUNT = 0x713,
    MOT_ATTACK_STEALTH_BACK = 0x714,  /* stealth kills; victim plays the */
    MOT_ATTACK_STEALTH_FRONT = 0x715, /* paired MOT_DEAD_STEALTH_* id;   */
    MOT_ATTACK_STEALTH_SIDE = 0x716,  /* Ayame's set is +3               */
    MOT_ATTACK_STEALTH_BACK_AYAME = 0x717,
    MOT_ATTACK_STEALTH_FRONT_AYAME = 0x718,
    MOT_ATTACK_STEALTH_SIDE_AYAME = 0x719,
    MOT_STATE_CLIMB = 0x801,
    MOT_STATE_VARIANT_2 = 0x802,
    MOT_STATE_FALL = 0x803,
    MOT_STATE_LAND = 0x804,
    MOT_STATE_LAND_HEAVY = 0x805,
    MOT_STATE_LAND_FLIP = 0x806,
    MOT_STATE_DRAW = 0x80e,       /* draw weapon, into the stance */
    MOT_STATE_SHEATHE = 0x80f,    /* stand down, into idle */
    MOT_STATE_PICKUP = 0x810,     /* frozen while pocketing a dropped item */
    MOT_JUMP_WALLKICK = 0x901,
    MOT_JUMP_FORWARD = 0x902,
    MOT_JUMP_BACK = 0x903,
    MOT_JUMP_RIGHT = 0x904,
    MOT_JUMP_LEFT = 0x905,
    MOT_JUMP_RUN = 0x906,         /* running leap out of the dash */
    MOT_JUMP_FLIP = 0x907,        /* half-turn flip, lands as LAND_FLIP */
    MOT_HANG_CATCH = 0xA01,
    MOT_HANG_SHIMMY_RIGHT = 0xA02,
    MOT_HANG_SHIMMY_LEFT = 0xA03,
    MOT_HANG_PULLUP = 0xA04,
    MOT_SQUAT_WALK_F = 0xB01,
    MOT_SQUAT_WALK_B = 0xB02,
    MOT_SQUAT_WALK_R = 0xB03,
    MOT_SQUAT_WALK_L = 0xB04,
    MOT_SQUAT_ROLL_F = 0xB05,
    MOT_SQUAT_ROLL_B = 0xB06,
    MOT_SQUAT_ROLL_R = 0xB07,
    MOT_SQUAT_ROLL_L = 0xB08,
    MOT_SQUAT_BACKFLIP = 0xB09,   /* X from crouch: 180-degree escape flip */
    MOT_STICKON_SLIDE_L = 0xC01,  /* wall sidles; L/R derived from the */
    MOT_STICKON_SLIDE_R = 0xC02,  /* MoveHumanoid side-sign convention */
    MOT_STICKON_THROW_L = 0xC03,  /* lean out past the corner and throw */
    MOT_STICKON_THROW_R = 0xC04,
    MOT_SYURI_RECOVER = 0xE01,
    MOT_ITEM_DRINK = 0xF01,
    MOT_ITEM_THROW = 0xF02,
    MOT_ITEM_PLANT = 0xF03,
    MOT_ITEM_KAENGEKI = 0xF04,    /* fire-breath loop */
    MOT_ITEM_SHINSOKU = 0xF05,    /* far-sight cast (widens AI activation) */
    MOT_DAMAGE_FRONT_MID = 0x1001,   /* damagemotion[]: front hits by      */
    MOT_DAMAGE_FRONT_HEAVY = 0x1002, /* severity; second half from behind */
    MOT_DAMAGE_BACK_LIGHT = 0x1003,
    MOT_DAMAGE_BACK_HEAVY = 0x1004,
    MOT_DAMAGE_LAUNCH_BACK = 0x1005, /* knocked off the feet backward */
    MOT_DAMAGE_LAUNCH_FORE = 0x1006, /* knocked forward (hit from behind) */
    MOT_DAMAGE_SLAM_BACK = 0x1007,   /* ground impact ending the launch */
    MOT_DAMAGE_SLAM_FORE = 0x1008,
    MOT_DAMAGE_DOWNED = 0x1009,      /* lying downed until GETUP */
    MOT_DAMAGE_MAKIBISHI = 0x100A,   /* caltrop hop */
    MOT_DAMAGE_CHOKE = 0x100B,       /* smoke/poison cough, no damage */
    MOT_DAMAGE_GETUP = 0x100C,
    MOT_DEAD_ALT = 0x1101,           /* coin-flip second ordinary death */
    MOT_DEAD_DROWN = 0x1108,
    MOT_DEAD_STEALTH_BACK = 0x1109,  /* stealth-kill collapses, paired */
    MOT_DEAD_STEALTH_FRONT = 0x110A, /* with the attacker's 0x714-0x719 */
    MOT_DEAD_STEALTH_SIDE = 0x110B,  /* (AttackControl adds 3 to both   */
    MOT_DEAD_STEALTH_BACK_AYAME = 0x110C,  /* ids when playing Ayame)   */
    MOT_DEAD_STEALTH_FRONT_AYAME = 0x110D,
    MOT_DEAD_STEALTH_SIDE_AYAME = 0x110E
};

/* damagemotion[] stores four front-hit severity tiers followed by the four
 * corresponding from-behind reactions. The last tier launches the victim. */
#define N_DAMAGE_MOTION_TIERS 4
#define DAMAGE_MOTION_LAUNCH_TIER (N_DAMAGE_MOTION_TIERS - 1)
#define DAMAGE_MOTION_FROM_BEHIND_OFFSET N_DAMAGE_MOTION_TIERS
#define N_DAMAGE_MOTIONS (N_DAMAGE_MOTION_TIERS * 2)

/* Camera-mode names recovered from the demo's CAMERA.C. This list is not
 * exhaustive for retail: current code also supplies unnamed modes 15-17. */
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
    CMODE_FALL = 14,
    /* Invented name: zeroes DirectionRX/RY and enters the direction view
     * looking straight ahead — the kaginawa first-person aim. */
    CMODE_AIM = 15,
    /* Invented names — one-shot poses that snap back to CMODE_NORMAL:
     * the heavy-damage knockback view (motions 0x1005-0x1009/0x100C,
     * with a near-wall fallback pose) and the ledge-hang view. */
    CMODE_KNOCKBACK = 0x10,
    CMODE_HANG = 0x11
};

/* CAMERA.C's original maximum-index spelling, plus retail's four-entry
 * critical-hit camera count. */
enum
{
    MaxCriticalValiation = 3,
    N_CRITICAL_CAMERA_POSITIONS = MaxCriticalValiation + 1
};

/* CAMERA.C's global camera state. Retail rearranges the demo PSX.SYM
 * record: DirectionRX/DirectionRY move ahead of OldMode, and OldMode becomes
 * a byte beside a new one-shot camera-snap flag. The resulting retail record
 * is 0x20 bytes; the demo's Valiation member at +0x20 is not part of it. */
typedef struct TCameraStatus TCameraStatus;
struct TCameraStatus
{
    VECTOR TargetVector;    /* 0x00 */
    struct Humanoid *Owner; /* 0x10 */
    TCameraMode Mode;       /* 0x14 */
    s16 DirectionRX;        /* 0x18 */
    s16 DirectionRY;        /* 0x1A */
    u8 OldMode;             /* 0x1C */
    u8 snap_pending;        /* 0x1D (retail-inferred role) */
}; /* 0x20 */

/* System flags named by the demo's PSX.SYM.  The random-layout name is
 * retail-inferred from CreateStage and the stage debug menu.
 * SYSFLAG_DEBUG_SELECT is a vestigial latch: PauseProc raises it when
 * Select exits a debug-mode pause and clears it on the next pause entry.
 * Its only reader (the DrawPause suppression inside the pause loop) can
 * never observe it set — the setter breaks out of the loop and re-entry
 * clears it first — so the effect is dead in retail (a demo-era debug
 * hook). */
typedef enum TSystemFlag TSystemFlag;
enum TSystemFlag
{
    SYSFLAG_DEBUGPRINT = 1,
    SYSFLAG_DEBUGMODE = 2,
    SYSFLAG_PAUSE = 4,
    SYSFLAG_RANDOM_LAYOUT = 8,
    SYSFLAG_DEBUG_SELECT = 0x10
};

/* The demo's TCameraPos contained one pos/ref pair. Retail replaces it
 * with reference and position endpoint pairs; r1/r2/p1/p2 are the game's
 * OWN labels (the debug camera editor prints exactly these strings for
 * the four slots of the live CamPos block). */
enum debug_camera_slot
{
    DEBUG_CAMERA_SLOT_R1 = 0,
    DEBUG_CAMERA_SLOT_R2 = 1,
    DEBUG_CAMERA_SLOT_P1 = 2,
    DEBUG_CAMERA_SLOT_P2 = 3,
    N_DEBUG_CAMERA_SLOTS = 4
};

typedef struct TCameraPos TCameraPos;
struct TCameraPos
{
    SVECTOR r1; /* 0x00 */
    SVECTOR r2; /* 0x08 */
    SVECTOR p1; /* 0x10 */
    SVECTOR p2; /* 0x18 */
}; /* 0x20 */

/* The debug editor treats the live camera block both as one four-vector
 * camera preset and as four independently selectable vectors. */
typedef union DebugCameraStorage DebugCameraStorage;
union DebugCameraStorage
{
    TCameraPos camera;
    SVECTOR slot[N_DEBUG_CAMERA_SLOTS]; /* indexed by enum debug_camera_slot */
}; /* 0x20 */

/* CDPLAYER.C's playback state and its original anonymous-enum constants.
 * Retail keeps the demo's original members but rearranges the tail, adds the
 * left/right volume bytes, and appends the pending drive command. */
typedef struct TCdaStatus TCdaStatus;
typedef s16 cda_play_mode;
enum cda_play_mode
{
    CDA_ONCE = 0,
    CDA_REPEAT = 1
};

typedef u8 cda_flags;
enum cda_flag
{
    CDA_FLAG_ACTIVE = 1
};

typedef u8 cda_drive_status;
enum cda_drive_status_value
{
    CDA_STATUS_IDLE = 0
};

typedef u8 cda_command;
enum cda_command_value
{
    CDA_COMMAND_NONE = 0,
    CDA_COMMAND_READ_XA = CdlReadS
};

enum cda_stream_constant
{
    CDA_SECTORS_PER_SECOND = 75,
    CDA_FILE_LEAD_IN_SECTORS = 2 * CDA_SECTORS_PER_SECOND,
    CDA_POSITION_GUARD_SECTORS = 4 * CDA_SECTORS_PER_SECOND,
    CDA_DATA_SECTOR_SHIFT = 11,
    CDA_STATUS_CHECK_THRESHOLD = 10,
    CDA_XA_FILE_NUMBER = 1,
    CDA_DRIVE_SETTLE_FRAMES = 3,
    CDA_STOPPED_POSITION = -2,
    CDA_XA_DRIVE_MODE = CdlModeSpeed | CdlModeRT | CdlModeSF | CdlModeDA
};

struct TCdaStatus
{
    s32 StartPos;            /* 0x00 */
    s32 CurPos;              /* 0x04 */
    s32 EndPos;              /* 0x08 */
    cda_play_mode mode;      /* 0x0C */
    s16 CheckCount;          /* 0x0E */
    cda_drive_status status; /* 0x10 */
    u8 voll;                 /* 0x11 */
    u8 volr;                 /* 0x12 */
    cda_flags flag;          /* 0x13 */
    cda_command command;     /* 0x14 */
}; /* 0x18 */

/* CAMERA.C's smoothing history. Retail inserted a per-frame acceleration
 * ahead of the demo's `spd`/`bef` fields, shifting them by two bytes. */
typedef struct TMakeDifInfo TMakeDifInfo;
struct TMakeDifInfo
{
    s16 div;     /* 0x00 */
    s16 ac;      /* 0x02 (retail; demo's local AC = 14) */
    s16 spd;     /* 0x04 */
    SVECTOR bef; /* 0x06 */
}; /* 0x0E */

/* STAGE.C's stage-event sentinels and descriptor. */
#define EVENT_TABLE_END (-1)
#define EVENT_ID_NONE 0xFF
#define EVENT_CVA_NONE 0xFF
#define EVENT_TARGET_PLAYER 0xFF

typedef struct EventSeqType EventSeqType;
typedef u8 event_trigger_kind;

typedef struct EventAxisBounds EventAxisBounds;
struct EventAxisBounds
{
    s16 min;
    s16 max;
}; /* 0x04 */

/* EventSeqType's original status/x/y/z fields, retained as the raw view of
 * the trigger payload. Non-zone records leave the six bounds at -1. */
typedef struct EventRawTrigger EventRawTrigger;
struct EventRawTrigger
{
    s16 status;
    s16 x[2];
    s16 y[2];
    s16 z[2];
}; /* 0x0E */

typedef struct EventZoneTrigger EventZoneTrigger;
struct EventZoneTrigger
{
    s16 reserved;
    EventAxisBounds x;
    EventAxisBounds y;
    EventAxisBounds z;
}; /* 0x0E */

/* mode selects the interpretation of the halfword at +6. The zone view is
 * the only one that also consumes the six following bounds. */
typedef union EventTrigger EventTrigger;
union EventTrigger
{
    EventRawTrigger raw;
    EventZoneTrigger zone;
    HumanoidAttribute attribute_mask;
    character_status status;
    motion_id motion;
    humanoid_life life;
    s16 time;
    s16 music;
}; /* 0x0E */

typedef struct EventRoute EventRoute;
struct EventRoute
{
    u8 id;    /* sequence id (2-3 = the root scripts) */
    u8 event; /* CVA sequence (EVENT_CVA_NONE = none) */
    u8 next1; /* slot-0 successor (EVENT_ID_NONE = stop) */
    u8 next2; /* slot-1 successor (EVENT_ID_NONE = stop) */
};

typedef union EventHeader EventHeader;
union EventHeader
{
    EventRoute route;
    s32 word; /* EVENT_TABLE_END terminates the table */
};

struct EventSeqType
{
    EventHeader header; /* 0x00 */
    u8 target;  /* 0x04 watched humanoid (EVENT_TARGET_PLAYER = player) */
    event_trigger_kind mode; /* 0x05 EVTRIG_ trigger kind (stage.h) */
    EventTrigger trigger; /* 0x06 */
}; /* 0x14 */

/* APPEAR.C's per-weapon anchor points, in the weapon model's local
 * space: confp positions the CONFlict hitbox (its .pad doubles as the
 * hitbox size — ActATTACK), and ilup0/ilup1 are the two ILlUsion
 * Points the afterimage trail (BattleType's ilus/ilue window)
 * stretches between — blade root and tip. ilup1.pad doubles as the
 * row's weapon id, with WEAPON_KIND_END terminating the table
 * (GetWeaponData). */
typedef struct WeaponType WeaponType;
struct WeaponType
{
    SVECTOR confp; /* 0x00 */
    SVECTOR ilup0; /* 0x08 */
    SVECTOR ilup1; /* 0x10 */
}; /* 0x18 */

/* APPEAR.C's weapon-model database row. */
/* Weapon kinds use signed halfword storage so -1 can terminate the APPEAR
 * tables; enum weapon_kind below supplies the named values. */
typedef s16 weapon_kind;

typedef struct WeaponModelType WeaponModelType;
struct WeaponModelType
{
    u8 *name;      /* 0x00 */
    weapon_kind wid; /* 0x04 */
    u_long *model; /* 0x08 */
}; /* 0x0C */

/* APPEAR.C's character database row. */
/* Weapon kinds (HumanDataType.wepid, copied into Humanoid.wpatk by
 * SetupWeapon). The high nibble is the RANGE CLASS the think layer
 * extracts with `wpatk >> 4` to pick the Attack* controller (0 short /
 * 1 general / 2 long / 3 indirect-ranged: every *_YUMI archer is 0x32).
 * The names in `weapon_kind` above are the game's own for every kind
 * from 0x04 up: WeaponModel[] pairs each wid with the string it builds
 * its .TMD path from, and all of 0x04..0x37 match. Only 0x00..0x03 have
 * no WeaponModel row, because they load no model at all — GetWeaponData
 * just registers the id and the strike comes off the body. WeaponDB
 * gives them away: every carried weapon reaches forward (KATANA_0 sits
 * at z -450), while CLAW and FIST sit at z 0 on the body itself (y 200
 * and y 600) and JAW reaches 300 at ground level. Their wielders agree
 * — CLAW is KUMA plus BALMA's off-hand, FIST the barehanded great-ninja
 * twins (limbs 8/0xb), JAW the whole beast page including wolf, firedog
 * and ninken — so those three names describe the strike rather than one
 * wielder, and are the only invented ones here. */

typedef struct HumanDataType HumanDataType;
struct HumanDataType
{
    character_kind type;           /* 0x00 */
    weapon_kind wepid;             /* 0x02 */
    s16 turn;                      /* 0x04 */
    humanoid_life life;            /* 0x06 */
    s16 width;                     /* 0x08 */
    s16 height;                    /* 0x0A */
    struct MotionRegistType *mtbl; /* 0x0C */
    u8 *name;                      /* 0x10 */
    u_long *model;                 /* 0x14 */
}; /* 0x18 */

/* STAGE.C's per-stage character placement. */
#define STAGE_CHAR_END (-1)

typedef struct StageCharType StageCharType;
struct StageCharType
{
    s16 stage;        /* 0x00 */
    character_kind chrid; /* 0x02 */
    SVECTOR position; /* 0x04 */
    TThinkType think; /* 0x0C */
}; /* 0x0E */

/* Retail has two distinct stage-number spaces.
 *
 * stage_id indexes StageConfig and the stage-specific runtime tables.  Its
 * order is the physical STAGE1..STAGE11 asset order.  stage_uid is the
 * campaign order stored in TStageConfig.uid and StageNoMAX.  StageOrder is
 * the inverse map from uid to id:
 *
 *   uid:  0  1  2  3  4  5  6  7  8  9 10
 *   id:   8  0  1  2  9 10  3  4  5  6  7
 *
 * The mapping and names come directly from the retail StageOrder table and
 * StageConfig title/uid fields at 0x8008ea78 and 0x80011f18.  Keeping the two
 * enums separate prevents a numeric stage_id such as 8 (Training) from being
 * mislabeled with stage_uid 8's name (Cure the Princess). */
typedef s32 stage_id;
typedef u8 compact_stage_id;
typedef s16 packed_stage_id;

enum stage_id
{
    STAGE_ID_EVIL_MERCHANT = 0,
    STAGE_ID_SECRET_MESSAGE = 1,
    STAGE_ID_CAPTIVE_NINJA = 2,
    STAGE_ID_MANJI_CULT = 3,
    STAGE_ID_PIRATES = 4,
    STAGE_ID_CURE_PRINCESS = 5,
    STAGE_ID_RECLAIM_CASTLE = 6,
    STAGE_ID_FREE_PRINCESS = 7,
    STAGE_ID_TRAINING = 8,
    STAGE_ID_CHECKPOINT = 9,
    STAGE_ID_CORRUPT_MINISTER = 10,
    N_STAGE_CONFIGS = STAGE_ID_CORRUPT_MINISTER + 1
};

typedef u8 stage_uid;
enum stage_uid
{
    STAGE_UID_TRAINING = 0,
    STAGE_UID_EVIL_MERCHANT = 1,
    STAGE_UID_SECRET_MESSAGE = 2,
    STAGE_UID_CAPTIVE_NINJA = 3,
    STAGE_UID_CHECKPOINT = 4,
    STAGE_UID_CORRUPT_MINISTER = 5,
    STAGE_UID_MANJI_CULT = 6,
    STAGE_UID_PIRATES = 7,
    STAGE_UID_CURE_PRINCESS = 8,
    STAGE_UID_RECLAIM_CASTLE = 9,
    STAGE_UID_FREE_PRINCESS = 10,
    N_STAGE_UIDS = STAGE_UID_FREE_PRINCESS + 1,
    N_CAMPAIGN_MISSIONS = N_STAGE_UIDS - 1
};

/* Asset/table rows with a leading sentinel use the one-based stage number. */
#define STAGE_NUMBER(id) ((id) + 1)
#define NEXT_STAGE_UID(uid) ((uid) + 1)

/* mission_flags dedicates one completion bit to each non-training uid.
 * Retail also raises the following condition-specific bit after finishing
 * the final stage in English; its consumer lives outside main.exe. */
typedef u32 mission_progress_flags;
#define MISSION_COMPLETION_FLAG(uid) \
    (1 << ((uid) - STAGE_UID_EVIL_MERCHANT))
enum mission_progress_flag
{
    MISSION_FLAG_ENGLISH_FINAL_STAGE = 1 << N_CAMPAIGN_MISSIONS
};

/* Each stage supplies a two-way reinforcement choice. The alarm reaction
 * indexes it by stage and coin flip; Think3callaid walks the same storage as
 * a flat signed-halfword table. */
enum
{
    N_STAGE_REINFORCEMENT_CHOICES = 2
};

typedef struct StageReinforcementTypes StageReinforcementTypes;
struct StageReinforcementTypes
{
    character_kind type[N_STAGE_REINFORCEMENT_CHOICES];
}; /* 0x04 */

typedef union ReinforcementTypeTable ReinforcementTypeTable;
union ReinforcementTypeTable
{
    StageReinforcementTypes stage[N_STAGE_CONFIGS];
    character_kind flat[N_STAGE_CONFIGS * N_STAGE_REINFORCEMENT_CHOICES];
}; /* 0x2C */

/* Each stage has three authored layouts. A saved value outside that range
 * asks CreateStage to choose one at random; menus use the byte sentinel. */
#define N_STAGE_LAYOUTS 3
#define STAGE_LAYOUT_RANDOM 0xFF

/* STAGE.C's stage metadata and starting transform. */
typedef struct TStageConfig TStageConfig;
struct TStageConfig
{
    stage_uid uid; /* 0x00 campaign order; StageOrder maps it back to an id */
    u8 *name; /* 0x04 */
    u8 *path; /* 0x08 */
    s32 px;   /* 0x0C */
    s32 py;   /* 0x10 */
    s32 pz;   /* 0x14 */
    s32 pr;   /* 0x18 */
}; /* 0x1C */

/* AUDIO.C's loaded VAB handle. */
typedef s16 vab_id;
#define VAB_ID_AUTO (-1)
#define VAB_ID_ERROR (-1)

/* PsyQ VAB header tables recovered from PSX.SYM. */
typedef struct VabHdr VabHdr;
struct VabHdr
{
    s32 form;      /* 0x00 */
    s32 ver;       /* 0x04 */
    s32 id;        /* 0x08 */
    u32 fsize;     /* 0x0C */
    u16 reserved0; /* 0x10 */
    u16 ps;        /* 0x12 */
    u16 ts;        /* 0x14 */
    u16 vs;        /* 0x16 */
    u8 mvol;       /* 0x18 */
    u8 pan;        /* 0x19 */
    u8 attr1;      /* 0x1A */
    u8 attr2;      /* 0x1B */
    u32 reserved1; /* 0x1C */
}; /* 0x20 */

typedef struct ProgAtr ProgAtr;
struct ProgAtr
{
    u8 tones;      /* 0x00 */
    u8 mvol;       /* 0x01 */
    u8 prior;      /* 0x02 */
    u8 mode;       /* 0x03 */
    u8 mpan;       /* 0x04 */
    u8 reserved0;  /* 0x05 */
    s16 attr;      /* 0x06 */
    u32 reserved1; /* 0x08 */
    u32 reserved2; /* 0x0C */
}; /* 0x10 */

typedef struct VagAtr VagAtr;
struct VagAtr
{
    u8 prior;        /* 0x00 */
    u8 mode;         /* 0x01 */
    u8 vol;          /* 0x02 */
    u8 pan;          /* 0x03 */
    u8 center;       /* 0x04 */
    u8 shift;        /* 0x05 */
    u8 min;          /* 0x06 */
    u8 max;          /* 0x07 */
    u8 vibW;         /* 0x08 */
    u8 vibT;         /* 0x09 */
    u8 porW;         /* 0x0A */
    u8 porT;         /* 0x0B */
    u8 pbmin;        /* 0x0C */
    u8 pbmax;        /* 0x0D */
    u8 reserved1;    /* 0x0E */
    u8 reserved2;    /* 0x0F */
    u16 adsr1;       /* 0x10 */
    u16 adsr2;       /* 0x12 */
    s16 prog;        /* 0x14 */
    s16 vag;         /* 0x16 */
    s16 reserved[4]; /* 0x18 */
}; /* 0x20 */

enum vab_table_dimension
{
    VAB_PROGRAM_ATTRIBUTE_COUNT = 128,
    VAB_TONES_PER_PROGRAM = 16,
    VAB_OFFSET_TABLE_ENTRY_COUNT = 256,
    VAB_TONE_ATTRIBUTE_SHIFT = 9
};

#define VAB_TONE_ATTRIBUTE_BYTES_PER_PROGRAM \
    (VAB_TONES_PER_PROGRAM * sizeof(VagAtr))
#define VAB_FIXED_METADATA_SIZE                                      \
    (sizeof(VabHdr) + VAB_PROGRAM_ATTRIBUTE_COUNT * sizeof(ProgAtr) + \
     VAB_OFFSET_TABLE_ENTRY_COUNT * sizeof(u16))

typedef struct SoundEffect SoundEffect;
struct SoundEffect
{
    vab_id VABid;      /* 0x00 */
    s16 program;       /* 0x02 */
    VabHdr *VABhead;   /* 0x04 */
}; /* 0x08 */

/* INFOVIEW.C's active life-bar slot (anonymous in PSX.SYM). */
typedef struct LifeBarEntry LifeBarEntry;
struct LifeBarEntry
{
    struct Humanoid *target; /* 0x00 */
    s32 life;                /* 0x04 */
    s32 max;                 /* 0x08 */
    s32 count;               /* 0x0C */
    s32 style;               /* 0x10 */
}; /* 0x14 */

/* Retail adds one slot to the demo's original nLifeBar = 4 pool. */
enum
{
    nLifeBar = 5
};

/* Retail's redesigned INFOVIEW.C life-bar style. */
enum
{
    nLifeBarStyle = 2
};

typedef struct TLifeBarStyle TLifeBarStyle;
struct TLifeBarStyle
{
    u16 base;       /* 0x00 */
    s16 scale;      /* 0x02 */
    s16 dx;         /* 0x04 */
    s16 dy;         /* 0x06 */
    GsSPRITE frame; /* 0x08 */
    GsSPRITE fill;  /* 0x2C */
}; /* 0x50 */

/* HUMAN.C/WORLD.C's waypoint path. Each point steers the AI walk:
 * range is the arrival radius, and pad holds extra PAD buttons OR'd
 * into the synthesized input from that point on (ControlTraceLine —
 * how patrol routes make a guard crouch or run on a segment);
 * TRACE_POINT_END terminates the list and restarts the patrol at
 * index 0. */
typedef s16 trace_pad;
#define TRACE_POINT_END (-1)

typedef struct TracePoint TracePoint;
struct TracePoint
{
    s32 x;        /* 0x00 */
    s32 z;        /* 0x04 */
    s16 range;    /* 0x08 */
    trace_pad pad; /* 0x0A */
}; /* 0x0C */

typedef struct TraceLine TraceLine;
struct TraceLine
{
    s16 index;         /* 0x00 */
    s16 count;         /* 0x02 */
    TracePoint *point; /* 0x04 */
}; /* 0x08 */

/* EFFECT.C's draw-mode-plus-flat-quad primitive. */
typedef struct POLY_XF4 POLY_XF4;
struct POLY_XF4
{
    DR_TPAGE tpage; /* 0x00 */
    POLY_F4 ply;    /* 0x08 */
}; /* 0x20 */

/* EFFECT.C's draw-mode-plus-Gouraud-quad primitive. */
typedef struct POLY_XG4 POLY_XG4;
struct POLY_XG4
{
    DR_TPAGE tpage; /* 0x00 */
    POLY_G4 ply;    /* 0x08 */
}; /* 0x2C */

enum weapon_kind
{
    NO_WEAPON = 0x00,
    CLAW = 0x01,
    FIST = 0x02,
    JAW = 0x03,
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
    WEAPON_KIND_END = -1,
};

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
    CHARACTER_KIND_END = -1,
};

/* The persistent player state has one row for each selectable protagonist. */
#define N_PLAYABLE_CHARACTERS (AYAME_0 + 1)

/* The character roster is paged by the type's high nibble (type & 0xf0):
 * palace/story 0x00, common guards 0x10, ninja 0x20, Manji cult 0x30,
 * pirates 0x40, tengu 0x50, oni 0x60, undead 0x70, named characters and
 * bosses 0x80, civilians 0x90 (killing these counts FriendHits), beasts and
 * summons 0xa0. */
typedef enum character_page character_page;
/* Humanoid.type packs the character family ("page") in the high nibble
 * and the roster index within it in the low nibble; PAGE_MASK extracts
 * the family. */
#define PAGE_MASK 0xf0

enum character_page
{
    PAGE_PALACE = 0x00,
    PAGE_GUARD = 0x10,
    PAGE_NINJA = 0x20,
    PAGE_MANJI = 0x30,
    PAGE_PIRATE = 0x40,
    PAGE_TENGU = 0x50,
    PAGE_ONI = 0x60,
    PAGE_UNDEAD = 0x70,
    PAGE_BOSS = 0x80,
    PAGE_CIVILIAN = 0x90,
    PAGE_BEAST = 0xa0
};

/* Humanoid.status is the same 18-way family index as the motion-id high
 * byte: both index the Act* handler table at 0x80086b24, so the names below
 * mirror the demo's own handler names (STAT_SYURI = shuriken aiming,
 * STAT_STATE = ActSTATE's pick/drop/fall transitions, STAT_DAMAGE = the
 * recovery/stagger state, ...).  This retires the old descriptive guesses
 * (ATTACKING, PRESSED_AGAINST_WALL, ...), which matched these values 1:1. */
/* Humanoid stores this domain in a signed halfword. */
enum character_status
{
    STAT_NORMAL = 0x00,
    STAT_ACTION = 0x01,
    STAT_MOVE = 0x02,
    STAT_SWIM = 0x03,
    STAT_KAGI = 0x04,
    STAT_ENGAGE = 0x05,
    STAT_CHASE = 0x06,
    STAT_ATTACK = 0x07,
    STAT_STATE = 0x08,
    STAT_JUMP = 0x09,
    STAT_HANG = 0x0a,
    STAT_SQUAT = 0x0b,
    STAT_STICKON = 0x0c,
    STAT_CEILHANG = 0x0d,
    STAT_SYURI = 0x0e,
    STAT_ITEM = 0x0f,
    STAT_DAMAGE = 0x10,
    STAT_DEAD = 0x11
};

#define N_CHARACTER_STATUSES (STAT_DEAD + 1)

/* Named values for the fixed-width stage_rank storage type above. */
enum stage_rank
{
    RANK_THUG = 0x00,
    RANK_NOVICE = 0x01,
    RANK_NINJA = 0x02,
    RANK_MASTER_NINJA = 0x03,
    RANK_GRAND_MASTER = 0x04,
};

#define N_STAGE_RANKS (RANK_GRAND_MASTER + 1)
#define N_HIGH_SCORES 5 /* leaderboard rows, independently also five */

/* Difficulty: the persistent-state byte at 0x80010058 (symbol gNannido).
 * Official name from the demo PSX.SYM: WORLD.C's cross-exe config struct
 * (typedef TLinkInfo) carries `unsigned char Nannido` at +0x5 and the demo
 * exe keeps a standalone `unsigned char gNannido` (0x80098090); "nannido"
 * is Japanese for difficulty. Retail moved it into the 0x80010000 blob;
 * the neighbouring bytes keep TLinkInfo's field run (0x5A SoundLevel ->
 * CD volume in apply_cd_volume_, 0x5B SELevel -> PlaySE scale, 0x5D Anakon ->
 * PadShock gate). The demo has NO enum for the values -- the original does
 * arithmetic on the raw byte (`EngageLevel = 3 - pt[0x58]` in
 * SetupAppearance, `rand() % 4 - 2 >= gNannido` in StateTransition) -- so
 * these member names are ours. 0=easy proven by FileOption pairing
 * gNannido=0 with EngageLevel=3 (heaviest attack veto, no NPC auto-guard)
 * and gNannido=2 with EngageLevel=1 (no veto, densest attack cadence,
 * 600-frame alerts). */
/* Difficulty is persisted and exported as a single byte. */
typedef u8 game_difficulty;
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
typedef s16 item_selection;
/* SelectedItem's "nothing selected" sentinel (invented name). A #define, not
 * an enumerator: a negative member would sign the enum and flip the item
 * switches' unsigned jump-table bounds checks (sltiu -> slti; measured). */
#define ITEM_NONE (-1)

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
    ITEM_NINGYO = 0x0e, /* debug menu: "rikimarukochan" */
    ITEM_HAPPOU = 0x0f,
    ITEM_NINKEN = 0x10,
    ITEM_KAENGEKI = 0x11,
    ITEM_MANEBUE = 0x12,
    ITEM_ARMOUR = 0x13,
    ITEM_GUN = 0x14,
    ITEM_ARROW = 0x15,
    ITEM_NAPALM = 0x16,
    ITEM_LIGHTNINGBOLT = 0x17,
    ITEM_TELEPORT = 0x18, /* debug menu: "the world" */
    /* Kind count — and retail reuses the item[ITEM_N] inventory slot as
     * the aimed-projectile flag (ProcKaginawa's hook flag; item.h sizes
     * Humanoid.item[] to 0x1A to cover it). The demo enum had a dedicated
     * ITEM_SYSFLAG kind for that role, which retail folded away (it also
     * inserted ITEM_ARMOUR and swapped GOSHIKIMAI/NEMURI, so demo and
     * retail item ids diverge from 0x08 up). */
    ITEM_N = 0x19,
};

/* Item-kind storage plus the system/aiming flag kept at item[ITEM_N]. */
#define N_ITEM_SLOTS (ITEM_N + 1)

/* The pre-mission shop and carried loadout stop at armour. The five item
 * kinds after it are combat/projectile effects, not selectable stock. */
#define N_LOADOUT_ITEMS (ITEM_ARMOUR + 1)

/* Stock markers shared by TLinkInfo.gItem and Humanoid.item[]: a locked
 * (not yet earned) special item, and the infinite-ammo count. */
/* Stride of the persistent state's per-character item arrays. It is not
 * the item-kind count (ITEM_N, 0x19) nor the world item-pool size
 * (MAX_ITEMS, 30): it is a padded slot count, 30 in the demo and
 * rounded to 32 in retail -- which is also why SAVE_ITEM_ROW_OFFSET is a
 * five-bit shift. selItem, saveItem and each gItem row are all this wide. */
#define SAVE_ITEM_ROW_SHIFT 5
#define SAVE_ITEM_SLOTS (1 << SAVE_ITEM_ROW_SHIFT)
#define SAVE_ITEM_ROW_OFFSET(character) \
    ((character) << SAVE_ITEM_ROW_SHIFT)
#define SAVE_ITEM_INDEX(character, item) \
    ((item) + SAVE_ITEM_ROW_OFFSET(character))

/* Rows in SHOP_ITEM_DEFAULTS: the briefing screen and clamp_shop_stock_
 * both walk the whole table. */
#define N_SHOP_ITEMS 0x13

#define ITEM_LOCKED 0xFE
#define ITEM_INFINITE 0xFF

// The persistent game state blob at 0x80010000 (below the exe image; survives
// across screens). TLinkInfo is the OFFICIAL typedef from the demo PSX.SYM
// (WORLD.C's cross-exe config struct — it "links" state between the separate
// executables); the full demo definition is reference/psxsym-types.h (216
// bytes, CharType short at +0, options at +0x5..+0xE). Retail rearranged and
// extended the layout (0xE70 memset by InitPersistentState, magic 0x19981110
// at +0, options run moved to +0x58, language added, and gItem moved/expanded
// to two character rows), so the demo definition CANNOT be adopted verbatim
// — byte-identity pins the retail offsets below. Original demo member names
// are called out per field;
// lowercase otherwise denotes a retail/repo name. Retail expands the demo's
// `selItem[30]` and `saveItem[30]` arrays to 32 entries while retaining their
// original roles.
// The 0x80010000 instance keeps splat's descriptive symbol name
// PersistentState — the demo names only the type, not the retail instance.
/* Language ids (TLinkInfo.language) — values and names from the game's
 * own debug language menu (DEBUG_MENU_LANGUAGE_CHOICES). */
typedef u8 game_language;
enum game_language
{
    LANG_ENGLISH = 0,
    LANG_FRENCH = 1,
    LANG_ITALIAN = 2,
    LANG_JAPANESE = 3
};

#define N_LANGUAGES (LANG_JAPANESE + 1)

typedef u8 sound_output_mode;
enum sound_output_mode
{
    SOUND_MODE_MONO = 0,
    SOUND_MODE_STEREO = 1
};

// Offsets proven by BriefingAndInventorySelectionScreen.
// Splat also names some fields as standalone globals (CHOSEN_CHARACTER = +4,
// CHOSEN_STAGE = +5, STAGE_LAYOUT_NUMBER = +6, CHOSEN_LANGUAGE = +0x5E,
// SHOP_STOCK_STATE_BY_CHAR = +0x40C); the original source mixed direct global
// accesses with pointer-based ones, so both views coexist on purpose.
typedef struct TLinkInfo
{
    u32 magic;                        /* 0x000 0x19981110 (InitPersistentState) */
    compact_character_kind CharType; /* 0x004 CHOSEN_CHARACTER (stock matrix row;
                                       *       demo +0x0, short) */
    compact_stage_id StageNo;         /* 0x005 CHOSEN_STAGE: StageConfig id
                                       *       (demo +0x2) */
    u8 layout;                        /* 0x006 STAGE_LAYOUT_NUMBER */
    u8 selItem[SAVE_ITEM_SLOTS];      /* 0x007 selected count per item;
                                       *       retail expansion of demo selItem[30] */
    u8 saveItem[SAVE_ITEM_SLOTS];     /* 0x027 loadout backup (restore on abort);
                                       *       retail expansion of demo saveItem[30] */
    u8 analog_pad_present;            /* 0x047 bit0: analog pad detected */
    u8 GameRetry;                     /* 0x048 bit0: retry/continue current stage;
                                       *       original demo member name (+0x0D) */
    ScoreStats score_stats;           /* 0x04C current mission counters */
    game_difficulty Nannido;          /* 0x058 gNannido (demo +0x5) */
    sound_output_mode Stereo;         /* 0x059 gSound: stereo/mono selector
                                       *       (InitSoundEffect/InitPersistentState
                                       *       -> SsSetStereo/SsSetMono; demo +0x7) */
    u8 SoundLevel;                    /* 0x05A gSoundLevel: music/CD volume 0..0x7F
                                       *       (apply_cd_volume_, _PlayMusic; demo +0x8) */
    u8 SELevel;                       /* 0x05B gSELevel: SE volume 0..0x7F
                                       *       (PlaySE, PlayVoice; demo +0x9) */
    u8 fMemory;                       /* 0x05C gfMemory: post-mission memory-card
                                       *       save flow enabled (StageEndScreen /
                                       *       mission_score_screen -> score_screen_input_
                                       *       save UI; demo +0xC) */
    u8 Anakon;                        /* 0x05D analog pad / rumble enabled (PadShock
                                       *       gate, PadProc; demo +0xE; default 1) */
    game_language language;           /* 0x05E CHOSEN_LANGUAGE (retail-only) */
    u8 control_scheme;                /* 0x05F saved pad-remapping row (retail-only) */
    stage_uid StageNoMAX[N_PLAYABLE_CHARACTERS]; /* 0x060 highest campaign
                                                  *       uid per character;
                                                  *       official demo member
                                                  *       name (demo +0x3) */
    ScoreStats
        stage_stats[N_PLAYABLE_CHARACTERS][13][N_STAGE_LAYOUTS]; /* 0x064 */
    u8 gItem[N_PLAYABLE_CHARACTERS][SAVE_ITEM_SLOTS]; /* 0x40C shop stock,
                                                       *       per character;
                                                       *       [CharType][item];
                                                       *       retail expansion
                                                       *       of demo gItem[30];
                                                       *       0xFE = locked,
                                                       *       0xFF = infinite;
                                                       *       [CharType][ITEM_ARMOUR] =
                                                       *       stage bonus flag */
    compact_character_kind t_char[N_HIGH_SCORES]; /* 0x44C high-score character */
    compact_stage_rank t_dani[N_HIGH_SCORES]; /* 0x451 high-score rank */
    long t_time[N_HIGH_SCORES];       /* 0x458 completion time; retail replacement for
                                       *       the demo's t_fun/t_byou byte arrays */
    mission_progress_flags mission_flags; /* 0x46C completion plus the
                                            *       condition-specific flag
                                            *       above */
} TLinkInfo;

/* Raw persistent-state accesses that are required for matching can still
 * derive their displacements from the canonical layout. */
#define TLINKINFO_BYTE_OFFSET(member) ((u32)&((TLinkInfo *)0)->member)
#define TLINKINFO_FLAT_STOCK(state, index) \
    ((&((state)->gItem[0][0]))[index])
#define TLINKINFO_STOCK(state, character, item) \
    TLINKINFO_FLAT_STOCK(state, SAVE_ITEM_INDEX(character, item))
