#include "common.h"
#include "main.exe.h"

extern int AdtVsprintf(s32 *args, char *dst, u32 n, char *fmt);
extern char *AdtMsgPtr;
extern char AdtMsgEnd[];

void debug_printf_(char *arg0, ...)
{
    s32 *ap = (s32 *)((char *)&arg0 + sizeof(arg0));
    AdtMsgPtr += AdtVsprintf(ap, AdtMsgPtr, AdtMsgEnd - AdtMsgPtr, arg0);
}
