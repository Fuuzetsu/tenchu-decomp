#include "common.h"
#include "main.exe.h"

/*
 * MemCardCallback (0x80081fec) — install a new memory-card driver callback and
 * return the previous one (the standard PSY-Q `old = f(new); ...; f(old);`
 * save/restore pattern used around every card operation: MemCardOpen,
 * MemCardGetDirentry, MemCardCreateFile, MemCardDeleteFile all call it in
 * pairs). `D_800CD7B8` is a ~0x20-byte path buffer (used by MemCardOpen via
 * strcat/open) that sits immediately before the callback slot, hence the pad.
 *
 * STATUS: NON_MATCHING — 3 of 20 bytes differ (the base symbol's own
 * relocation vs. its field-offset split; a pure addressing-encoding tie, not
 * an instruction or register difference). Root cause, verified by extensive
 * direct experiment (20+ variants total, single-instruction-set diffs via
 * objdump -dr and tools/rtldump.py):
 *   - Accessing a non-small extern at RELATIVE OFFSET 0 — bare scalar, a
 *     properly-sized (0x20-byte) struct's offset-0 field, a 1-field struct,
 *     and a plain array index 0 (sizes 4/8/32/36/40 bytes all tried) —
 *     always FOLDS: cc1 emits `lui base,%hi(sym)` once (shared across both
 *     the read and the write) and lets each load/store's own `%lo(sym)`
 *     displacement supply the address — no separate address-materializing
 *     instruction, 4 total instructions (16 bytes). A genuinely BARE (non-
 *     struct) top-level scalar at offset 0 is the ONE exception: it left
 *     `lw $2,SYM`/`sw $4,SYM` as bare unsplit assembler pseudo-ops that
 *     maspsx/AS then expand PER OCCURRENCE (not CSE-shared), producing 24
 *     bytes — worse, not closer.
 *   - Accessing the SAME kind of extern at a NONZERO constant offset (field
 *     or array index, tried offsets 8, 0x20, 0x40) always MATERIALIZES: cc1
 *     emits `lui base,%hi(sym)` + `addiu base,base,%lo(sym)` (the DECLARED
 *     symbol's address, verbatim — never partially folded), then applies the
 *     extra constant purely as the load/store's displacement — 5 instructions
 *     (20 bytes, the right LENGTH), matching this function's real shape but
 *     with the WRONG base (materializes D_800CD7B8's own address, -10312,
 *     not the field's, -10280).
 *   - Taking the field's address explicitly into a local pointer
 *     (`s32 *p = &D_800CD7B8.func;` or the equivalent raw `(u8*)base + 0x20`
 *     pointer-arithmetic cast, dereferenced immediately after) does NOT
 *     force materialization either — cc1 folds `%hi/%lo(D_800CD7B8+32)`
 *     directly as EACH memory op's own displacement (still the 4-
 *     instruction/16-byte shape, just with the offset baked into the
 *     relocation instead of the symbol's bare `%lo`). A `volatile` field
 *     qualifier suppresses the CSE sharing entirely instead (2 independent
 *     lui+op pairs, 24 bytes) — moves further away, not closer.
 *   - A 50000-iteration bounded permuter run against the correct-length
 *     (20-byte) draft stayed flat at the base score the whole time (never
 *     found an improving candidate) — this is genuinely below the AST level
 *     the permuter operates on, matching the `la`/address-materialization
 *     reload-tie signature, but unlike PrepareAccess/FileRead's version of
 *     that signature (a block-crossing pseudo blocking combine_regs, fixed
 *     by restructuring which BLOCK the reload lives in), no combination of
 *     restructuring here changes which VALUE gets materialized — every
 *     tried C shape puts either the enclosing symbol's own address (nonzero
 *     offset, direct access) or the fully-folded field address (any offset,
 *     via a captured pointer) into the addiu/displacement, never the target's
 *     "field's own final address as a materialized, 0-residual base".
 *   The one combination this needs — MATERIALIZE (not fold) an address that
 *   equals a NONZERO-offset field's OWN final value — was not reached by any
 *   spelling tried. It may need the true, wider enclosing card-control-block
 *   struct (confirmed real from other MemCard* functions' disassembly: a
 *   base at 0x800CD798, with MemCardOpen using offsets +0x10 (0x800CD7A8)
 *   and +0x20 (0x800CD7B8 = our pad) from it, and MemCardCreateFile/
 *   MemCardDeleteFile/MemCardGetDirentry referencing 0x800CD7A0..0x800CD7E4)
 *   to be matched first — matching THOSE may pin down whether `func` is
 *   really reached through some OTHER field's address already live in a
 *   register at this exact call site (a cross-function calling-convention
 *   detail this isolated 5-instruction leaf function cannot exhibit on its
 *   own), which is beyond what this function's own source can control.
 */
typedef struct
{
    u8 pad[0x20];
    s32 func;
} MemCB;

extern MemCB D_800CD7B8;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/MemCardCallback", MemCardCallback);
#else
s32 MemCardCallback(s32 func)
{
    s32 old = D_800CD7B8.func;
    D_800CD7B8.func = func;
    return old;
}
#endif
