#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern int puts(char *s);
extern char msg_cd_getsize_invalid_handle[]; /* "cd_getsize:invalid handle" */

int cd_getsize(FILE *f)
{
    if (f == 0)
    {
        puts(msg_cd_getsize_invalid_handle);
        return -1;
    }
    return f->finfo.size;
}
