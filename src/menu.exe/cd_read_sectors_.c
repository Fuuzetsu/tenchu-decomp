/* Shared engine implementation; symbols are linked for MENU.EXE. */
#include "../main.exe/cd_read_sectors_.c"

/* MENU string storage. Keep the retail diagnostic text. */
/* Retained CD diagnostic; no surviving code reference is known. */
char msg_waitcd_read_error[] = "waitcd: read error!";
