/* Shared engine implementation; symbols are linked for MENU.EXE. */
#include "../main.exe/AfsOpenVolume.c"

/* MENU string storage. Keep the retail diagnostic text. */
char msg_afsvolume_open_err[] = "AfsOpenVolume: %s open err\n";
char msg_afsopenvolume_header_error[] = "AfsOpenVolume: Header error";
char msg_afsopenvolume_entry_error[] = "AfsOpenVolume: Entry error";
