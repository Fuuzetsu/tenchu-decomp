#include "common.h"
#include "imagegrid.h"
#include "../main.exe/vmemory.h"

void DrawMenuImageGrid(MenuImageGrid *grid, u_short priority)
{
    POLY_FT4 *polygons;
    POLY_FT4 *polygon;
    u_short columns;
    u_short width;
    u_short height;
    short x;
    short y;

    polygons = grid->polygons;
    columns = grid->columns;
    height = grid->visible_rows;
    width = grid->visible_columns;
    for (y = 0; y < height; y++)
    {
        polygon = &polygons[y * columns];
        for (x = 0; x < width; x++)
        {
            GsSortPoly(polygon++, OTablePt, priority);
        }
    }
}

void DisposeMenuImageGrid(MenuImageGrid *grid)
{
    if (grid)
    {
        if (grid->polygons)
            vfree(grid->polygons);
        vfree(grid);
    }
}
