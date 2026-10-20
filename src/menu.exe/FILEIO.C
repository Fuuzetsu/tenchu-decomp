/* Shared engine implementation; symbols are linked for MENU.EXE. */
#include "../main.exe/FILEIO.C"

/* MENU string storage. Keep the retail spelling and development paths. */
u8 str_acqurememorydisk[] = "ACQUREMEMORYDISK";
char path_tenchu_data[] = "TENCHU\\DATA";
char fmt_load_pc[] = "%$LOAD(PC)\n%d[%s]";
char msg_load_pc_error[] = "LOAD(PC) ERROR\n%d[%s]";
char msg_memory_load_is_disabled[] = "*** memory load is disabled now ***";
char fmt_load_cd[] = "%$LOAD(CD)\n%d[%s]";
char msg_load_cd_error[] = "LOAD(CD) ERROR\n%d[%s]";
char path_demo_loading_tim[] = "K:\\WORK\\CDIMAGE\\DEMO\\loading.tim";
char path_demo_load_ten_tim[] = "K:\\WORK\\CDIMAGE\\DEMO\\load_ten.tim";
char path_tenchu_run_exe_1[] = "\\TENCHU\\RUN.EXE;1";
