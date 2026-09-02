#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

extern int VSync(int mode);

void cd_control(u8 com, u8 *param, u8 *result)
{
    while (1)
    {
        if (CdControlB(com, param, result) != 0)
            break;
        VSync(0);
    }
}
