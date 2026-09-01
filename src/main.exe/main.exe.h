#include <psxsdk/libgs.h>
#include "game_types.h"
#include "conflict.h"
#include "humanoid.h"
#include "game_globals.h"

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
extern COMMAND *Command[12];
/* The two standard 34-byte PSY-Q pad receive buffers InitPAD registers. */
extern unsigned char ComBuf[2][34];
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
/* A DR_TPAGE-style mode word: GP0 command 0xE1 (draw mode) with dithering
 * on. Callers OR the semi-transparency mode into bits 5-6. */
#define GPU_DRAWMODE_DITHER 0xE1000200

extern GsRVIEW2 ViewInfo;

/* Every routine that projects a single world point borrows the same corner of
 * the scratchpad for its GTE work frame: a MATRIX at 0 whose rotation comes
 * from GsWSMATRIX and whose translation is zeroed, the view-relative SVECTOR
 * fed through it, and RotTransPers's two long out-params. Other functions
 * borrow other corners for unrelated temporaries -- the pad is a scratch
 * arena, not one shared struct. These stay separate integer constants rather
 * than members of a MATRIX/SVECTOR because retail materialises each address
 * as its own lui+ori; a struct spelling folds them into load displacements
 * and does not match (ram_layout.h records the measurement). */
#define SCRATCH_LS 0x00                     /* MATRIX ls */
#define SCRATCH_LS_TX (SCRATCH_LS + 0x14)   /* its long t[3] translation */
#define SCRATCH_LS_TY (SCRATCH_LS + 0x18)
#define SCRATCH_LS_TZ (SCRATCH_LS + 0x1c)
#define SCRATCH_POINT 0x20                  /* the SVECTOR fed through it */
#define SCRATCH_POINT_X (SCRATCH_POINT + 0)
#define SCRATCH_POINT_Y (SCRATCH_POINT + 2)
#define SCRATCH_POINT_Z (SCRATCH_POINT + 4)
#define SCRATCH_RTP_P 0x28                  /* RotTransPers long *p */
#define SCRATCH_RTP_FLAG 0x2c               /* RotTransPers long *flag */
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

/* Music ids the code plays by literal — _PlayMusic's argument is the
 * PHYSICAL MusicTable row. Names are official (the demo CHRANIM enum);
 * script data instead uses the demo-era LOGICAL ids, remapped through
 * MusicIDTable by PlayMusicFormID (row = table index whose byte equals
 * id-100): logical 0-8 = MUSIC_STAGE1..9 keep rows 0-8, rows 9/10/18
 * are retail-new themes (logical 26/27/30), and logical 9-15
 * (GAMEOVER, COMPLETE, KIKI, TITLE, CHARA, BARMAR, MEIOU) sit at rows
 * 11-17. */
#define MUSIC_GAMEOVER 11
#define MUSIC_COMPLETE 12

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
extern short dtCMD;
extern short MotionUpdateMode;
extern short SelectedItem;
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
extern int CurrentEnemyID; /* enemy[] index latched by leFindEnemy */
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
extern s32 DrawTMDmode;
/* Retail's WEAPON_KIND_END row is entry 30; the demo table had 28. */
extern WeaponType WeaponDB[31];
/* Retail's WEAPON_KIND_END row is entry 47; the demo table had 41. */
extern WeaponModelType WeaponModel[48];
extern MotionRegistType MOTcommon[41];
extern MotionPackType *MotionPack;
extern MotionPackType *CommonMotion;
extern MotionPackType *PlayerMotion;
extern MotionPackType *StageMotion;
/* One sprite/model pointer for every carried-item storage slot. */
extern Sprite3D *ItemImage[N_ITEM_SLOTS];
extern SoundEffect *StageSE;
extern short VoiceMode;
extern CVAType *CVAdata;
extern CVAType *CVAnow;
extern short CVAtime;
extern struct Humanoid *CameraTarget;
extern short CameraSpeed;
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
    CAMERA_PAN_TRACK_TARGET = 8
};
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
/* ReadMode packs the file source into its low two bits, plus flags.
 * Bit 3 is set together with the memory source by InitFileSystem's
 * memory-disk handshake and only ever tested as part of that pair, so it
 * has no name here. */
#define READ_SOURCE_MASK 3
#define READ_SOURCE_DEVPC 0  /* files come over the PC link */
#define READ_SOURCE_MEMORY 1 /* files come from the PC memory disk */
#define READ_SOURCE_CDROM 2  /* files come from the disc's AFS archive */
#define READ_MODE_TRACE 4    /* log every load through AdtMessageBox */

extern int ReadMode;
extern int TotalIO;
extern POLY_GT4 AccessImage;

extern void SetCameraMode(TCameraMode mode);
extern void UpdateCoordinate(ModelType *dim);
extern short NowReturnNormal(struct Humanoid *human);
extern short GetMotionID(MotionManager *mmp, motion_id mid);
extern short ActiveMotion(MotionManager *mmp);
extern long GetTargetDistance(struct Humanoid *human, short *deg);
extern int leFindEnemy(void);
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
