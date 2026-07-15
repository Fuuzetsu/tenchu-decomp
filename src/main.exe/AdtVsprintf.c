#include "common.h"
#include "main.exe.h"

/*
 * Copy ten argument words into a local array and forward them to sprintf,
 * provided the destination has room for twice the format-string length.
 *
 * Matching notes (252 bytes / 63 instructions):
 *  - The true carve includes the stack-restoring delay slot at 0x8005fe34;
 *    Ghidra's 248-byte function entry omits that final instruction.
 *  - The capacity failure is an early return. A shared result local survives
 *    both calls, adds a fifth saved register, and lengthens the function.
 *  - Incrementing args before reading args[-1] fixes the loop's otherwise
 *    byte-neutral load/increment ordering.
 *  - copied[10] intentionally reproduces the retail call's eleventh variadic
 *    word. It reads the word immediately beyond the ten-word array, which is
 *    the caller's saved $s0 at sp+0x60 in the matched frame.
 */

extern u32 strlen(const char *s);
extern int sprintf(char *dst, const char *fmt, ...);

int AdtVsprintf(s32 *args, char *dst, u32 size, char *fmt)
{
    s32 copied[10];
    s32 *out;
    int i;

    if (size < (u32)(strlen(fmt) << 1))
        return 0;

    i = 0;
    out = copied;
    do {
        args++;
        *out++ = args[-1];
        i++;
    } while (i < 10);
    return sprintf(dst, fmt, copied[0], copied[1], copied[2], copied[3],
                   copied[4], copied[5], copied[6], copied[7], copied[8],
                   copied[9], copied[10]);
}
