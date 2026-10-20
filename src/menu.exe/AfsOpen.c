/* Shared engine implementation; symbols are linked for MENU.EXE. */
#include "../main.exe/AfsOpen.c"

/* MENU string storage. Keep the retail diagnostic text. */
char msg_afsopen_not_found[] = "AfsOpen: %s not found\n";
char msg_afsopen_no_handle[] = "AfsOpen: no more handle\n[%s]";
