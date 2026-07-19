#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * FUN_8001b2b8 (0x8001b2b8, 0x3c bytes) — called only from LoadExecEx: mirrors
 * byte 7 of PadPort[0][0] into bit0 of the persistent-state byte at 0x80010047.
 * Ghidra calls that byte PersistentState._71_1_ (it prints the offset in
 * DECIMAL: 71 = 0x47), which lands on field_0x3b[0xC] — the byte just before
 * flags48, still unidentified.
 *
 * The `lui $v1, 0x8001` with NO `addiu` (hoisted into the branch delay slot and
 * reused as the base for both arms' lbu/sb) is the tell that the source holds
 * the blob's address in a LOCAL, via the literal pointer cast this codebase
 * already uses — `(PersistentState *)0x80010000`, see
 * BriefingAndInventorySelectionScreen.c's PSTATE / FUN_800565f0.c. Referencing
 * the byte as a plain `extern u8 D_80010047` instead lets cc1 fold the address
 * into each memory operand, so `as` re-materialises `%hi` per arm through `$at`
 * — two extra `lui`s and the wrong length.
 *
 * The pad byte is reached as an offset cast off the proven PadPort extern
 * (offset 7 is the high half of controller_input.unk_2[2], so it has no named
 * field), which keeps the absolute %hi/%lo pair identical to a bare D_800BE6D7
 * while avoiding a symbol nothing else would reference.
 */

void FUN_8001b2b8(void)
{
    PersistentState *ps =
        (PersistentState *)TENCHU_PERSISTENT_STATE_ADDRESS;

    if (((u8 *)PadPort)[7] != 0) {
        ps->field_0x3b[0xC] |= 1;
    } else {
        ps->field_0x3b[0xC] &= 0xfe;
    }
}
