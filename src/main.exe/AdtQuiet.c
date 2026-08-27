#include "common.h"
#include "main.exe.h"
#include "adt.h"

extern AdtFntState AdtFnt;

AdtQuietMode AdtQuiet(AdtQuietMode quiet)
{
    AdtQuietMode old = AdtFnt.quiet;
    AdtFnt.quiet = quiet;
    return old;
}
