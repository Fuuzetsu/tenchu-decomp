#include "common.h"
#include "main.exe.h"

/*
 * STATUS: NON_MATCHING — 15 of 112 bytes differ (whole-image: 15, function
 * is the right length, 28 insns both sides) — a pure register-color
 * residual (uniform +1 shift: a1<-a2, a2<-a3, a3<-t0). autorules: no
 * genuinely-correct improving edit (its own scoring disagreed with
 * tools/matchdiff.py's raw byte diff twice on this function — see notes).
 * Three bounded tools/permute.py runs (score 510->145->85, each seeded
 * from the prior best, ~30k iterations total) found real improvements
 * that were hand-applied (see below) but none reached 0.
 *
 * FUN_8001b2f4 (0x8001b2f4) — pad button REMAPPER: called by
 * ThinkBasicHuman1 as `pad = FUN_8001b2f4(GetPad(0));` (see
 * ThinkBasicHuman1.c's matching notes — established prototype
 * `extern s32 FUN_8001b2f4(s16 pad);`, wide s32 return). Not yet named or
 * placed in reference/psxsym-candidates.tsv / psxsym-unplaced.tsv (no
 * PSX.SYM block — checked both by address and by name).
 *
 * D_80011210 is a 32-byte table: 4 "control-scheme" rows of 8 bytes, each
 * row a permutation of the 8 single-bit pad masks (0x01/0x02/.../0x80).
 * D_800976F6 (a %gp_rel short, control-scheme index 0-3) selects the row
 * (`row = D_800976F6 << 3`). For each of the 8 canonical button positions
 * i, the function tests the CALLER's raw pad bits against row 0's mask
 * `D_80011210[i]` and, if set, OR's the SELECTED scheme's mask
 * `D_80011210[row+i]` into the result (remapping physical button i to
 * logical position row+i); if clear, it AND-clears that same logical bit.
 * This is a classic PS1 "control type A/B/C/D" pad remapper. Untried
 * candidate names from the unplaced/candidates lists (SetPadState,
 * SetPad) don't fit this address or behavior.
 *
 * Matching notes:
 *  - `pad` (the parameter, s16) is a byte/bit TEST source only; `acc` (the
 *    OR/AND accumulator, u16, seeded from `pad` so the untouched high
 *    bits/unlisted buttons pass through unchanged) is the separate
 *    register that survives to the return. Ghidra's `ushort uVar2` is the
 *    accumulator's real type — the final `(int)(short)` cast at the
 *    return re-establishes sign, matching the caller's s32-return
 *    prototype.
 *  - The row-clearing branch's mask is read raw as `u8` (`D_80011210[row]`)
 *    then bitwise-NOT'd in the FULL (int-promoted, zero-extended) width —
 *    this is what compiles to the `nor $zero,v0` / `and` pair, not an
 *    `andi` (0x80011210's bytes never need masking down further).
 *  - `i` (0..7, the row-0 test index) and `row` (the selected row's index,
 *    incrementing in lockstep) are two SEPARATE loop-carried values
 *    (register $a2 / $v1) — both start together (`i=0`, `row=scheme<<3`)
 *    and both `+= 1` every iteration, but they are NOT the same variable
 *    (only `row`'s start is offset by the scheme selection).
 *  - Loop is a genuine `do { ... } while (i < 8);` — `i` starts at 0 and
 *    the bound is a compile-time constant 8, so jump.c's
 *    duplicate_loop_exit_test folds away any entry guard regardless of
 *    for/while/do-while spelling; do-while matches Ghidra's own shape.
 *  - The declaration-order/dead-initial-`rp` and the `test` split (holding
 *    the bit-test result in its OWN pseudo instead of inline in the `if`)
 *    both came from decomp-permuter, verified afterward with
 *    `tools/matchdiff.py` (28->15 bytes) — a THIRD permuter round found a
 *    45-scoring "improvement" (making the loop bound test `test < 8`
 *    instead of `i < 8`) that is semantically WRONG (`test` holds
 *    `pad & D_80011210[i]`, not the counter) and was rejected despite the
 *    better score — permuter/autorules scores are a proxy, not proof;
 *    only `tools/matchdiff.py`'s raw byte diff and a manual read of what
 *    changed are.
 */

extern u8 D_80011210[32];
extern s16 D_800976F6;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8001b2f4", FUN_8001b2f4);
#else

s32 FUN_8001b2f4(s16 pad)
{
    s32 i;
    s32 row;
    u16 acc;
    s32 test;
    u8 *rp;

    rp = &D_80011210[row];
    acc = pad;
    row = (s32)D_800976F6 << 3;
    i = 0;
    do
    {
        test = pad & D_80011210[i];
        rp = &D_80011210[row];
        if (test != 0)
        {
            acc = acc | *rp;
        }
        else
        {
            acc = acc & ~*rp;
        }
        i = i + 1;
        row = row + 1;
    } while (i < 8);
    return (s32)(s16)acc;
}
#endif
