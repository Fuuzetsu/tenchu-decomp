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
        image++;
        pixel = image + (*image >> TIM_BYTE_TO_WORD_SHIFT);
        image++;
        tim->cx = ((TIMBlockPosition *)image)->x;
        tim->cy = ((TIMBlockPosition *)image)->y;
        image++;
        tim->cw = ((TIMBlockSize *)image)->width;
        tim->ch = ((TIMBlockSize *)image)->height;
        image++;
        tim->clut = image;

        pixel++;
        tim->px = ((TIMBlockPosition *)pixel)->x;
        tim->py = ((TIMBlockPosition *)pixel)->y;
        pixel++;
        tim->pw = ((TIMBlockSize *)pixel)->width;
        tim->ph = ((TIMBlockSize *)pixel)->height;
        pixel++;
        tim->pixel = pixel;
    }
    else
    {
        image += 2;
        tim->px = ((TIMBlockPosition *)image)->x;
        tim->py = ((TIMBlockPosition *)image)->y;
        image++;
        tim->pw = ((TIMBlockSize *)image)->width;
        tim->ph = ((TIMBlockSize *)image)->height;
        image++;
        tim->pixel = image;
    }
}
