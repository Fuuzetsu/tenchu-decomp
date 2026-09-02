#include "common.h"
#include "main.exe.h"
#include <psxsdk/libsnd.h>

extern void SsSetSerialAttr(u8 a, u8 b, u8 c);
extern void SsSetSerialVol(u8 a, u8 voll, u8 volr);

void apply_cda_attr_(s32 arg0)
{
    if (arg0 != 0)
    {
        SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
        SsSetSerialVol(SS_SERIAL_A, CdaStatus.voll, CdaStatus.volr);
    }
    else
    {
        SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
        SsSetSerialVol(SS_SERIAL_A, 0, 0);
    }
}
