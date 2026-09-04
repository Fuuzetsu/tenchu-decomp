#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

void cd_control(u8 com, u8 *param, u8 *result)
{
    while (CdControlB(com, param, result) == 0)
    {
        VSync(0);
    }
}
