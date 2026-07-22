#include "common.h"
#include "main.exe.h"
#include "adt.h"

/*
 * AdtDmyPadRead (0x8005fd34, 8 bytes) — dummy pad-read callback matching
 * AdtPadRead's signature (`long (*)(int)`, declared in adt.h); AdtMessageBox
 * compares AdtPadRead against this function's address to detect
 * "AdtInit not called". Always returns 0 (no buttons held); the argument
 * is unused.
 */

long AdtDmyPadRead(int port)
{
    return 0;
}
