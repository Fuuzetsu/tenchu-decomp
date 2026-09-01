#include "common.h"
#include "main.exe.h"
#include "tim.h"

/* Parse a TIM in memory into a GsIMAGE descriptor: mode word, then the
 * optional CLUT block, then the pixel block's VRAM rect (libgs API shape). */
void GsGetTimInfo(unsigned long *image, GsIMAGE *tim)
{
    unsigned long *pixel;

    tim->pmode = *image;
    if (TIM_HAS_CLUT(tim->pmode))
    {
        image = TIM_IMAGE_CURSOR_ADVANCE(image, mode, blocks);
        pixel = TIM_BLOCK_NEXT(image);
        image = TIM_BLOCK_CURSOR_ADVANCE(image, byte_size, position);
        tim->cx = TIM_BLOCK_POSITION(image)->x;
        tim->cy = TIM_BLOCK_POSITION(image)->y;
        image = TIM_BLOCK_CURSOR_ADVANCE(image, position, size);
        tim->cw = TIM_BLOCK_SIZE(image)->width;
        tim->ch = TIM_BLOCK_SIZE(image)->height;
        image = TIM_BLOCK_CURSOR_ADVANCE(image, size, data);
        tim->clut = image;

        pixel = TIM_BLOCK_CURSOR_ADVANCE(pixel, byte_size, position);
        tim->px = TIM_BLOCK_POSITION(pixel)->x;
        tim->py = TIM_BLOCK_POSITION(pixel)->y;
        pixel = TIM_BLOCK_CURSOR_ADVANCE(pixel, position, size);
        tim->pw = TIM_BLOCK_SIZE(pixel)->width;
        tim->ph = TIM_BLOCK_SIZE(pixel)->height;
        pixel = TIM_BLOCK_CURSOR_ADVANCE(pixel, size, data);
        tim->pixel = pixel;
    }
    else
    {
        image = TIM_IMAGE_CURSOR_ADVANCE(image, mode, blocks[0].position);
        tim->px = TIM_BLOCK_POSITION(image)->x;
        tim->py = TIM_BLOCK_POSITION(image)->y;
        image = TIM_BLOCK_CURSOR_ADVANCE(image, position, size);
        tim->pw = TIM_BLOCK_SIZE(image)->width;
        tim->ph = TIM_BLOCK_SIZE(image)->height;
        image = TIM_BLOCK_CURSOR_ADVANCE(image, size, data);
        tim->pixel = image;
    }
}
