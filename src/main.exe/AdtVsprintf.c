#include "common.h"
#include "main.exe.h"


int AdtVsprintf(s32 *args, char *dst, u32 size, char *fmt)
{
    s32 copied[10];
    s32 *out;
    int i;

    if (size < (u32)(strlen(fmt) << 1))
        return 0;

    i = 0;
    out = copied;
    do
    {
        args++;
        *out++ = args[-1];
        i++;
    } while (i < 10);
    return sprintf(dst, fmt, copied[0], copied[1], copied[2], copied[3],
                   copied[4], copied[5], copied[6], copied[7], copied[8],
                   copied[9], copied[10]);
}
