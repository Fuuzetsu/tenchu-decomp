#include "common.h"
#include "main.exe.h"

// INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/initialise_font", initialise_font);

void initialise_font(void)
{
    GsIMAGE sp10;
    u_long *tim;

    tim = PathFileRead((u8 *)&IMAGES_PREFIX_STR, (u8 *)&FONT_FILE_NAME);
    GetTIMInfo(tim, &sp10);
    LoadTIMAndFree(tim);
    load_font_image_into_global(&sp10);
}
