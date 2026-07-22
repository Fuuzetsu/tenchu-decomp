#include <psxsdk/libgs.h>
#include "game_types.h"
#include "ram_layout.h"

extern void PadProc(void);
extern TPadPort PadPort[][4];

extern int turn_towards_player_(int x_diff, int z_diff);
extern struct Humanoid *Me_THINK_C;
extern struct Humanoid *HumanGroup[32];
extern struct Humanoid *StagePlayer;
extern EventSeqType *StageEvent;
extern s16 SR;
extern s16 Attrib;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern u_long *GlobalAreaMap;
extern u_long *D_800976E8;
extern char FONT_FILE_NAME;
extern char IMAGES_PREFIX_STR;

extern u_long *FileRead(u8 *filename);
extern u_long *PathFileRead(u8 *resource_prefix, u8 *resource_name);
extern void *GetTIMInfo(u_long *tim, GsIMAGE *im);
extern void LoadTIMAndFree(u_long *tim);
extern void load_font_image_into_global(GsIMAGE *image);
