#include "common.h"
#include "main.exe.h"

/*
 * dmyGsPrstF3NL (0x80066b04) — LIBGS "dummy" primitive-sort placeholder: the
 * first of 108 byte-identical warn-once stubs at 0x80066b04..0x80068964 (72
 * bytes each), installed as the default per-primitive-type handler until the
 * real GsPrstXXX is registered. Each stub has its own private flag (a `s32`
 * at 0x8008f618 + 4*index, index in address order) and name string (in
 * 0x80015104..0x800156b0, "<name-without-dmyGs>\n"); on first call it prints
 * the name once via printf, then always returns the 4th argument unchanged
 * (the passed-through packet pointer). Never called directly in this image
 * (indirect via a primitive-type function-pointer table); the argument types
 * are unknown/unused, so generic pointers stand in for the real ABI.
 *
 * Exception: the 15 "N" (no-light) variants — TMDdiv/TMDfast [T]N{F,G}{3,4} —
 * save/return $a2 (3 register args), not $a3, e.g. dmyGsTMDdivNF3 (see that
 * file). Verified per-function from the raw asm, not assumed.
 */
extern int printf(const char *fmt, ...);
extern char str_dmyPrstF3NL[]; /* "PrstF3NL\n" */
extern s32 warn_dmyPrstF3NL;

void *dmyGsPrstF3NL(void *arg0, void *arg1, void *arg2, void *arg3)
{
    if (warn_dmyPrstF3NL == 0)
    {
        printf(str_dmyPrstF3NL);
        warn_dmyPrstF3NL = 1;
    }
    return arg3;
}
