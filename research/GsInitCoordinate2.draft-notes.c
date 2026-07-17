#include "common.h"
#include "main.exe.h"

/*
 * GsInitCoordinate2 (0x80065064) — stock PsyQ libgs: initialise a coordinate
 * system. Reset `base`'s matrix to the identity, hang it under `super`, and — if
 * `super` is a real node rather than the WORLD sentinel — link it back as
 * super's sub.
 *
 * `super > (GsCOORDINATE2 *)1` is PsyQ's WORLD test; as a pointer compare it is
 * UNSIGNED, which is the `sltiu a0,a0,2` (skip when super < 2). The write-back
 * reads `base->super` rather than reusing the parameter — the target reloads it
 * (`lw v0,72(a3)`), so the source really does say `base->super->sub`.
 *
 * The identity reset is a whole-MATRIX struct assignment; cc1 expands it to the
 * word-by-word copy out of GsIDMATRIX.
 */
extern MATRIX GsIDMATRIX;

/*
 * STATUS: NON_MATCHING — LENGTH MISMATCH, 27 insns vs the target's 28. The
 * SEMANTICS are right (Ghidra agrees, and the struct-copy expansion, the WORLD
 * test, and the base->super reload all reproduce); the residual is address
 * materialisation + register choice, and it is one insn, not a byte tie.
 *
 *   TARGET                        OURS
 *     move  a3,a1                 (absent)
 *     lui   a2,0x800c             lui   v0,0x800c
 *     addiu a2,a2,25856           addiu t0,v0,25856
 *     ... scratch v0,v1,a1        ... scratch v1,a2,a3
 *     ... base in a3              ... base in a1
 *
 * THE TELL: the target materialises &GsIDMATRIX into ONE register in place
 * (`lui a2 / addiu a2,a2`) and then uses a1 as a COPY SCRATCH — which is why it
 * must first move base out of a1 into a3, and that move is the whole length
 * difference. We split the address across TWO registers (`lui v0 / addiu t0,v0`)
 * and reach for t0. mips.h defines no REG_ALLOC_ORDER so find_reg walks
 * numerically (global.c:960, local-alloc.c:2266) — the target uses NO t-register
 * at all (a0..a3, v0, v1), so our t0 says we hold one more value live than it
 * does, at the point where the address is formed.
 *
 * So the question is the ADDRESS SHAPE, not the copy: what makes cc1 form
 * &GsIDMATRIX in place rather than through a second register? See the cookbook's
 * addressing-shape family and `tools/maspsxflags.py` (already run, no gp-extern
 * needed). Everything else is already exact.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GsInitCoordinate2", GsInitCoordinate2);
#else
void GsInitCoordinate2(GsCOORDINATE2 *super, GsCOORDINATE2 *base)
{
    base->coord = GsIDMATRIX;
    base->super = super;
    base->flg = 0;
    if (super > (GsCOORDINATE2 *)1) {
        base->super->sub = base;
    }
}
#endif
