#include "common.h"
#include "main.exe.h"

extern TPadPort *get_pad_record_(s32 arg0);

void clear_pad_send_(void)
{
    get_pad_record_(PAD_PORT_1)->Send = 0;
}
