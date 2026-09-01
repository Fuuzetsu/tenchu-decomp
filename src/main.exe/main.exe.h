#include <psxsdk/libgs.h>
#include "game_types.h"
#include "conflict.h"
#include "humanoid.h"
#include "game_globals.h"
#include "music.h"

/* Commit motID/motMODE to the humanoid -- unless a cutscene (CVA) is
 * currently driving them, in which case the script owns the motion and
 * the caller bails out via `escape` instead. Retail copy-pastes this
 * guard at every damage/death motion commit; the macro is
 * reconstruction shorthand for that copy-paste (expands to the
 * identical text). */
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
extern COMMAND *Command[N_PAD_COMMAND_TABLE_ENTRIES];
/* The two standard PSY-Q pad receive buffers InitPAD registers. */
extern unsigned char ComBuf[PAD_PORT_COUNT][PAD_REPORT_BUFFER_SIZE];
extern PadArrangeType PadArrange;

extern int turn_towards_player_(int x_diff, int z_diff);
extern struct Humanoid *Me_THINK_C;
/* Retail permits 40 actors and reserves 0xA0 bytes before the next global. */
#define MAX_HUMANS 40
extern struct Humanoid *HumanGroup[MAX_HUMANS];
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

/* models.arc slots (GetArcData indices) — official names from the demo
 * debug symbols' MODEL_/ICON_ enum, whose values line up 1:1 with the
 * retail slots the code pins (…SYURIKEN 0x14, ARROW 0x15, CARD1-3,
 * SHADOW 0x19, KAGIHEAD 0x1A, NINGYO 0x1B, HAPPOU 0x1C — so retail
 * kept the demo's archive order; the demo list runs MON6B 0 .. AKINb
 * 0x1E with MODEL_N = 0x1F). Retail appends entries past MODEL_N;
 * ARC_BLOOD_POOL_MODEL is our invented name for one of them. */
enum
{
    MODEL_SYURIKEN = 0x14,
    MODEL_ARROW = 0x15,
    ICON_CARD1 = 0x16,
    ICON_CARD2 = 0x17,
    ICON_CARD3 = 0x18,
    MODEL_SHADOW = 0x19,
    MODEL_KAGIHEAD = 0x1A,
    MODEL_NINGYO = 0x1B,
    MODEL_HAPPOU = 0x1C,
    ARC_BLOOD_POOL_MODEL = 0x1F
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
extern TLifeBarStyle LifeBarStyle[nLifeBarStyle];
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
extern SVECTOR SplineTable[N_SPLINE_BASIS_ROWS];
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
extern void UpdateCoordinate(ModelType *dim);
extern short NowReturnNormal(struct Humanoid *human);
extern short GetMotionID(MotionManager *mmp, motion_id mid);
extern short ActiveMotion(MotionManager *mmp);
extern void eval_spline_gte_(SVECTOR *out, SplineControlType *spc,
                             SVECTOR *basis);
extern long GetTargetDistance(struct Humanoid *human, short *deg);
extern enemy_layout_index leFindEnemy(void);
extern void leLayoutEnemy(int mode);
extern int leRemoveEnemy(void);

extern u_long *FileRead(u8 *filename);
extern u_long *PathFileRead(u8 *resource_prefix, u8 *resource_name);
extern u_long *GetArcData(int index);
extern u_long *get_tim_from_archive(u_long *archive, int idx);
extern short GetTIMInfo(u_long *adr, GsIMAGE *image);
extern short LoadTIM(u_long *adr);
extern void LoadTIMAndFree(u_long *tim);
extern void load_font_image_into_global(GsIMAGE *image);
extern void InitSprite(GsIMAGE *image, GsSPRITE *sprite);
extern void StartDrawing(void);
extern void EndDrawing(short sync);
extern void SystemOut(unsigned char *string);

extern void SetSmoke(VECTOR *pos, SVECTOR *vect, short n, short time);
extern void SetImpact(VECTOR *pos, short size, short type);
extern void SetSplash(VECTOR *pos, short sx, short sy, int speed);
extern void SetBleed(VECTOR *pos, SVECTOR *vec, int time, long col);
extern void SetBleeds(VECTOR *pos, short grange, short srange, short n,
                      int time, long col);
extern void SetBleedsDir(VECTOR *pos, SVECTOR *vec, short grange, short n,
                         int time, long col);
extern void SetFrame(VECTOR *pos, short size, short time,
                     GsCOORDINATE2 *super);
extern void GetVectorRotation(VECTOR *start, VECTOR *end, int *rx, int *ry);
extern void SetExplosion(VECTOR *pos, SVECTOR *vect);
extern void SetHinoko(VECTOR *pos, SVECTOR *power, int n);
extern void SetLightning(VECTOR *start, VECTOR *end,
                         short r, short g, short b);
extern int SetFlyWire(VECTOR *start, VECTOR *end);
extern void SetWire(VECTOR *start, VECTOR *end, VECTOR *center, long len);
extern void RotateVector(VECTOR *vec, int rx, int ry, int rz);
extern void RotateVectorS(SVECTOR *vec, int rx, int ry, int rz);
extern int GetVectorDistance(VECTOR *v1, VECTOR *v2);
extern s32 trace_ground_(VECTOR *from, VECTOR *to, VECTOR *out, u32 flag);
extern void GetScreenPosition(long x, long y, long z, SVECTOR *scr);
