#include "common.h"
#include "main.exe.h"

/*
 * STATUS: NON_MATCHING — 8 of 88 bytes differ (2 adjacent instructions,
 * "lui t0,0x8010" / "addiu a0,a0,6", swapped in order; same instructions,
 * same registers otherwise, 0 net length change).
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
 * Matching notes (both required to reach the 8-byte residual from a
 * 25-vs-22-insn / 65-byte-diff starting point):
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
 *  - Residual: the base-pointer `lui` and the `arg0 = arg0 + 6` `addiu` are
 *    mutually independent (no data dependency), and the target schedules
 *    the `lui` FIRST while every C statement-order permutation tried here
 *    (both textual orders, split declaration/init, reordering the
 *    unrelated `i = 0` around them) scheduled the `addiu` first instead.
 *    `tools/autorules.py` found no local-rule win (int/s16/u32 width
 *    sweep). `tools/regalloc.py` shows no calls and no copy-chains in this
 *    function at all — confirming it's a pure sched1 tie between two
 *    independent single-cycle ops, not a register-allocation question.
 *    `tools/rtldump.py --pass sched` names the exact mechanism: at the
 *    ready-list step where both candidates (the `rec` address load, insn
 *    15, and the `arg0+=6` add, insn 23) are simultaneously ready, sched.c's
 *    backward list scheduler flags the STORE that consumes insn 15
 *    (`rec->magic = ...`, insn 20) as having "a greater potential hazard"
 *    and launches it (and transitively its feeder, insn 15) first — this
 *    urgency comes from insn 20's STORE through `rec` being scheduled
 *    relative to insn 30's LOAD through `arg0` (the `if (*arg0 != 0)`
 *    check): cc1's alias.c cannot prove the literal-address pointer `rec`
 *    and the parameter pointer `arg0` don't alias, so the two unrelated
 *    memory accesses are kept conservatively ordered, and that ordering
 *    pressure is what makes insn 15 win the tie over insn 23 in the
 *    TARGET but not in our draft (where the tie breaks the other way with
 *    no hazard note attached to either candidate). This is genuinely a
 *    scheduler-internal hazard heuristic operating on two DIFFERENT
 *    pointers whose non-aliasing is real but unprovable in C89 (this cc1
 *    predates `restrict`; confirmed by direct probe — `__restrict__` is a
 *    parse error in this cc1-281 build) — there is no source-level way to
 *    assert it. A bounded ~6-minute `tools/permute.py --stop-on-zero` run
 *    found nothing (timed out, flat, base score never beaten). Matches the
 *    cookbook's "sub-C-level residual early-stop" class (adjacent-
 *    instruction reorder, same instructions/registers), now with the
 *    specific sched.c mechanism identified rather than a bare
 *    "permuter-immune" label; parked here rather than burning further
 *    budget on a hazard heuristic with no C-level lever.
 */

typedef struct
{
    u32 magic;   /* 0x00 = 0xDEF0C0DE */
    char name[0x50]; /* 0x04 */
    u32 param2;  /* 0x54 */
    u32 param3;  /* 0x58 */
} BootExecRecord;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005e8f0", FUN_8005e8f0);
#else
void FUN_8005e8f0(char *arg0, u32 arg1, u32 arg2)
{
    int i;
    BootExecRecord *rec;

    rec = (BootExecRecord *)0x80100000;
    rec->magic = 0xDEF0C0DE;
    arg0 = arg0 + 6;
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
#endif
