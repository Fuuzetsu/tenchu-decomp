#include "common.h"
#include "text_entry.h"
#include <psxsdk/libgpu.h>

/* Fill columns from right to left, with five glyphs down each keyboard column.
 * A single-column upload is also used for labels and leaves the key map alone. */
void LoadTrialGlyphGrid(u16 first_code, RECT *area, short columns, short rows)
{
    RECT rect = *area;
    int x, y;

    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    rect.w = 16;
    rect.h = 15;
    rect.x += (columns - 1) * 18;
    for (x = 0; x < columns; x++)
    {
        for (y = 0; y < rows; y++)
        {
            if (UploadTrialGlyph(first_code, &rect))
            {
                if (columns != 1)
                    TrialKeyboardCursor->characters[x + y * columns] = first_code;
                first_code++;
                rect.y += 19;
            }
        }
        rect.x = (short)(rect.x - 2) - rect.w;
        rect.y = area->y;
    }
}
