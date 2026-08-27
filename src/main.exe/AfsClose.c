#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * AfsClose (0x8005efd0) — TAFS file-handle closer: given a
 * `TAFSFileHandle *` (see filesystem.h for the PSX.SYM-proven layout),
 * clears flagUse to mark the slot free; a NULL handle reports via
 * AdtMessageBox() and returns -1 instead of crashing. Ghidra mistypes the
 * parameter `TAFSElement *` (it only ever sees the offset-0 store, so it
 * can't tell TAFSFileHandle.flagUse from TAFSElement.flag apart) — offset 0
 * plus "this frees a handle" semantics point at TAFSFileHandle, matching
 * AfsOpen's counterpart search-by-flagUse convention.
 *
 * Near-clone of cd_close (same guard-clause polarity: the null-check is
 * written FIRST in source even though its body ends up adjacent to the
 * epilogue in the assembly — see cd_getsize.c's header for why).
 */

extern void AdtMessageBox(char *fmt, ...);
extern char msg_afsclose_invalid_handle[]; /* AfsClose: invalid handle */ /* "AfsClose: invalid handle" — lives in this
                            * TU's unsplit data blob (splat auto-symbol),
                            * same pattern as AfsInit's msg_afsinit_not_enough_memory. */

int AfsClose(TAFSFileHandle *fd)
{
    if (fd == 0) {
        AdtMessageBox(msg_afsclose_invalid_handle);
        return -1;
    }
    fd->flagUse = 0;
    return 0;
}
