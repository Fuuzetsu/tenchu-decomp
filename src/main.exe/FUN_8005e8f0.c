#include "common.h"
#include "main.exe.h"

/*
 * STATUS: MATCHING — pure C, all 88 bytes / 22 instructions exact, with the
 * target's 2 conditional branches, no calls or jumps, and 1 return.
 *
 * FUN_8005e8f0 (0x8005e8f0, 0x58 bytes) — stages a boot-exec record at the
 * fixed address 0x80100000 (Ghidra sees this as `MemoryPool[0x24000]`, but
 * a raw LITERAL pointer cast `(BootExecRecord *)0x80100000` is what
 * actually reproduces the shape — same distinction vinit.c's header
 * documents for its own literal `0x800DC000` store: Ghidra's "MemoryPool"
 * label is cosmetic, not proof of a real extern-symbol reference). Copies a
 * magic value, then copies a NUL-terminated name starting 6 bytes into
 * `arg0` (likely a CD directory entry, past its BCD position field) into
 * the record's name buffer, then stores two caller params (likely
 * load/exec addresses) past the name. Called by LoadExecEx.
 *
 * Matching notes:
 *  - The base address MUST be a literal integer cast held in a named local
 *    pointer (`BootExecRecord *rec = (BootExecRecord *)0x80100000;`), not a
 *    declared `extern BootExecRecord D_80100000;` (which emits a full `la`
 *    per access — the cookbook's "extern-array symbol emits la" rule) and
 *    not an inline cast at each use site (three separate `lui`/`addiu`
 *    materializations instead of one shared `lui` reused via displacement
 *    for every field, cookbook's "named local pointer" table-chain rule).
 *    Because 0x80100000's low 16 bits are exactly 0, a bare `lui` alone is
 *    a fully valid pointer value, so cc1 can fold every field's constant
 *    offset (0/4/0x54/0x58) directly into each access's own displacement
 *    and reuse ONE register for the whole function — this only works for a
 *    compile-time-KNOWN literal base, not a relocatable symbol (whose low
 *    bits are unknown until link).
 *  - The mutating string cursor must REUSE the `arg0` parameter register
 *    directly (`arg0 = arg0 + 6; ... *arg0; arg0++;`), not a fresh named
 *    local — a fresh local swapped the cursor/index register ROLES
 *    (cursor ended up in the register the original gives to the index, and
 *    vice versa), a 16-byte residual on its own.
 *  - The final adjacent schedule closes by splitting the magic literal into
 *    a named producer, placing a byte-neutral empty one-shot loop immediately
 *    after that assignment, then spelling pointer setup, cursor advance, and
 *    the magic store as separate statements. The LOOP_END barrier partitions
 *    the producer phase without fencing the store: sched emits exactly
 *    `lui/ori magic; lui rec; addiu arg0,6; sw magic`. Fencing `rec` put its
 *    `lui` too early; fencing the store put the `sw` before the cursor add.
 */

typedef struct
{
    u32 magic;   /* 0x00 = 0xDEF0C0DE */
    char name[0x50]; /* 0x04 */
    u32 param2;  /* 0x54 */
    u32 param3;  /* 0x58 */
} BootExecRecord;

void FUN_8005e8f0(char *arg0, u32 arg1, u32 arg2)
{
    int i;
    u32 magic;
    BootExecRecord *rec;

    magic = 0xDEF0C0DE;
    do {
    } while (0);
    rec = (BootExecRecord *)0x80100000;
    arg0 = arg0 + 6;
    rec->magic = magic;
    i = 0;
    if (*arg0 != 0) {
        do {
            rec->name[i] = *arg0;
            arg0++;
            i++;
        } while (*arg0 != 0);
    }
    rec->name[i] = 0;
    rec->param2 = arg1;
    rec->param3 = arg2;
}
