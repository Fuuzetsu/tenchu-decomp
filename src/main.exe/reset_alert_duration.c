#include "common.h"
#include "main.exe.h"

/*
 * STATUS: NON_MATCHING — reset_alert_duration was never CARVED (no `c` subsegment), so its
 * .c was never linked and never actually compared: its bytes came from the
 * raw data blob and matchdiff reported a false MATCH. Carved now, and the
 * draft below does not reproduce the original. matchdiff/progress refuse to
 * count an un-carved function from now on.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/reset_alert_duration", reset_alert_duration);
#else
/*
 * reset_alert_duration (0x8002f7f4, 0x24 bytes) — (re)starts the
 * alert-state countdown: 300 frames normally, 600 when D_80010058 == 2.
 * Part of the original "think" TU (same as Think1sleep.c): it gp-addresses
 * FRAMES_UNTIL_END_OF_ALERT, already declared extern in main.exe.h.
 *
 * D_80010058 is read via absolute (non-gp) addressing — Ghidra names it
 * PersistentState._88_1_ (decimal offset 88 = 0x58 from the 0x80010000
 * persistent-state blob, inside game_types.h's opaque field_0x49[0x15]
 * span); kept as a fresh extern byte here rather than editing that shared
 * struct for one still-unidentified field.
 */

extern u8 D_80010058;

void reset_alert_duration(void)
{
    s32 duration;

    duration = 300;
    if (D_80010058 == 2)
    {
        duration = 600;
    }
    FRAMES_UNTIL_END_OF_ALERT = duration;
}
#endif /* NON_MATCHING */
