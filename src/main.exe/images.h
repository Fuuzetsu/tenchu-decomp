#ifndef TENCHU_IMAGES_H
#define TENCHU_IMAGES_H

/* IMAGES.C's original CD location lead-in. */
enum
{
    OFFSET = 150
};

extern void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply,
                                short x, short y);
extern void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply,
                                short x, short y);

#endif
