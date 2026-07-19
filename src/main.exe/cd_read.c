#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/*
 * cd_read (0x8005f590) — reads up to `length` bytes from a CD file handle
 * (the proven `FILE *` shared with cd_close/cd_getsize/cd_tell/cd_init/
 * cd_open) at its current `pos`, clamped to the remaining `finfo.size`;
 * a NULL handle reports via puts() and returns -1 (see cd_getsize's header
 * for the guard-clause polarity note). CdPosToInt(&f->finfo.pos) converts
 * the handle's start-of-file BCD CdlLOC into an absolute sector number
 * (&f->finfo.pos is offset 0 of the struct, i.e. bit-identical to `f`
 * itself — the m2c reference under-counts this as a zero-arg call because
 * $a0 is the live incoming `f` parameter, never reassigned before the jal;
 * see the cookbook's "leading argument carried in live" rule); adding
 * `pos` (rounded down to a whole sector via the 0x7FF bias before the
 * arithmetic shift) locates the absolute sector/byte-offset FUN_8005f380
 * forwards to the raw sector reader.
 */

typedef struct FILE FILE;

struct FILE
{
    CdlFILE finfo;   /* 0x0 */
    s32 flagUse;     /* 0x18 */
    s32 pos;         /* 0x1C */
};

extern int puts(char *s);
extern void FUN_8005f380(void *buffer, int sector, int byteOffset, int byteLength);
extern char D_80014A94[]; /* "cd_read:invalid handle" — lives in this TU's
                            * unsplit data blob (splat auto-symbol), same
                            * pattern as AfsInit's D_80014944. */

int cd_read(FILE *f, void *buffer, int length)
{
    s32 pos;
    s32 sector;
    s32 adj;

    if (f == 0) {
        puts(D_80014A94);
        return -1;
    }
    pos = f->pos;
    if (f->finfo.size < pos + length) {
        length = f->finfo.size - pos;
    }
    if (length > 0) {
        sector = CdPosToInt(&f->finfo.pos);
        adj = pos;
        if (pos < 0) {
            adj = pos + 0x7FF;
        }
        FUN_8005f380(buffer, sector + (adj >> 11), pos - ((adj >> 11) << 11), length);
        return length;
    }
    return 0;
}
