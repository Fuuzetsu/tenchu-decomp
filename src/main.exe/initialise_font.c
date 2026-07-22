#include "common.h"
#include "main.exe.h"

// INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/initialise_font", initialise_font);

void initialise_font(void)
{
    GsIMAGE sp10;
    u_long *temp_v0;

    temp_v0 = PathFileRead((u8 *)&IMAGES_PREFIX_STR, (u8 *)&FONT_FILE_NAME);
    GetTIMInfo(temp_v0, &sp10);
    LoadTIMAndFree(temp_v0);
    load_font_image_into_global(&sp10);
}
