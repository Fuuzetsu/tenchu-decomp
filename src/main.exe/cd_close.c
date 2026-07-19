#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/*
 * cd_close (0x8005f558) — thin CD-file-handle closer: given a `FILE *`
 * (the raw CD-image file object shared with cd_getsize/cd_tell/cd_read/
 * cd_seek — see FILE/CdlFILE in AfsInit.c for the proven layout), clears
 * flagUse to mark the slot free; a NULL handle reports via puts() and
 * returns -1 instead of crashing.
 */

typedef struct FILE FILE;

struct FILE
{
    CdlFILE finfo;   /* 0x0 */
    s32 flagUse;     /* 0x18 */
    s32 pos;         /* 0x1C */
};

extern int puts(char *s);
extern char D_80014A30[]; /* "close:invalid handle" — lives in this TU's
                            * unsplit data blob (splat auto-symbol), same
                            * pattern as AfsInit's D_80014944. */

int cd_close(FILE *f)
{
    if (f == 0) {
        puts(D_80014A30);
        return -1;
    }
    f->flagUse = 0;
    return 0;
}
