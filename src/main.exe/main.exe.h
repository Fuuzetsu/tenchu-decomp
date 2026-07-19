#include <psxsdk/libgs.h>
#include "game_types.h"
#include "ram_layout.h"

extern void PadProc(void);
extern controller_input PadPort[][4];

extern int turn_towards_player_(int x_diff, int z_diff);
extern character_state *Me_THINK_C;
extern s16 SR;
extern s16 Attrib;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern char FONT_FILE_NAME;
extern char IMAGES_PREFIX_STR;

extern void *PathFileRead(char *resource_prefix, char *resource_name);
extern void *GetTIMInfo(u_long *tim, GsIMAGE *im);
extern void LoadTIMAndFree(u_long *tim);
extern void load_font_image_into_global(GsIMAGE *image);
