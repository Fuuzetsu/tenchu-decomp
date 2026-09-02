#include "common.h"
#include "main.exe.h"

typedef void (*MemCardCallbackFn)(u_long, u_long);

extern MemCardCallbackFn McardCallbacks[];

MemCardCallbackFn MemCardCallback(MemCardCallbackFn func)
{
    MemCardCallbackFn *slot;
    MemCardCallbackFn old;

    slot = McardCallbacks;
    old = *slot;
    *slot = func;
    return old;
}
