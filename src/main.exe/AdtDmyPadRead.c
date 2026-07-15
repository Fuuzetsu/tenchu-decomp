#include "common.h"
#include "main.exe.h"

/*
 * AdtDmyPadRead (0x8005fd34, 8 bytes) — dummy pad-read callback matching
 * AdtPadRead's signature (s32 (*)(s32), see AdtSelect.c); AdtMessageBox
 * compares AdtPadRead against this function's address to detect
 * "AdtInit not called". Always returns 0 (no buttons held); the argument
 * is unused.
 */

s32 AdtDmyPadRead(s32 port)
{
    return 0;
}
