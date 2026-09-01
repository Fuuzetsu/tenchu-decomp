/* search_id_table_ is byte-identical in main.exe and trial.exe -- it uses no
 * relocations, so the same source links unchanged in both. Share the C
 * rather than copying it; cpp resolves this relative to THIS file, and
 * -MMD records it so an edit to the original rebuilds trial.exe too. */
#include "../main.exe/search_id_table_.c"
