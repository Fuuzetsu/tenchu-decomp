#include "common.h"
#include "main.exe.h"

/* Current GPU packet cursor (libgs API shape). */
PACKET *GsGetWorkBase(void)
{
    return GsOUT_PACKET_P;
}
