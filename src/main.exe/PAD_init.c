#include "common.h"
#include "main.exe.h"
#include <psxsdk/libpad.h>

extern s32 PadInitFlag;

void PAD_init(char *buf0, long len0, char *buf1, long len1)
{
    _remove_ChgclrPAD();
    EnterCriticalSection();
    _patch_pad();
    ExitCriticalSection();
    ChangeClearPAD(0);
    kernel_start_pad_();
    PAD_init2(buf0, len0, buf1, len1);
    PadInitFlag = 1;
    /* Keeps the flag store in the function body under the original scheduler. */
    do
    {
    } while (0);
}
