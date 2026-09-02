#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern int puts(char *s);
extern char msg_cd_seek_invalid_handle[]; /* "cd_seek:invalid handle" */

int cd_seek(FILE *f, int offset, TSeekMode whence)
{
    s32 pos;
    u32 size;

    if (f == 0)
    {
        puts(msg_cd_seek_invalid_handle);
        return -1;
    }
    switch (whence)
    {
    case CDSEEK_SET:
        pos = offset;
        break;
    case CDSEEK_END:
        pos = f->finfo.size + offset;
        break;
    case CDSEEK_CUR:
        pos = f->pos + offset;
        break;
    }
    size = f->finfo.size;
    if (size < pos)
    {
        pos = size;
    }
    else if (pos < 0)
    {
        pos = 0;
    }
    f->pos = pos;
    return 0;
}
