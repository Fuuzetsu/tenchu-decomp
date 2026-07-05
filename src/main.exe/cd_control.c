#include "common.h"
#include "main.exe.h"

/*
 * cd_control (0x8005f738) — thin retry wrapper over the BIOS CdControlB:
 * spins calling CdControlB(mode, param, result) while it returns 0 (command
 * rejected because the drive is busy), yielding a frame via VSync(0) between
 * attempts.
 */

extern int CdControlB(u8 com, u8 *param, u8 *result);
extern int VSync(int mode);

void cd_control(u8 param_1, u8 *param_2, u8 *param_3)
{
    while (1) {
        if (CdControlB(param_1, param_2, param_3) != 0)
            break;
        VSync(0);
    }
}
