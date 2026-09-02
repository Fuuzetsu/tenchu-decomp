#include "common.h"
#include "main.exe.h"

void save_pad_analog_(void)
{
    TLinkInfo *ps =
        (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;

    if (PadPort[0][0].fAnalog != 0)
    {
        ps->analog_pad_present |= 1;
    }
    else
    {
        ps->analog_pad_present &= ~1;
    }
}
