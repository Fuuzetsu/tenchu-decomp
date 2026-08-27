#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/*
 * cd_control (0x8005f738) — thin retry wrapper over the BIOS CdControlB:
 * spins calling CdControlB(mode, param, result) while it returns 0 (command
 * rejected because the drive is busy), yielding a frame via VSync(0) between
 * attempts.
 */

extern int VSync(int mode);

void cd_control(u8 com, u8 *param, u8 *result)
{
    while (1) {
        if (CdControlB(com, param, result) != 0)
            break;
        VSync(0);
    }
}
