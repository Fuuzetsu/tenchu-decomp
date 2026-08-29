#include "common.h"
#include "main.exe.h"
#include "adt.h"

extern AdtFntState AdtFnt;

/* Set the Adt debug console's quiet mode, returning the previous one. */

AdtQuietMode AdtQuiet(AdtQuietMode quiet)
{
    AdtQuietMode old = AdtFnt.quiet;
    AdtFnt.quiet = quiet;
    return old;
}
