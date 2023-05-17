#include "common.h"
#include "main.exe.h"

// INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/initialise_font", initialise_font);

void initialise_font(void)
{
    GsIMAGE sp10;
    u32 *temp_v0;

    temp_v0 = load_resource_from(&IMAGES_PREFIX_STR, &FONT_FILE_NAME);
    tenchu_GsGetTimInfo(temp_v0, &sp10);
    load_image_from_tim_to_gpu_and_dealloc_data_(temp_v0);
    load_font_image_into_global(&sp10);
}
