#include "common.h"
#include "main.exe.h"
#include <psxsdk/libpad.h>

/* The game's RAM-resident copy of the kernel StartPad routine (B(13h)
 * shape: critical section, SysDeq/SysEnqIntRP(1, element) with the pad
 * handlers, return 1) — the patched pad path calls it directly instead
 * of the BIOS StartPAD2 stub. Invented name. */
extern s32 PadInitFlag;

long InitPAD(char *buf0, long len0, char *buf1, long len1)
{
    _remove_ChgclrPAD();
    EnterCriticalSection();
    _patch_pad();
    ExitCriticalSection();
    ChangeClearPAD(0);
    kernel_start_pad_();
    InitPAD2(buf0, len0, buf1, len1);
    PadInitFlag = 1;
    /* Keeps the flag store in the function body under the original scheduler. */
    do
    {
    } while (0);
    return 1;
}
