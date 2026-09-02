#include "common.h"
#include "main.exe.h"

void set_cda_volume_(u8 voll, u8 volr)
{
    CdaStatus.voll = voll;
    CdaStatus.volr = volr;
}
