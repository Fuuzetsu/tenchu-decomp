#include "common.h"
#include "main.exe.h"

/* Reset an ordering table for the frame: set its window, point the tag at
 * the last bucket, and clear the reversed table (libgs API shape). */
void GsClearOt(unsigned short offset, unsigned short point, GsOT *ot)
{
    ot->offset = offset;
    ot->point = point;
    ot->tag = ot->org + (1 << ot->length) - 1;
    ClearOTagR((u_long *)ot->org, 1 << ot->length);
}
