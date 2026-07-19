#include "common.h"
#include "main.exe.h"

/*
 * GS_107_OBJ_4B8 (0x80065a9c) — stock PsyQ libgs: cache the light-colour matrix
 * in `_LC` (already a known symbol at 0x800c64c0) and load it into the GTE.
 * The cache write is a whole-MATRIX struct assignment, which cc1 expands to the
 * word-by-word copy; `SetColorMatrix` still receives the caller's pointer, not
 * the cache (a0 is untouched across the copy).
 *
 * The name is Ghidra's placeholder (object 107, offset 0x4B8). PsyQ calls this
 * one of the GsSetLight* entries — not renamed here: per the contract a rename
 * needs callmatch --verify plus a separate commit.
 */
extern MATRIX _LC;
extern void SetColorMatrix(MATRIX *m);

/*
 * STATUS: NON_MATCHING — ONE INSTRUCTION short of exact (26 insns vs 25). Every
 * other instruction, register and operand matches.
 *
 * **`-mno-split-addresses` is REQUIRED here and it is the whole reason this is
 * close** (Build.hs original-object profile; precedent: LIBMCRD.OBJ). Without
 * it cc1 splits the address of `_LC` across two registers and everything
 * downstream shifts:
 *
 *   TARGET / WITH the flag        WITHOUT the flag (the default)
 *     lui   a2,0x800c               lui   v0,0x800c
 *     addiu a2,a2,25792             addiu a3,v0,25792      <- 2 regs, not 1
 *     scratch v0,v1,a1              scratch v1,a1,a2       <- all shifted up
 *
 * That is the cookbook's named `la` address-materialisation signature — `%hi` in
 * a temp vs the target register. It is an original-object compiler-input
 * question, NOT a source-shape one: no C spelling fixed it (a
 * `MATRIX *lc = &_LC;` local is a
 * measured no-op — cse folds it). The flag is justified by provenance: this is a
 * stock PsyQ library leaf, and the SDK need not share the game's
 * address-splitting default.
 *
 * THE REMAINING INSTRUCTION: the target puts the copy's LAST store in the call's
 * delay slot; we emit it before the `jal` and leave the slot empty, which also
 * costs a load-delay `nop` later in the epilogue:
 *
 *   TARGET                        OURS
 *     sw v0,24(a2)                  sw v0,24(a2)
 *     jal SetColorMatrix            sw v1,28(a2)
 *     sw v1,28(a2)   <- slot        jal SetColorMatrix
 *     lw ra,16(sp)                  nop            <- empty slot
 *     addiu sp,sp,24                lw ra,16(sp)
 *     jr ra                         nop            <- load delay
 *     nop                           jr ra
 *
 * Compiler dumps isolate the remaining question. Our GCC 2.8.1 keeps this copy
 * as one `movstrsi_internal` whose destination is
 * `mem:BLK(symbol_ref "_LC")`.
 * The MIPS backend's delay-slot `define_split` accepts only register-addressed
 * block moves, so reorg cannot expose the final store. The stock GS_107.OBJ is
 * relocation-normalised byte-for-byte identical to the target's complete
 * 0x570-byte object, including this filled slot. It is unchanged between PsyQ
 * 4.5 and 4.6 even though those kits bundle different cc1 versions, so archive
 * adjacency does not identify the compiler that built it. Direct compiler
 * probes also split the evidence: Sony 2.8.1 preserves the target's 3+3+2 copy
 * batching but leaves the slot empty, while Sony 2.95.2 fills the slot but emits
 * sequential load/store pairs. The ordinary struct assignment remains the
 * strongest human-source reconstruction; explicit word copies fill the slot
 * only by rotating every scratch register and perturbing the epilogue.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GS_107_OBJ_4B8", GS_107_OBJ_4B8);
#else
void GS_107_OBJ_4B8(MATRIX *m)
{
    _LC = *m;
    SetColorMatrix(m);
}
#endif
