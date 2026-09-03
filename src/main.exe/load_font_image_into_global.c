#include "common.h"
#include "main.exe.h"
#include "font.h"

extern GsIMAGE FONT_IMAGE_;

void load_font_image_into_global(GsIMAGE *image)
{
    FONT_IMAGE_ = *image;
}
