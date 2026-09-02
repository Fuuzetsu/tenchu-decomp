#include "common.h"
#include "main.exe.h"
#include "filesystem.h"

extern int puts(char *s);
extern char msg_close_invalid_handle[]; /* "close:invalid handle" */

int cd_close(FILE *f)
{
    if (f == 0)
    {
        puts(msg_close_invalid_handle);
        return -1;
    }
    f->flagUse = 0;
    return 0;
}
