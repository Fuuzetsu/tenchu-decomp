#include "common.h"
#include "main.exe.h"

extern void AdtMessageBox(char *fmt, ...);
extern void LoadExecEx(u8 *file, u32 stack, u32 size);

extern char path_tenchu_menu_exe_1[];   /* cdrom:\\TENCHU\\MENU.EXE;1 */
extern char path_tenchu_main_exe_1[];   /* cdrom:\\TENCHU\\MAIN.EXE;1 */
extern char path_tenchu_ending_exe_1[]; /* cdrom:\\TENCHU\\ENDING.EXE;1 */
extern char path_tenchu_trial_exe_1[];  /* cdrom:\\TENCHU\\TRIAL.EXE;1 */
extern char fmt_bad_process_id[];       /* bad process id %x */

void exec_process_(int id)
{
    switch (id)
    {
    case PROCESS_MENU:
        LoadExecEx((u8 *)path_tenchu_menu_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case PROCESS_MAIN:
        LoadExecEx((u8 *)path_tenchu_main_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case PROCESS_ENDING:
        LoadExecEx((u8 *)path_tenchu_ending_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case PROCESS_TRIAL:
        LoadExecEx((u8 *)path_tenchu_trial_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    default:
        AdtMessageBox(fmt_bad_process_id, id);
        exec_process_(PROCESS_MENU);
        return;
    }
}
