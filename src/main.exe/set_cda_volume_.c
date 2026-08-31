#include "common.h"
#include "main.exe.h"

/* CD-audio status (Ghidra: TCdaStatus). Only voll/volr are touched here. */
void set_cda_volume_(u8 voll, u8 volr)
{
    CdaStatus.voll = voll;
    CdaStatus.volr = volr;
}
