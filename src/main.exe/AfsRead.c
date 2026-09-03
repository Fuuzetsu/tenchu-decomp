#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "filesystem.h"

extern char msg_afsread_invalid_handle[]; /* "AfsRead: invalid handle" */

u32 AfsRead(TAFS *volume, TAFSFileHandle *fd, void *buffer, u32 length)
{
    u32 size;
    u32 pos;

    if (fd == 0)
    {
        AdtMessageBox(msg_afsread_invalid_handle);
        return 0;
    }
    cd_seek(volume->fpVol, fd->info->pos + fd->pos, CDSEEK_SET);
    size = fd->info->size;
    pos = fd->pos;
    if (size < length + pos)
    {
        length = size - pos;
        if (length == 0)
        {
            return 0;
        }
    }
    cd_read(volume->fpVol, buffer, length);
    fd->pos += length;
    return length;
}
