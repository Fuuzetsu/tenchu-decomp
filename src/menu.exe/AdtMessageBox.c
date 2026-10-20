/* Shared engine implementation; symbols are linked for MENU.EXE. */
#include "../main.exe/AdtMessageBox.c"

/* MENU string storage. Keep the retail diagnostic text. */
char msg_adtinit_not_called[] = "*** AdtInit not called ***";
char fmt_messagebox_count[] = "AdtMessageBox #%d\n\n";
char msg_press_start[] = "\n\nPress start to continue...";
