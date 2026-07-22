#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "ram_layout.h"

extern void PadProc(void);
extern TPadPort PadPort[2][4];

extern int turn_towards_player_(int x_diff, int z_diff);
extern struct Humanoid *Me_THINK_C;
extern struct Humanoid *HumanGroup[32];
extern HumanDataType HumanData[63];
extern HumanAnimType CVAhuman[5];
extern SVECTOR UnitVector;
extern VECTOR UnitVector2;
extern short RefrectMove[16][2];
extern TCameraStatus CamState;
extern GsRVIEW2 ViewInfo;
extern ModelType World;
extern short MotionUpdateMode;
extern short SelectedItem;
extern TCdaStatus CdaStatus;
extern GsSPRITE NumberImage;
extern GsOT *OTablePt;
extern GsOT OTable[2];
extern GsFOGPARAM Fog;
extern short DrawingPage;
extern WeaponType WeaponDB[28];
extern WeaponModelType WeaponModel[41];
extern MotionRegistType MOTcommon[41];
/* Retail adds one debug-menu icon slot to PSX.SYM's 25-entry table. */
extern Sprite3D *ItemImage[26];
extern SoundEffect *StageSE;
extern short VoiceMode;
/* CONFLICT.C's retail-expanded pool and query outputs. */
extern ConflictObjectType ConflictObject[80];
extern s16 ConflictObjects;
extern SVECTOR ConflictDistance;
extern ModelType *ConflictModel;
extern s16 SR;
extern s16 Attrib;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern u_long *GlobalAreaMap;
extern u_long *D_800976E8;
extern char FONT_FILE_NAME;
extern char IMAGES_PREFIX_STR;

extern void SetCameraMode(TCameraMode mode);
extern short NowReturnNormal(struct Humanoid *human);

extern u_long *FileRead(u8 *filename);
extern u_long *PathFileRead(u8 *resource_prefix, u8 *resource_name);
extern void *GetTIMInfo(u_long *tim, GsIMAGE *im);
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
extern void SetExplosion(VECTOR *pos, SVECTOR *vect);
extern void SetHinoko(VECTOR *pos, SVECTOR *power, int n);
extern int SetFlyWire(VECTOR *start, VECTOR *end);
extern void SetWire(VECTOR *start, VECTOR *end, VECTOR *center, long len);
extern void RotateVector(VECTOR *vec, int rx, int ry, int rz);
extern void RotateVectorS(SVECTOR *vec, int rx, int ry, int rz);
extern int GetVectorDistance(VECTOR *v1, VECTOR *v2);
extern void GetScreenPosition(long x, long y, long z, SVECTOR *scr);
