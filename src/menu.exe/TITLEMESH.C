#include "common.h"
#include "title.h"
#include "../main.exe/images.h"
#include "../main.exe/tim.h"
#include "../main.exe/tuning.h"

/* A 16 by 16 quad mesh ripples the title background while its outer
 * vertices remain fixed at the edges of the screen. */

void InitTitleMesh(void)
{
    GsIMAGE image;
    u_long *data;
    POLY_FT4 *polygon;
    short x;
    short y = 0;
    u_short width;
    u_short height;
    short mode;
    short tx;
    short ty;

    data = get_tim_from_archive(TitleImageArchive, TITLE_IMAGE_BACKGROUND);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    mode = TIM_PIXEL_MODE((u_short)image.pmode);
    width = (image.pw / TITLE_MESH_CELLS) << (2 - mode);
    height = image.ph / TITLE_MESH_CELLS;
    memset(TitleMeshPhase, 0, sizeof(TitleMeshPhase));
    UpdateTitleMesh();
    for (; y < TITLE_MESH_CELLS; y++)
    {
        for (x = 0; x < TITLE_MESH_CELLS; x++)
        {
            polygon = &TitleMeshPolygons[y * TITLE_MESH_CELLS + x];
            SetPolyFT4(polygon);
            setRGB0(polygon, 128, 128, 128);
            setXY4(polygon,
                   TitleMeshX[x][y], TitleMeshY[x][y],
                   TitleMeshX[x + 1][y], TitleMeshY[x + 1][y],
                   TitleMeshX[x][y + 1], TitleMeshY[x][y + 1],
                   TitleMeshX[x + 1][y + 1], TitleMeshY[x + 1][y + 1]);
            tx = image.px + image.pw * x / TITLE_MESH_CELLS;
            ty = image.py + image.ph * y / TITLE_MESH_CELLS;
            polygon->clut = GetClut(image.cx, image.cy);
            polygon->tpage = GetTPage(mode, 0, tx, ty);
            tx = (tx << (2 - mode)) & ((1 << (8 - mode)) - 1);
            setUV4(polygon, tx, ty, tx + width, ty,
                   tx, ty + height, tx + width, ty + height);
        }
    }
}

void UpdateTitleMesh(void)
{
    short x;
    short y;
    int distance;

    for (y = 0; y < TITLE_MESH_VERTICES; y++)
    {
        for (x = 0; x < TITLE_MESH_VERTICES; x++)
        {
            TitleMeshX[x][y] = x * (SCREEN_W / TITLE_MESH_CELLS) - SCREEN_W / 2;
            TitleMeshY[x][y] = y * (SCREEN_H / TITLE_MESH_CELLS) - SCREEN_H / 2;
            if (x != 0 && x != TITLE_MESH_CELLS &&
                y != 0 && y != TITLE_MESH_CELLS)
            {
                distance = SquareRoot0(TitleMeshX[x][y] * TitleMeshX[x][y] +
                                       TitleMeshY[x][y] * TitleMeshY[x][y]);
                TitleMeshX[x][y] +=
                    (rcos(TitleMeshPhase[x][y]) * TitleMeshX[x][y] /
                     TITLE_RIPPLE_ATTENUATION) >> FIXED_SHIFT;
                TitleMeshY[x][y] +=
                    (rcos(TitleMeshPhase[x][y]) * TitleMeshY[x][y] /
                     TITLE_RIPPLE_ATTENUATION) >> FIXED_SHIFT;
                TitleMeshPhase[x][y] += TITLE_RIPPLE_PHASE_STEP - distance;
            }
        }
    }
}

void DrawTitleMesh(void)
{
    POLY_FT4 *polygon;
    short x;
    short y;

    if (GameClock % TITLE_MESH_UPDATE_FRAMES == 0)
        UpdateTitleMesh();
    for (y = 0; y < TITLE_MESH_CELLS; y++)
    {
        for (x = 0; x < TITLE_MESH_CELLS; x++)
        {
            polygon = &TitleMeshPolygons[y * TITLE_MESH_CELLS + x];
            setXY4(polygon,
                   TitleMeshX[x][y], TitleMeshY[x][y],
                   TitleMeshX[x + 1][y], TitleMeshY[x + 1][y],
                   TitleMeshX[x][y + 1], TitleMeshY[x][y + 1],
                   TitleMeshX[x + 1][y + 1], TitleMeshY[x + 1][y + 1]);
            GsSortPoly(polygon, OTablePt, TITLE_MESH_PRIORITY);
        }
    }
}
