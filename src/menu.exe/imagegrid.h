#ifndef MENU_IMAGEGRID_H
#define MENU_IMAGEGRID_H

#include "menu.exe.h"

/* The mission menu splits an image into texture-page-sized quads.
 * A grid may draw fewer columns or rows than its allocated image contains. */
typedef struct MenuImageGrid
{
    POLY_FT4 *polygons;
    short x;
    short y;
    short cell_width;
    short cell_height;
    u_short columns;
    u_short rows;
    u_short visible_columns;
    u_short visible_rows;
    GsIMAGE image;
} MenuImageGrid;

void SetupMenuImageGrid(MenuImageGrid *grid, short image_index);
void DrawMenuImageGrid(MenuImageGrid *grid, u_short priority);
void DisposeMenuImageGrid(MenuImageGrid *grid);

#endif
