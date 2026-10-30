#include "common.h"
#include "mission_editor.h"

/* Put the overview on the side opposite the cursor, with a red box for
 * the editor's current viewport. The Z cursor argument is unused. */
void DrawTrialEditorOverviewFrame(short cursor_x, short cursor_z,
                                 short center_x, short center_z)
{
    GsLINE line;
    short side = -120;
    short overview_y = -80;
    short view_x, view_y;
    short i;

    line.attribute = 0;
    if (cursor_x < center_x)
        side = 120;
    view_x = side + center_x - 15;
    view_y = center_z - 96;
    for (i = 0; i < 4; i++)
    {
        line.r = line.g = line.b = 128;
        line.x0 = side + (((i + 1) & 2) ? 26 : -26);
        line.y0 = overview_y + (i < 2 ? -24 : 24);
        line.x1 = side + (i < 2 ? 26 : -26);
        line.y1 = overview_y + (((i + 1) & 2) ? 24 : -24);
        if (line.x0 > side) line.x0++;
        if (line.x1 > side) line.x1++;
        if (line.y0 > overview_y) line.y0--;
        if (line.y1 > overview_y) line.y1--;
        GsSortLine(&line, OTablePt, 3);

        line.r = 240;
        line.g = line.b = 0;
        line.x0 = view_x + (((i + 1) & 2) ? 13 : -13);
        line.y0 = view_y + (i < 2 ? -9 : 9);
        line.x1 = view_x + (i < 2 ? 13 : -13);
        line.y1 = view_y + (((i + 1) & 2) ? 9 : -9);
        GsSortLine(&line, OTablePt, 3);
    }
}
