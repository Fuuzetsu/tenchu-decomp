#include <psxsdk/libgs.h>
#include "game_types.h"
#include "conflict.h"
#include "humanoid.h"
#include "game_globals.h"
#include "music.h"

/* Commit motID/motMODE to the humanoid unless a cutscene (CVA) currently
 * drives it, in which case the caller leaves through `escape`. */
#define SET_NOW_MOTION_UNLESS_CVA(escape)                                     \
    {                                                                         \
        short i;                                                              \
                                                                              \
        if (MotionUpdateMode != 0)                                            \
        {                                                                     \
            for (i = 0; i < N_CVA_HUMANS; i++)                                \
            {                                                                 \
                if (CVAhuman[i].human == Me_MOTION_C)                         \
                {                                                             \
                    escape;                                                   \
                }                                                             \
            }                                                                 \
        }                                                                     \
        SetNowMotion(Me_MOTION_C, motID, motMODE);                            \
        motMODE = MOTION_MOVE_UNSET;                                          \
    }

/* Clamp a >>2 screen depth into [0, DEPTH_LIMIT - 1] for the OT sort;
 * the copy-paste block every sprite-effect renderer carries (macro is
 * reconstruction shorthand, expands to the identical text). */
#define CLAMP_SORT_DEPTH(pri, z)                                              \
    if ((z) >= 0)                                                             \
    {                                                                         \
        (pri) = DEPTH_LIMIT - 1;                                              \
        if ((z) < DEPTH_LIMIT)                                                \
        {                                                                     \
            (pri) = (z);                                                      \
        }                                                                     \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        (pri) = 0;                                                            \
    }

#include "ram_layout.h"

extern void PadProc(void);
extern void PadShockAR(int port, int pow, int attack, int release);
extern long GetRealPad(int port);
extern TPadPort PadPort[PAD_PORT_COUNT][PAD_SLOTS_PER_PORT];
extern PadCommandSequence *Command[N_PAD_COMMAND_TABLE_ENTRIES];
/* The two standard PSY-Q pad receive buffers InitPAD registers. */
extern unsigned char ComBuf[PAD_PORT_COUNT][PAD_REPORT_BUFFER_SIZE];
extern PadArrangeType PadArrange;

extern s16 GotoPosition(s32 vx, s32 vz);
/* Retail permits 40 actors and reserves 0xA0 bytes before the next global. */
#define MAX_HUMANS 40
extern struct Humanoid *HumanGroup[MAX_HUMANS];
/* The per-frame render roster is parallel to HumanGroup and has the same
 * retail capacity: one visible actor pointer and saved TMD draw mode per
 * slot, with VISIBLE_ENEMIES_ entries live. */
extern s16 VISIBLE_ENEMIES_;
extern s16 DrawModeSave[MAX_HUMANS];
extern struct Humanoid *VISIBLE_CHARACTERS_ON_STAGE_[MAX_HUMANS];
/* Retail's CHARACTER_KIND_END row is entry 77; the demo table had 63. */
extern HumanDataType HumanData[78];
extern HumanAnimType CVAhuman[N_CVA_HUMANS];
extern SVECTOR UnitVector;
extern VECTOR UnitVector2;
/* Unit x/z push per 4-bit probe direction code (retail data: 0/±1
 * pairs) — FallCheck and DefaultActionHumanoid nudge the character
 * away from the coded edge by width*RefrectMove[code]/4. Sibling of
 * RefrectVector below. */
extern short RefrectMove[N_MAP_PROBE_MASKS][2];
extern facing_angle RefrectVector[N_MAP_PROBE_MASKS];
extern TCameraStatus CamState;
/* Retail expands the demo's three-entry table to four camera placements. */
extern TCameraPos CamPosCriticalHit[N_CRITICAL_CAMERA_POSITIONS];
extern TCameraPos CamPos;
extern TCameraPos CamPosDefault;
/* The GPU has four semi-transparency equations. GsSPRITE attributes and
 * DR_TPAGE packets encode this same value in different bit positions; keep
 * the equation itself representation-independent. */
enum gpu_blend_mode
{
    GPU_BLEND_AVERAGE = 0,     /* back / 2 + front / 2 */
    GPU_BLEND_ADD = 1,         /* back + front */
    GPU_BLEND_SUBTRACT = 2,    /* back - front */
    GPU_BLEND_ADD_QUARTER = 3, /* back + front / 4 */
    GPU_BLEND_MODE_MASK = 3
};

/* A DR_TPAGE-style mode word: GP0 command 0xE1 (draw mode) with dithering
 * on. Its bits 5-6 carry gpu_blend_mode. */
#define GPU_DRAWMODE_DITHER 0xE1000200
#define GPU_DRAWMODE_BLEND(mode) (((mode) & GPU_BLEND_MODE_MASK) << 5)

extern GsRVIEW2 ViewInfo;

/* GsRVIEW2 keeps its viewpoint and reference coordinates as scalar triples;
 * the game's vector helpers consume the addresses of those triples. */
#define CAMERA_VIEWPOINT(view_) ((VECTOR *)(view_))
#define CAMERA_REFERENCE(view_) ((VECTOR *)&(view_)->vrx)

/* Keep each point-projection member address independently materialized.
 * Directly caching one ScreenProjectionWorkspace pointer makes cc1 fold later
 * members into load displacements, unlike retail's repeated absolute
 * scratchpad addresses. */
#define SCREEN_PROJECTION_BYTE_OFFSET(member)                         \
    ((u32)&((ScreenProjectionWorkspace *)0)->member)
#define SCREEN_PROJECTION_ADDRESS(member)                             \
    TENCHU_SCRATCHPAD(SCREEN_PROJECTION_BYTE_OFFSET(member))
#define SCREEN_PROJECTION_MATRIX                                      \
    ((MATRIX *)SCREEN_PROJECTION_ADDRESS(local_screen))
#define SCREEN_PROJECTION_TRANSLATION_X                               \
    ((s32 *)SCREEN_PROJECTION_ADDRESS(local_screen.t[0]))
#define SCREEN_PROJECTION_TRANSLATION_Y                               \
    ((s32 *)SCREEN_PROJECTION_ADDRESS(local_screen.t[1]))
#define SCREEN_PROJECTION_TRANSLATION_Z                               \
    ((s32 *)SCREEN_PROJECTION_ADDRESS(local_screen.t[2]))
#define SCREEN_PROJECTION_POINT                                       \
    ((SVECTOR *)SCREEN_PROJECTION_ADDRESS(point))
#define SCREEN_PROJECTION_POINT_X                                     \
    ((s16 *)SCREEN_PROJECTION_ADDRESS(point.vx))
#define SCREEN_PROJECTION_POINT_Y                                     \
    ((s16 *)SCREEN_PROJECTION_ADDRESS(point.vy))
#define SCREEN_PROJECTION_POINT_Z                                     \
    ((s16 *)SCREEN_PROJECTION_ADDRESS(point.vz))
#define SCREEN_PROJECTION_PERSPECTIVE                                 \
    ((s32 *)SCREEN_PROJECTION_ADDRESS(perspective))
#define SCREEN_PROJECTION_FLAG                                        \
    ((s32 *)SCREEN_PROJECTION_ADDRESS(flag))

/* Keep these as independently materialized member addresses: IsVisible
 * caches the view and rotated-result pointers at different times. */
#define CONSTRUCTION_VISIBILITY_BYTE_OFFSET(member)                    \
    ((u32)&((ConstructionVisibilityWorkspace *)0)->member)
#define CONSTRUCTION_VISIBILITY_ADDRESS(member)                        \
    TENCHU_SCRATCHPAD(CONSTRUCTION_VISIBILITY_BYTE_OFFSET(member))
#define CONSTRUCTION_VISIBILITY_VIEW_SPACE                              \
    ((VECTOR *)CONSTRUCTION_VISIBILITY_ADDRESS(view_space))
#define CONSTRUCTION_VISIBILITY_RELATIVE                                \
    ((SVECTOR *)CONSTRUCTION_VISIBILITY_ADDRESS(relative))
#define CONSTRUCTION_VISIBILITY_VIEW                                    \
    ((GsRVIEW2 *)CONSTRUCTION_VISIBILITY_ADDRESS(view))
extern ModelType World;
extern WorldType WorldMap[WORLD_MAP_AXIS_SIZE][WORLD_MAP_AXIS_SIZE][WORLD_MAP_AXIS_SIZE];
#define MAX_ENEMIES 30
/* Layout save-file sections (the names FileOption's writer uses): the
 * packed enemy layout, then the packed item layout. */
enum
{
    ENESIZE = 5000,
    ITEMSIZE = 2000
};

/* images.arc slots consumed by GetImage. The demo symbols provide the
 * original asset vocabulary; retail's 62-entry archive, its TIM artwork,
 * and the tables that feed InitEffect/InitializeInfoView establish the new
 * ordering. In particular, item icons occupy one slot per selectable
 * TItemType, and each life-bar style owns a frame/fill pair. */
typedef enum ImageArchiveId ImageArchiveId;
enum ImageArchiveId
{
    IMG_BLOOD_FLY_0 = 0,
    IMG_BLOOD_STAY_0 = 1,
    IMG_BLOOD_FLY_1 = 2,
    IMG_BLOOD_STAY_1 = 3,
    IMG_BLOOD_FLY_2 = 4,
    IMG_BLOOD_STAY_2 = 5,
    IMG_SMOKE = 6,
    IMG_BOMB0 = 7,
    IMG_BOMB1 = 8,
    IMG_BOMB2 = 9,
    IMG_AFTERIMAGE = 10,
    IMG_GUARD = 11,
    IMG_HIT = 12,
    IMG_GOSHIKIMAI = 13,
    IMG_SPLASH = 14,
    IMG_GUNFIRE = 15,
    IMG_PLAYER_LIFEBAR_FRAME = 16,
    IMG_PLAYER_LIFEBAR_FILL = 17,
    IMG_ENEMY_LIFEBAR_FRAME = 18,
    IMG_ENEMY_LIFEBAR_FILL = 19,
    IMG_ICON_KAGINAWA = 20,
    IMG_ICON_SHURIKEN = 21,
    IMG_ICON_MAKIBISHI = 22,
    IMG_ICON_KUSURI = 23,
    IMG_ICON_FIRE = 24,
    IMG_ICON_SMOKE = 25,
    IMG_ICON_JIRAI = 26,
    IMG_ICON_DOKUDANGO = 27,
    IMG_ICON_GOSHIKIMAI = 28,
    IMG_ICON_NEMURI = 29,
    IMG_ICON_KAWARIMI = 30,
    IMG_ICON_HENSHIN = 31,
    IMG_ICON_GOSIN = 32,
    IMG_ICON_SHINSOKU = 33,
    IMG_ICON_NINGYO = 34,
    IMG_ICON_HAPPOU = 35,
    IMG_ICON_NINKEN = 36,
    IMG_ICON_KAENGEKI = 37,
    IMG_ICON_MANEBUE = 38,
    IMG_ICON_ARMOUR = 39,
    IMG_FRAME0 = 40,
    IMG_FRAME1 = 41,
    IMG_FRAME2 = 42,
    IMG_FRAME3 = 43,
    IMG_LOADING = 44,
    IMG_TENCHU = 45,
    IMG_KEHAI_GREEN = 46,
    IMG_KEHAI_YELLOW = 47,
    IMG_KEHAI_RED = 48,
    IMG_KEHAI_CRITICAL = 49,
    IMG_CURSOR = 50,
    IMG_FONT_NUMBER = 51,
    IMG_SIGHT = 52,
    IMG_MISC_FIRE1 = 53,
    IMG_MISC_FIRE2 = 54,
    IMG_MISC_SNOW = 55,
    IMG_BLOOD_FLY_3 = 56,
    IMG_BLOOD_STAY_3 = 57,
    IMG_SMOKE_ALT = 58,
    IMG_SHINSOKU = 59,
    IMG_GOSIN = 60,
    IMG_PAUSE = 61,
    N_IMAGES = 62
};

/* models.arc slots consumed by GetArcData. The demo symbols supply the
 * original names through MODEL_AKINb, and retail's door/pitfall tables and
 * direct callers retain that ordering. MODEL_N was the demo's end marker;
 * retail reuses its slot for the blood pool and appends a one-piece pitfall
 * model. The two retail-only names describe their observed consumers. */
typedef enum ModelArchiveId ModelArchiveId;
enum ModelArchiveId
{
    MODEL_ARCHIVE_NONE = -1,
    MODEL_MON6B = 0,
    MODEL_MON6A = 1,
    MODEL_DOORZ00 = 2,
    MODEL_DOORZ01 = 3,
    MODEL_MON5B = 4,
    MODEL_MON5A = 5,
    MODEL_MONB = 6,
    MODEL_MONA = 7,
    MODEL_MON2B = 8,
    MODEL_MON2A = 9,
    MODEL_MON3B = 10,
    MODEL_MON3A = 11,
    MODEL_MON01 = 12,
    MODEL_MON00 = 13,
    MODEL_GMON01 = 14,
    MODEL_GMON00 = 15,
    MODEL_AKI01 = 16,
    MODEL_AKI00 = 17,
    MODEL_OTO_L = 18,
    MODEL_OTO_R = 19,
    MODEL_SYURIKEN = 20,
    MODEL_ARROW = 21,
    ICON_CARD1 = 22,
    ICON_CARD2 = 23,
    ICON_CARD3 = 24,
    MODEL_SHADOW = 25,
    MODEL_KAGIHEAD = 26,
    MODEL_NINGYO = 27,
    MODEL_HAPPOU = 28,
    MODEL_AKINa = 29,
    MODEL_AKINb = 30,
    MODEL_BLOOD_POOL = 31,
    MODEL_SINGLE_PITFALL = 32
};

/* exec_process_ ids — which PS-X EXE boots next ("bad process id" is
 * the game's own error message; the value names are invented). */
enum
{
    PROCESS_MENU = 0x10,
    PROCESS_MAIN = 0x11,
    PROCESS_ENDING = 0x12,
    PROCESS_TRIAL = 0x13
};

extern TEnemyLayout enemy[MAX_ENEMIES];
extern TStageConfig StageConfig[N_STAGE_CONFIGS];
/* StageOrder converts campaign uid to runtime stage id; StageItem maps each
 * runtime stage to its Grand Master reward. Both retail tables contain one
 * signed halfword per member of their respective stage domain. */
extern packed_stage_id StageOrder[N_STAGE_UIDS];
extern item_selection StageItem[N_STAGE_CONFIGS];
extern MotionManager *dtM;
extern VECTOR *dtL;
extern SVECTOR *dtR;
extern SVECTOR *dtV;
extern pad_command dtCMD;
extern short MotionUpdateMode;
extern item_selection SelectedItem;
extern TCdaStatus CdaStatus;
/* INFOVIEW.C's shared UI and layout-editor state. */
extern short ItemCursor;
/* PutMapMode: DoInfoViewProc arms the overlay by resetting it to OPEN;
 * PutMap plays the open sound and slides the map in from the right, then
 * parks in SHOWN until the view is closed again. */
enum
{
    PUTMAP_OPEN = 0,
    PUTMAP_SLIDE_IN = 1,
    PUTMAP_SHOWN = 2
};

extern unsigned char PutMapMode;
extern enemy_layout_index CurrentEnemyID; /* enemy[] index latched by leFindEnemy */
extern GsSPRITE CursorImage;
extern GsSPRITE NumberImage;
/* Retail groups the demo's three named Kehai sprites with one new state. */
#define N_KEHAI_IMAGES 4
extern GsSPRITE KehaiImage[N_KEHAI_IMAGES];
#define KehaiGreenImage (KehaiImage[0])
#define KehaiYellowImage (KehaiImage[1])
#define KehaiRedImage (KehaiImage[2])
/* Descriptive name: this fourth, extreme-state sprite is retail-only. */
#define KehaiCriticalImage (KehaiImage[N_KEHAI_IMAGES - 1])
extern GsSPRITE MapImage;
extern LifeBarEntry LifeBar[nLifeBar];
extern TLifeBarStyle LifeBarStyle[N_LIFE_BAR_STYLES];
extern void PutItemIcon(int ItemID, short x, short y, short scale);
extern void PutItemCursor(short x, short y, short size, int rotdif);
extern GsOT *OTablePt;
/* Double-buffered GPU ordering tables and 64 KiB packet arenas. */
#define N_DRAW_PAGES 2
#define OT_LENGTH 11
#define N_OT_TAGS (1 << OT_LENGTH)
#define PACKET_PAGE_SHIFT 16
#define PACKET_PAGE_SIZE (1 << PACKET_PAGE_SHIFT)
extern GsOT OTable[N_DRAW_PAGES];
extern GsFOGPARAM Fog;
extern short DrawingPage;
/* Retail's draw-mode object is word-sized; one caller snapshots its low half. */
/* Retail's WEAPON_KIND_END row is entry 30; the demo table had 28. */
extern WeaponType WeaponDB[31];
/* Retail's WEAPON_KIND_END row is entry 47; the demo table had 41. */
extern WeaponModelType WeaponModel[48];
extern MotionRegistType MOTcommon[41];
extern MotionPackType *MotionPack;
extern MotionPackType *CommonMotion;
extern MotionPackType *PlayerMotion;
extern MotionPackType *StageMotion;
/* ACTION.C-private in the demo; externally linked while its storage is raw. */
extern SVECTOR HermiteTable[N_SPLINE_BASIS_ROWS];
extern s16 SplineFracOld;
extern s16 SplineFrac;
extern SVECTOR *SplineRow;
/* One sprite/model pointer for every carried-item storage slot. */
extern Sprite3D *ItemImage[N_ITEM_SLOTS];
extern SoundEffect *StageSE;
extern short VoiceMode;
extern CVAType *CVAdata;
extern CVAType *CVAnow;
extern short CVAtime;
extern struct Humanoid *CameraTarget;
extern short CameraSpeed;
extern camera_pan_mode CameraPanMode;
extern POLY_FT4 TelopP;
extern POLY_F4 TelopbgP;
/* CONFLICT.C's retail-expanded pool and query outputs. */
extern ConflictObjectType ConflictObject[N_CONFLICT_OBJECTS];
extern s16 ConflictObjects;
extern SVECTOR ConflictDistance;
extern ModelType *ConflictModel;
extern long EmergencyNotice;
extern AreaMapType *BalmaAreaMap;
extern AreaNodeType *FieldArea;
extern NodeIndexType *FieldIndex;
extern char FONT_FILE_NAME;
extern char IMAGES_PREFIX_STR;
extern unsigned long *MemoryLoadAddress;
extern unsigned char *ImagePath;
extern int AccessPower;
extern int TotalIO;
extern POLY_GT4 AccessImage;

extern void SetCameraMode(TCameraMode mode);
extern void SetupSoundEffect(character_kind character, short stage);
extern short NowReturnNormal(struct Humanoid *human);
extern void eval_spline_gte_(SVECTOR *out, SplineControlType *spc,
                             SVECTOR *basis);
extern long GetTargetDistance(struct Humanoid *human, short *deg);
extern enemy_layout_index leFindEnemy(void);
extern void leLayoutEnemy(enemy_layout_mode mode);
extern int leRemoveEnemy(void);

extern u_long *FileRead(u8 *filename);
extern u_long *PathFileRead(u8 *resource_prefix, u8 *resource_name);
extern u_long *GetArcData(int index);
extern u_long *get_tim_from_archive(ArcFile *archive, int idx);
extern void load_font_image_into_global(GsIMAGE *image);
