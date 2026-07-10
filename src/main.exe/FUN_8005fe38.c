#include "common.h"
#include "main.exe.h"

/*
 * FUN_8005fe38 (0x8005fe38, 0x50 bytes) — appends a printf-style formatted
 * message to the shared debug-message ring buffer that FUN_8005fe88 seeds
 * with the "%#" mode prefix and whose write cursor (D_80097E98) it walks
 * forward: the same buffer the "Adt" on-screen debug console
 * (AdtMessageBox/AdtVsprintf/AdtSelect, all in this address range) draws
 * from. Takes a printf-style format string plus up to 4 vararg words
 * (spilled to the stack by a MIPS varargs prologue — taking the address of
 * one incoming register parameter forces cc1 to spill all four), forwards
 * them through AdtVsprintf at the buffer's current write cursor
 * D_80097E98, and advances the cursor by however many bytes were written.
 *
 * SPLIT-BOUNDARY FIX: this function used to be carved as FUN_8005fe34
 * (0x8005fe34, 0x54 bytes), i.e. starting 4 bytes earlier. Those first 4
 * bytes (`addiu sp,sp,0x78`, encoded 0x27BD0078) are NOT this function's —
 * they are AdtVsprintf's own epilogue delay-slot fill (its `jr ra` sits at
 * 0x8005FE30 with the .s carve for AdtVsprintf, size 0xF8, ending with a
 * bare `jr ra` and no delay-slot instruction of its own). Per this repo's
 * cookbook ("Toolchain gotchas": trivial ra-only frames), our cc1's
 * mips_expand_epilogue ALWAYS reorgs the sp-restore into `jr ra`'s own
 * delay slot for every function it compiles — there is no source shape
 * that leaves a `jr ra` with an unfilled/foreign delay slot. AdtVsprintf's
 * true carve is therefore 0xFC bytes (ending in `addiu sp,sp,0x78`), one
 * word longer than Ghidra's functions.tsv entry (248, i.e. 0xF8) — the
 * "UNDER-SIZED functions.tsv entry" class in docs/matching-cookbook.md,
 * except here the missing tail landed inside a neighbouring (phantom)
 * function carve instead of an orphan .data blob, because Ghidra also
 * independently placed a spurious function label at 0x8005fe34: nothing
 * in the whole retail image calls it (`tools/xref.py` found zero direct
 * `jal` callers) or references it as a literal 32-bit pointer value (a
 * `grep` for the word 0x8005fe34 across disks/tenchu/main.exe finds zero
 * hits) — there is no real callback/table entry there. `config/splat.
 * main.exe.yaml` was corrected to carve AdtVsprintf through 0x8005fe38 and
 * this function starting there (0x50 bytes, matching exactly what remains
 * once AdtVsprintf's own delay slot is excluded).
 *
 * D_80097E98 is %gp_rel in this TU (tools/gpsyms.py --write); D_800C3EB0
 * is materialized with lui/addiu (not gp-relative) even though it is the
 * end of the very same D_800C2EB0[0x1000] buffer FUN_8005fe88 writes into —
 * Ghidra assigns it a separate symbol because it sits outside
 * D_800C2EB0's known array bounds.
 */
extern int AdtVsprintf(s32 *args, char *dst, u32 n, char *fmt);
extern char *D_80097E98;
extern char D_800C3EB0[];

void FUN_8005fe38(char *arg0, ...)
{
    s32 *ap = (s32 *)((char *)&arg0 + sizeof(arg0));
    D_80097E98 += AdtVsprintf(ap, D_80097E98, D_800C3EB0 - D_80097E98, arg0);
}
