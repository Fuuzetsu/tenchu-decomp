#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTNG4 (0x800685bc) — LIBGS "dummy" TMD-fast-draw placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape), except
 * this "N" (no-light) variant takes only 3 register arguments: the asm
 * saves/returns $a2, not $a3 (verified: word 4 is `move s0,a2`, matching
 * every other byte of the family's 18-instruction shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTNG4[]; /* "TMDfastTNG4\n" */
extern s32 warn_dmyTMDfastTNG4;

void *dmyGsTMDfastTNG4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastTNG4 == 0)
    {
        printf(str_dmyTMDfastTNG4);
        warn_dmyTMDfastTNG4 = 1;
    }
    return arg2;
}
