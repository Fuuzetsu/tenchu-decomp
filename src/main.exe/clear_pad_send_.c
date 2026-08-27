#include "common.h"
#include "main.exe.h"

/*
 * clear_pad_send_ (0x8001b4bc) — zeroes the low byte of PadPort[0][0]'s
 * TPadPort.Send (offset 0xC of the record returned by
 * get_pad_record_(0); a plain sb, not the sign-extension shift-split GetPad/
 * get_pad_active_ need, since the index here is the literal 0, not a variable).
 */
extern TPadPort *get_pad_record_(s32 arg0);

void clear_pad_send_(void)
{
    get_pad_record_(0)->Send = 0;
}
