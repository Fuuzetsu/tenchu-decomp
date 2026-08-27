#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

/*
 * cd_tell (0x8005f798) — thin CD-file-handle position accessor: given a
 * `FILE *` (the raw CD-image file object shared with cd_close/cd_getsize/
 * cd_read/cd_seek — see TFileHandle/CdlFILE in filesystem.h for the proven layout),
 * returns the current stream position cached in FILE.pos; a NULL handle
 * reports via puts() and returns -1 instead of crashing. Exact clone of
 * cd_getsize but for the `pos` field instead of `finfo.size` — see its
 * header for the guard-clause polarity note (verified again here).
 */

extern int puts(char *s);
extern char msg_cd_tell_invalid_handle[]; /* cd_tell:invalid handle */ /* "cd_tell:invalid handle" — lives in this TU's
                                                                        * unsplit data blob (splat auto-symbol), same
                                                                        * pattern as AfsInit's msg_afsinit_not_enough_memory. */

int cd_tell(FILE *f)
{
    if (f == 0)
    {
        puts(msg_cd_tell_invalid_handle);
        return -1;
    }
    return f->pos;
}
