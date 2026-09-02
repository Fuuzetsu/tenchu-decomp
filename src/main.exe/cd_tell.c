#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern int puts(char *s);
extern char msg_cd_tell_invalid_handle[]; /* "cd_tell:invalid handle" */

int cd_tell(FILE *f)
{
    if (f == 0)
    {
        puts(msg_cd_tell_invalid_handle);
        return -1;
    }
    return f->pos;
}
