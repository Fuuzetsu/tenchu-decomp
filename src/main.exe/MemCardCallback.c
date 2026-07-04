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
 * an instruction or register difference). Root cause, verified by direct
 * experiment (13+ variants, single-instruction-set diffs via objdump -dr):
 *   - Accessing a non-small extern at RELATIVE OFFSET 0 (bare scalar, struct
 *     field at offset 0, array index 0 — tried all three, at sizes 4/8/36/40
 *     bytes) always FOLDS: cc1 emits `lui base,%hi(sym)` once (shared across
 *     both the read and the write) and lets each load/store's own `%lo(sym)`
 *     displacement supply the address — no separate address-materializing
 *     instruction, 4 total instructions (16 bytes).
 *   - Accessing the SAME kind of extern at a NONZERO constant offset (field
 *     or array index, tried offsets 8 and 0x20) always MATERIALIZES: cc1
 *     emits `lui base,%hi(sym)` + `addiu base,base,%lo(sym)` (the DECLARED
 *     symbol's address, verbatim — never partially folded), then applies the
 *     extra constant purely as the load/store's displacement — 5 instructions
 *     (20 bytes), matching this function's real shape.
 *   The target's actual bytes need the materialized address to equal the
 *   final address EXACTLY (0x800CD7D8) with a 0 residual displacement — the
 *   one combination excluded by the rule above (offset 0 never materializes;
 *   a materializing nonzero offset never leaves 0 displacement). Reachable
 *   only by finding the field whose declared offset from a REAL earlier
 *   symbol is exactly right for cc1 to fold entirely into the addiu instead
 *   of the displacement, which contradicts the observed mechanism — so this
 *   likely needs the true enclosing struct (probably a card control block
 *   starting at/before D_800CD7B8, shared with MemCardOpen/MemCardCreateFile/
 *   MemCardDeleteFile/MemCardGetDirentry's D_800cd794..D_800cd7ec fields) to
 *   be matched first, which may change how this field is reached.
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
