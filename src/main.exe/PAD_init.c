#include "common.h"
#include "main.exe.h"

extern void _remove_ChgclrPAD(void);
extern void EnterCriticalSection(void);
extern void _patch_pad(void);
extern void ExitCriticalSection(void);
extern void ChangeClearPAD(long mode);
extern void kernel_start_pad_(void);
extern void PAD_init2(u_long a, u_long b, u_long c, u_long d);

extern s32 PadInitFlag;

void PAD_init(u_long a, u_long b, u_long c, u_long d)
{
    int new_var;

    _remove_ChgclrPAD();
    EnterCriticalSection();
    _patch_pad();
    ExitCriticalSection();
    ChangeClearPAD(0);
    kernel_start_pad_();
    PAD_init2(a, b, c, d);
    new_var = 1;
    PadInitFlag = new_var;
    /* Empty loop retained for code layout; its original source construct is unknown. */
    do
    {
    } while (0);
}
