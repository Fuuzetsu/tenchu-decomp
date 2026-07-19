#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/*
 * cd_tell (0x8005f798) — thin CD-file-handle position accessor: given a
 * `FILE *` (the raw CD-image file object shared with cd_close/cd_getsize/
 * cd_read/cd_seek — see FILE/CdlFILE in AfsInit.c for the proven layout),
 * returns the current stream position cached in FILE.pos; a NULL handle
 * reports via puts() and returns -1 instead of crashing. Exact clone of
 * cd_getsize but for the `pos` field instead of `finfo.size` — see its
 * header for the guard-clause polarity note (verified again here).
 */

typedef struct FILE FILE;

struct FILE
{
    CdlFILE finfo;   /* 0x0 */
    s32 flagUse;     /* 0x18 */
    s32 pos;         /* 0x1C */
};

extern int puts(char *s);
extern char D_80014A60[]; /* "cd_tell:invalid handle" — lives in this TU's
                            * unsplit data blob (splat auto-symbol), same
                            * pattern as AfsInit's D_80014944. */

int cd_tell(FILE *f)
{
    if (f == 0) {
        puts(D_80014A60);
        return -1;
    }
    return f->pos;
}
