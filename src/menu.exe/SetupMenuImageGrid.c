#include "common.h"
#include "imagegrid.h"
#include "../main.exe/images.h"
#include "../main.exe/tim.h"
#include "../main.exe/vmemory.h"

extern u_long *MissionImageArchive;

void SetupMenuImageGrid(MenuImageGrid *grid, short image_index)
{
    short mode;
    u_short columns;
    u_short rows;
    u_long *data;
    GsIMAGE *image;
    POLY_FT4 *polygon = (POLY_FT4 *)TENCHU_SCRATCHPAD(0);
    short x;
    short y;
    short sx;
    short sy;
    short width;
    short height;
    short tx;
    short ty;
    short u;

    columns = grid->columns;
    rows = grid->rows;
    image = &grid->image;
    grid->polygons = valloc((columns * sizeof(POLY_FT4)) * rows);
    data = get_tim_from_archive(MissionImageArchive, image_index);
    GetTIMInfo(data, image);
    LoadTIM(data);
    mode = TIM_PIXEL_MODE(image->pmode);
    sy = grid->y;
    width = grid->cell_width;
    height = grid->cell_height;
    for (y = 0; y < rows; y++)
    {
        sx = grid->x;
        for (x = 0; x < columns; x++)
        {
            setPolyFT4(polygon);
            setRGB0(polygon, 128, 128, 128);
            setXYWH(polygon, sx, sy, width, height);
            tx = image->px + ((width * x) >> (2 - mode));
            ty = image->py + height * y;
            setClut(polygon, image->cx, image->cy);
            setTPage(polygon, mode, 0, tx, ty);
            u = tx << (2 - mode);
            u &= (1 << (8 - mode)) - 1;
            setUV4(polygon, u, ty, u + width - 1, ty,
                   u, ty + height - 1, u + width - 1, ty + height - 1);
            grid->polygons[y * columns + x] = *polygon;
            sx += width;
        }
        sy += height;
    }
}
