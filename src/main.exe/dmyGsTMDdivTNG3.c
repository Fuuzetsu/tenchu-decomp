#include "common.h"
#include "main.exe.h"

/*
 * dmyGsTMDdivTNG3 (0x800673bc) — LIBGS "dummy" primitive-sort placeholder; warn-once
 * clone of dmyGsPrstF3NL (see that file for the family's shape), except
 * this "N" (no-light) variant takes only 3 register arguments: the asm
 * saves/returns $a2, not $a3 (verified: word 4 is `move s0,a2`, matching
 * every other byte of the family's 18-instruction shape).
 */
extern int printf(const char *fmt, ...);
extern char str_dmyTMDdivTNG3[]; /* "TMDdivTNG3\n" */
extern s32 warn_dmyTMDdivTNG3;

void *dmyGsTMDdivTNG3(void *arg0, void *arg1, void *arg2)
{
    if (warn_dmyTMDdivTNG3 == 0)
    {
        printf(str_dmyTMDdivTNG3);
        warn_dmyTMDdivTNG3 = 1;
    }
    return arg2;
}
