#include "common.h"
#include "main.exe.h"

extern int toupper(int c);

void AfsFilenameFix(char *path)
{
    char *p;

    if (*path != 0)
    {
        p = path;
        do
        {
            *p = toupper(*p);
            p++;
        } while (*p != 0);
    }
}
