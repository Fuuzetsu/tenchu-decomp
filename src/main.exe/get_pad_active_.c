#include "common.h"
#include "main.exe.h"

u8 get_pad_active_(short arg0)
{
    s32 port = arg0 << 4;
    TPadPort *pad = &PadPort[port >> PAD_PORT_INDEX_SHIFT]
                           [port & PAD_SLOT_INDEX_MASK];

    return pad->active;
}
