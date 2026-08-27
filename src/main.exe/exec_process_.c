#include "common.h"
#include "main.exe.h"

/*
 * exec_process_ (0x8004f6c0, 0xb0 bytes) - process-exe dispatcher: picks the
 * CD-ROM path for a requested "process id" (menu/main/ending/trial exe) and
 * hands it to LoadExecEx; an unrecognized id reports the error and retries
 * with the menu (0x10).
 *
 * Splat splits this into two pieces at a Ghidra __override__prt_ marker
 * (the default/bad-id tail) - not a jump table, both case-ladder paths
 * that fall out of range `j` straight into it (see docs/matching-cookbook.md
 * "not every 2-piece report is a real jump table").
 *
 * $a1 (a copy of id taken once at entry) stays live and unreloaded
 * through every test AND into the shared default tail - AdtMessageBox's
 * second argument (id) is passed in the SAME register the whole
 * function long, never recomputed at the call site (the m2c
 * leading-live-argument undercount rule: m2c/Ghidra only show the format
 * string as AdtMessageBox's argument, but the raw .s proves $a1==id
 * is also passed).
 */
extern void AdtMessageBox(char *fmt, ...);
extern void LoadExecEx(u8 *file, u32 addr, s32 arg2);

extern char path_tenchu_menu_exe_1[];   /* cdrom:\\TENCHU\\MENU.EXE;1 */
extern char path_tenchu_main_exe_1[];   /* cdrom:\\TENCHU\\MAIN.EXE;1 */
extern char path_tenchu_ending_exe_1[]; /* cdrom:\\TENCHU\\ENDING.EXE;1 */
extern char path_tenchu_trial_exe_1[];  /* cdrom:\\TENCHU\\TRIAL.EXE;1 */
extern char fmt_bad_process_id[];       /* bad process id %x */

void exec_process_(int id)
{
    switch (id)
    {
    case 0x10:
        LoadExecEx((u8 *)path_tenchu_menu_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case 0x11:
        LoadExecEx((u8 *)path_tenchu_main_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case 0x12:
        LoadExecEx((u8 *)path_tenchu_ending_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    case 0x13:
        LoadExecEx((u8 *)path_tenchu_trial_exe_1, TENCHU_INITIAL_STACK_ADDRESS, 0);
        break;
    default:
        AdtMessageBox(fmt_bad_process_id, id);
        exec_process_(0x10);
        return;
    }
}
