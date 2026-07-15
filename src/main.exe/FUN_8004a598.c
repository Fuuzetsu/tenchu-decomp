#include "common.h"
#include "main.exe.h"

/*
 * FUN_8004a598 (0x8004a598, 0x34 bytes) — 2-column byte-table lookup:
 * row = param_2, column = (param_1 == 1). No direct (jal) callers found
 * (tools/xref.py); reached indirectly (proc pointer or similar).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Ghidra rendered the row scale as `(param_2 << 0x10) >> 0xf` (raw shift
 *    translation) and defaulted param_2 to `int` since it didn't recognize
 *    the pattern; the asm's `sll 16 / sra 15` pair is cc1's combined
 *    sign-extend-and-double for a `short` row index into a 2-byte-wide row
 *    (a plain `int` index would need only a single `sll 1`). Both params
 *    are `short` in source.
 *  - The column flag needs its OWN earlier statement (`flag = (param_1 ==
 *    1);` before the return) — inlined into the array subscript directly
 *    it either mismerges into the row term or (added as a raw `+` operand)
 *    materializes via a branch instead of the target's `xori`+`sltiu`.
 *  - That flag temp must be `int`, not `short`: with `short flag` the la
 *    (lui/addiu of the table base) schedules ONE instruction too early,
 *    landing between the flag calc's `xori` and `sltiu` (12-byte pure
 *    reorder, same instructions/registers); `int flag` removes whatever
 *    re-widening note made that tie go the other way, matching the
 *    target's flag-then-base-then-row order exactly.
 */

extern u8 D_8008E3EC[][2];

u8 FUN_8004a598(short param_1, short param_2)
{
    int flag;

    flag = (param_1 == 1);
    return D_8008E3EC[param_2][flag];
}
