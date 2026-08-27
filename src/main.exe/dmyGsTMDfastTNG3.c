#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastTNG3 (0x8006813c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape), except
 * this "N" (no-light) variant takes only 3 register arguments: the asm
 * saves/returns $a2, not $a3 (verified: word 4 is `move s0,a2`, matching
 * every other byte of the family's 18-instruction shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastTNG3[]; /* "TMDfastTNG3\n" */
extern s32 warn_dmyTMDfastTNG3;

void *dmyGsTMDfastTNG3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastTNG3 == 0)
    {
        printf(str_dmyTMDfastTNG3);
        warn_dmyTMDfastTNG3 = 1;
    }
    return arg2;
}
