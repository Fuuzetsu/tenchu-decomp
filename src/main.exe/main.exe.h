#include <psxsdk/libgs.h>
#include "game_types.h"
#include "conflict.h"
#include "humanoid.h"
#include "game_globals.h"
#include "ram_layout.h"

extern void PadProc(void);
extern void PadShockAR(int port, int pow, int attack, int release);
extern long GetRealPad(int port);
extern TPadPort PadPort[2][4];
extern unsigned short *Command[12];
extern unsigned char ComBuf[2][34];
extern PadArrangeType PadArrange;

extern int turn_towards_player_(int x_diff, int z_diff);
extern struct Humanoid *Me_THINK_C;
/* Retail permits 40 actors and reserves 0xA0 bytes before the next global. */
extern struct Humanoid *HumanGroup[40];
/* Retail's type=-1 sentinel is entry 77; the demo table had 63 entries. */
extern HumanDataType HumanData[78];
extern HumanAnimType CVAhuman[5];
extern SVECTOR UnitVector;
extern VECTOR UnitVector2;
extern short RefrectMove[16][2];
extern short RefrectVector[16];
extern TCameraStatus CamState;
extern GsRVIEW2 ViewInfo;
extern ModelType World;
extern WorldType WorldMap[8][8][8];
extern TEnemyLayout enemy[30];
extern TStageConfig StageConfig[];
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
extern unsigned char PutMapMode;
extern int CurrentEnemyID; /* enemy[] index latched by leFindEnemy */
extern GsSPRITE CursorImage;
extern GsSPRITE NumberImage;
extern GsSPRITE MapImage;
extern TLifeBarStyle LifeBarStyle[nLifeBarStyle];
extern void PutItemIcon(int ItemID, short x, short y, short scale);
extern void PutItemCursor(short x, short y, short size, int rotdif);
extern GsOT *OTablePt;
extern GsOT OTable[2];
extern GsFOGPARAM Fog;
extern short DrawingPage;
/* Retail's ilup1.pad=-1 sentinel is entry 30; the demo table had 28. */
extern WeaponType WeaponDB[31];
/* Retail's wid=-1 sentinel is entry 47; the demo table had 41 entries. */
extern WeaponModelType WeaponModel[48];
extern MotionRegistType MOTcommon[41];
extern MotionPackType *MotionPack;
extern MotionPackType *CommonMotion;
extern MotionPackType *PlayerMotion;
extern MotionPackType *StageMotion;
/* Retail initializes slots 0..25; the next global confirms 26-pointer storage. */
extern Sprite3D *ItemImage[26];
extern SoundEffect *StageSE;
extern short VoiceMode;
extern CVAType *CVAdata;
extern CVAType *CVAnow;
extern struct Humanoid *CameraTarget;
extern short CameraPanMode;
extern POLY_FT4 TelopP;
extern POLY_F4 TelopbgP;
/* CONFLICT.C's retail-expanded pool and query outputs. */
extern ConflictObjectType ConflictObject[80];
extern s16 ConflictObjects;
extern SVECTOR ConflictDistance;
extern ModelType *ConflictModel;
extern s16 Attrib;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern u_long *D_800976E8;
extern AreaNodeType *FieldArea;
extern NodeIndexType *FieldIndex;
extern char FONT_FILE_NAME;
extern char IMAGES_PREFIX_STR;
extern unsigned long *MemoryLoadAddress;
extern unsigned char *ImagePath;
extern int AccessPower;
extern int ReadMode;
extern int TotalIO;
extern POLY_GT4 AccessImage;

extern void SetCameraMode(TCameraMode mode);
extern void UpdateCoordinate(ModelType *dim);
extern short NowReturnNormal(struct Humanoid *human);
extern short GetMotionID(MotionManager *mmp, short mid);
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
extern void GetScreenPosition(long x, long y, long z, SVECTOR *scr);
