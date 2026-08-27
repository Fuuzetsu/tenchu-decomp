#include "common.h"
#include "main.exe.h"
#include "adt.h"

/* Toggle the Adt debug console's quiet mode, returning the previous mode. */
extern AdtFntState AdtFnt;

AdtQuietMode AdtQuiet(AdtQuietMode quiet)
{
    AdtQuietMode old = AdtFnt.quiet;
    AdtFnt.quiet = quiet;
    return old;
}
