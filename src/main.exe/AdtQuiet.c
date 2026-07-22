#include "common.h"
#include "main.exe.h"
#include "adt.h"

extern AdtFntState D_8008F1B8;

AdtQuietMode AdtQuiet(AdtQuietMode quiet)
{
    AdtQuietMode old = D_8008F1B8.quiet;
    D_8008F1B8.quiet = quiet;
    return old;
}
