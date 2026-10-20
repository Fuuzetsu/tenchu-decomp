#ifndef PSXSDK_KERNEL_H
#define PSXSDK_KERNEL_H

#include <psxsdk/libapi.h>

struct XF_HDR
{
    char key[8];
    u_long text;
    u_long data;
    EXEC exec;
    char title[60];
};

#endif
