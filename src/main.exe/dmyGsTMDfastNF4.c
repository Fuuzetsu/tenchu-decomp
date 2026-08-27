#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDfastNF4 (0x8006825c) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape), except
 * this "N" (no-light) variant takes only 3 register arguments: the asm
 * saves/returns $a2, not $a3 (verified: word 4 is `move s0,a2`, matching
 * every other byte of the family's 18-instruction shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDfastNF4[]; /* "TMDfastNF4\n" */
extern s32 warn_dmyTMDfastNF4;

void *dmyGsTMDfastNF4(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDfastNF4 == 0)
    {
        printf(str_dmyTMDfastNF4);
        warn_dmyTMDfastNF4 = 1;
    }
    return arg2;
}
