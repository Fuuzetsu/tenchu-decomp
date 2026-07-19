#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * cd_getsize (0x8005f6d8) — thin CD-file-handle size accessor: given a
 * `FILE *` (the raw CD-image file object shared with cd_close/cd_tell/
 * cd_read/cd_seek — see TFileHandle/CdlFILE in filesystem.h for the proven layout),
 * returns the directory-record size cached in finfo.size; a NULL handle
 * reports via puts() and returns -1 instead of crashing.
 *
 * Matching notes: the null-check guard is written FIRST in source (matching
 * Ghidra's own rendering) even though its body ends up adjacent to the
 * epilogue (no trailing jump) in the assembly — with both arms of the if
 * ending in `return`, cc1 places the literal if-body at the branch target
 * (falls straight into the epilogue) and the trailing code as the branch's
 * fallthrough (which needs the explicit jump). Inverting the guard's
 * polarity to make the success path the literal if-body (matching the
 * fallthrough/branch-target roles naively) compiles to the wrong physical
 * layout — verified on cd_close.
 */

extern int puts(char *s);
extern char D_80014A78[]; /* "cd_getsize:invalid handle" — lives in this
                            * TU's unsplit data blob (splat auto-symbol),
                            * same pattern as AfsInit's D_80014944. */

int cd_getsize(FILE *f)
{
    if (f == 0) {
        puts(D_80014A78);
        return -1;
    }
    return f->finfo.size;
}
