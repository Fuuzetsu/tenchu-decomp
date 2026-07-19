#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/*
 * AfsRead (0x8005f048, 0xC4 bytes) — reads `length` bytes from a TAFS file
 * handle into `buffer`, clamped to the remaining size (fd->info->size -
 * fd->pos); seeks the underlying CD file to the handle's absolute position
 * first (fd->info->pos is the directory entry's start sector, fd->pos is
 * the handle's own read cursor). A NULL handle reports via AdtMessageBox()
 * and returns 0. Same proven TAFS/TAFSElement/TAFSFileHandle/FILE layout as
 * AfsClose.c/AfsFileSize.c/AfsInit.c/cd_read.c/cd_seek.c.
 *
 * Matching notes: Ghidra renders the clamp as one side-effecting `&&`
 * expression (`if ((size < length+pos) && (length = size-pos, length ==
 * 0)) { length = 0; } else { cd_read(...); pos += length; }`). Un-nest it
 * into plain sequential statements + an early return instead (`if (size <
 * length+pos) { length = size-pos; if (length == 0) return 0; }
 * cd_read(...); pos += length; return length;`) — matched first try; no
 * need to preserve Ghidra's comma-expression shape.
 */

typedef struct FILE FILE;
typedef struct TAFS TAFS;
typedef struct TAFSElement TAFSElement;
typedef struct TAFSFileHandle TAFSFileHandle;

struct FILE
{
    CdlFILE finfo;   /* 0x0 */
    s32 flagUse;     /* 0x18 */
    s32 pos;         /* 0x1C */
};

struct TAFS
{
    FILE *fpVol;             /* 0x0 */
    s32 fModified;           /* 0x4 */
    u32 posElement;          /* 0x8 */
    TAFSElement *pElement;   /* 0xC */
    u32 maxElements;         /* 0x10 */
    s32 maxElementArea;      /* 0x14 */
    TAFSFileHandle *pHandle; /* 0x18 */
};

struct TAFSElement
{
    u16 flag;    /* 0x0 */
    u32 pos;     /* 0x4 */
    u32 size;    /* 0x8 */
    u32 psize;   /* 0xC */
    u8 name[20]; /* 0x10 */
};

struct TAFSFileHandle
{
    s32 flagUse;       /* 0x0 */
    u32 pos;           /* 0x4 */
    TAFSElement *info; /* 0x8 */
};

extern void AdtMessageBox(char *fmt, ...);
extern int cd_seek(FILE *f, int offset, int whence);
extern int cd_read(FILE *f, void *buffer, int length);
extern char D_800149D0[]; /* "AfsRead: invalid handle" — lives in this TU's
                            * unsplit data blob (splat auto-symbol), same
                            * pattern as AfsInit's D_80014944. */

u32 AfsRead(TAFS *volume, TAFSFileHandle *fd, void *buffer, u32 length)
{
    u32 size;
    u32 pos;

    if (fd == 0) {
        AdtMessageBox(D_800149D0);
        return 0;
    }
    cd_seek(volume->fpVol, fd->info->pos + fd->pos, 0);
    size = fd->info->size;
    pos = fd->pos;
    if (size < length + pos) {
        length = size - pos;
        if (length == 0) {
            return 0;
        }
    }
    cd_read(volume->fpVol, buffer, length);
    fd->pos = fd->pos + length;
    return length;
}
