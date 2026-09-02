#include "common.h"
#include "main.exe.h"

/*
 * PsyQ host-link stub. The break 0x107 trap returns status in v0 and result
 * in v1, so the default build keeps the two assembly pieces. The guarded C
 * body is reference-only and cannot express that ABI faithfully.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PClseek", PClseek);
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PClseek", LSEEK_OBJ_1C);
#else
int PClseek(int fd, int offset, int mode)
{
    register int r_a3 __asm__("$7") = mode;
    register int r_a2 __asm__("$6") = offset;
    register int r_a1 __asm__("$5") = fd;
    register int r_v0 __asm__("$2");
    register int r_v1 __asm__("$3");

    __asm__ volatile("break 0x107\n\t"
                     "beqz %0, 0f\n\t"
                     "addu %0, %1, $zero\n\t"
                     "addiu %0, $zero, -1\n\t"
                     "0:"
                     : "=r"(r_v0), "=r"(r_v1)
                     : "r"(r_a1), "r"(r_a2), "r"(r_a3));

    return r_v0;
}
#endif
