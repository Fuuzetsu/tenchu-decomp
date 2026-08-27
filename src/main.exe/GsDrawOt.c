#include "common.h"
#include "main.exe.h"

/* Kick the ordering table's packet chain to the GPU (libgs API shape). */
void GsDrawOt(GsOT *ot)
{
    DrawOTag(ot->tag);
}
