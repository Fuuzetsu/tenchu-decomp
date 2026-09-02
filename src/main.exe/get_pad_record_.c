#include "common.h"
#include "main.exe.h"

TPadPort *get_pad_record_(s32 arg0)
{
    return &PadPort[arg0 >> PAD_PORT_INDEX_SHIFT]
                   [arg0 & PAD_SLOT_INDEX_MASK];
}
