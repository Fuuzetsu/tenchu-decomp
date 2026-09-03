#include "common.h"
#include "main.exe.h"

extern char fmt_reading_exec[];
extern char msg_exe_read_ok[];
extern void cb_nop_(void);
extern EXEC *CdReadExec(u8 *name);
extern int CdReadSync(s32 mode, u8 *result);
void run_exec_file(u8 *name, u32 stack, u32 size)
{
    EXEC *exec;

    VSyncCallback(cb_nop_);
    do
    {
        do
        {
            printf(fmt_reading_exec, name);
            exec = CdReadExec(name);
        } while (exec == NULL);
    } while (CdReadSync(0, NULL) != 0);

    VSyncCallback(NULL);
    printf(msg_exe_read_ok);
    exec->s_addr = stack;
    exec->s_size = size;
    StopCallback();
    Exec(exec, 0, NULL);
}
