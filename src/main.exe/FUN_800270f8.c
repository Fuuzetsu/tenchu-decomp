#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * FUN_800270f8 (0x800270f8, 0x118 bytes) — toggles bit 0 of ModelType.attribute
 * (item.h: proven s16 field, read/written here as u16 through an offset cast
 * off the same proven Humanoid->model->object[] pointer — the divergent-width
 * cast the cookbook documents for a TU that treats a sibling TU's proven
 * signed field as a bit-flag) across a Humanoid's model part list: always
 * part 0 (root), plus parts 7..min(model->n,13)-1 when that range is
 * non-empty. `hide` (param_2, Ghidra's naming) selects clear-bit0 (0) vs
 * set-bit0 (nonzero). Callers ProcItemNinken and SwimCheck suggest this
 * hides/shows body-part models (e.g. legs) while swimming or using a rope
 * item — no confirmed original name.
 *
 * STATUS: NON_MATCHING — 8 of 280 bytes differ (2 missing instructions,
 * one per loop). Root cause found and fixed:
 *  - The `if/else` polarity for `last` IS the opposite of Ghidra's literal
 *    "assign-then-override" rendering: write `if (0xc < model->n) last =
 *    0xc; else last = model->n - 1;` — this is what actually forces the
 *    genuine TWO separate loads (lh signed for the compare, lhu unsigned
 *    for the narrowing subtract) the target has; Ghidra's literal order
 *    (subtract first, then override) lets cc1 CSE both into one lhu +
 *    sign-extend, one instruction pair short.
 *  - The `hide` dispatch is ALSO opposite-polarity from Ghidra: the target
 *    falls through into the `hide != 0` (set-bit) body first and branches
 *    AWAY to the `hide == 0` (clear-bit) body — write `if (hide != 0) {
 *    ...; return; } ...` (both fixes verified against matchdiff).
 *  - The clear-bit AND uses a register mask (`li $t0,-2; and`), not `andi
 *    0xfffe`, ONLY inside the loop — this is what a plain (non-truncating)
 *    `int attr = *(u16*)&...; attr = attr & ~1; *(u16*)&... = attr;` produces
 *    (the truncating compound form `x &= ~1;`/`x &= 0xfffe;` folds the mask
 *    to 16 bits and emits andi instead) — the one-shot "entry 0" epilogue
 *    store, by contrast, DOES want the plain truncating `&= 0xfffe;` form
 *    (andi). Both confirmed against matchdiff.
 *
 * What's LEFT (the actual residual): each loop's per-iteration body is a
 * consistent 2 instructions (8 bytes total) shorter than the target — the
 * target computes the loop counter through an explicit `move $vN,$a1`
 * "working copy" before scaling it (and copies the incremented value back
 * with a second `move`), where our compiled code scales `$a1` in place with
 * no such redundant copy. Tried and confirmed NOT to change codegen at all
 * (byte-for-byte identical output every time): a separate `s16 idx = i;`
 * local for the array subscript (matches DeleteConflict/FUN_800566fc's
 * narrow-index-copy idiom elsewhere, no effect here); computing the
 * increment from idx (`i = idx + 1;`) instead of from i itself; a `for`
 * loop instead of a `while`; reordering the increment/idx-capture
 * statements. autorules found no win (its only "win" retyped the u16
 * `attribute` field to s8, which the cookbook explicitly flags as a
 * rejectable width-narrowing false-positive — verified: it degrades the
 * `lhu`/`sh` accesses to `lbu`/`sb`, wrong per item.h's proven struct).
 * This has the shape of the cookbook's permuter-immune register/scheduling
 * ties (the `la`-reload tie, the delay-slot-fill tie), but is 8 bytes SHORT
 * of target rather than same-length, so it falls outside the "permute one
 * bounded run" recipe (that's for same-length residuals); a background
 * permuter run here timed out with no verified finding. Parked rather than
 * burning further budget — NEW rule for the cookbook: a genuinely
 * unreproducible "redundant working copy before scaling a loop counter"
 * copy, immune to every idx/order/type respelling tried.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_800270f8", FUN_800270f8);
#else

void FUN_800270f8(Humanoid *human, s16 hide)
{
    ModelArchiveType *model;
    s16 last;
    s16 i;
    s16 idx;
    int attr;

    model = human->model;
    if (0xc < model->n) {
        last = 0xc;
    } else {
        last = model->n - 1;
    }
    if (hide != 0) {
        i = 7;
        while (i <= last) {
            idx = i;
            i = idx + 1;
            attr = *(u16 *)&model->object[idx]->attribute;
            attr = attr | 1;
            *(u16 *)&model->object[idx]->attribute = attr;
        }
        *(u16 *)&model->object[0]->attribute |= 1;
        return;
    }
    i = 7;
    while (i <= last) {
        idx = i;
        i = idx + 1;
        attr = *(u16 *)&model->object[idx]->attribute;
        attr = attr & ~1;
        *(u16 *)&model->object[idx]->attribute = attr;
    }
    *(u16 *)&model->object[0]->attribute &= 0xfffe;
}
#endif /* NON_MATCHING */
