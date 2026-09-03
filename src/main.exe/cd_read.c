#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern void cd_read_sectors_(u8 *buffer, s32 sector, s32 byteOffset, s32 length);
extern char msg_cd_read_invalid_handle[]; /* "cd_read:invalid handle" */

int cd_read(FILE *f, void *buffer, int length)
{
    s32 pos;
    s32 sector;

    if (f == 0)
    {
        puts(msg_cd_read_invalid_handle);
        return -1;
    }
    pos = f->pos;
    if (f->finfo.size < pos + length)
    {
        length = f->finfo.size - pos;
    }
    if (length > 0)
    {
        sector = CdPosToInt(&f->finfo.pos);
        cd_read_sectors_(buffer, sector + pos / 2048, pos % 2048, length);
        return length;
    }
    return 0;
}
