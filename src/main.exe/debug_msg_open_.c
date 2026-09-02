#include "common.h"
#include "main.exe.h"

extern void AdtMessageBox(char *fmt, ...);
extern char AdtMsgBuf[];
extern char *AdtMsgPtr;

void debug_msg_open_(void)
{
    AdtMsgBuf[0] = 0x25;
    AdtMsgBuf[1] = 0x23;
    AdtMessageBox(AdtMsgBuf);
    AdtMsgPtr = &AdtMsgBuf[2];
}
