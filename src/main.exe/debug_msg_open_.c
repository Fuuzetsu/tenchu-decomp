#include "common.h"
#include "main.exe.h"
#include "adt.h"

extern char AdtMsgBuf[];

void debug_msg_open_(void)
{
    AdtMsgBuf[0] = 0x25;
    AdtMsgBuf[1] = 0x23;
    AdtMessageBox(AdtMsgBuf);
    AdtMsgPtr = &AdtMsgBuf[2];
}
