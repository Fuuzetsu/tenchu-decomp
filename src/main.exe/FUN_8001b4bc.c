#include "common.h"
#include "main.exe.h"

/*
 * FUN_8001b4bc (0x8001b4bc) — zeroes the low byte of PadPort[0][0]'s
 * TPadPort.Send (offset 0xC of the record returned by
 * FUN_8001b4e0(0); a plain sb, not the sign-extension shift-split GetPad/
 * FUN_8001b174 need, since the index here is the literal 0, not a variable).
 */
extern TPadPort *FUN_8001b4e0(s32 arg0);

void FUN_8001b4bc(void)
{
    FUN_8001b4e0(0)->Send = 0;
}
