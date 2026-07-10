#include "common.h"
#include "main.exe.h"

/*
 * PathFileRead (0x80019394, 0x38 bytes) — joins a resource prefix and a
 * resource name into a 256-byte stack buffer and loads that file. Every caller
 * passes a directory prefix like D_800127A4 ("K:\WORK\CDIMAGE\IMAGE\") plus a
 * bare filename, so the "%s%s" is a plain path concatenation.
 */

extern char D_800976DC[]; /* "%s%s" */

extern int sprintf(char *buf, char *fmt, ...);
extern void *FileRead(char *path);

void *PathFileRead(char *resource_prefix, char *resource_name)
{
    char path[256];

    sprintf(path, D_800976DC, resource_prefix, resource_name);
    return FileRead(path);
}
