#include "common.h"
#include "main.exe.h"

extern AdtFntState D_8008F1B8;

s32 AdtQuiet(s32 quiet)
{
    s32 old = D_8008F1B8.quiet;
    D_8008F1B8.quiet = quiet;
    return old;
}
