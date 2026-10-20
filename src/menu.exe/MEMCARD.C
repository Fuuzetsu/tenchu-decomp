/* Shared card-file format and I/O; linked against MENU.EXE symbols. */
#include "../main.exe/MEMCARD.C"

/* MENU string storage. Keep the retail spelling and development paths. */
char CardDevelopmentProductId[] = "BISLPS-00000";
char CardRetailProductId[] = "BISLPS-01901";
/* Save title in Shift-JIS: Ｔｅｎｃｈｕ−忍凱旋 */
char str_sjis_title[] = "\x82\x73\x82\x85\x82\x8e\x82\x83\x82\x88\x82\x95\x81\x7c\x94\x45\x8a\x4d\x90\xf9";
char msg_save_version_mismatch[] = "version mismatch [%x]";
