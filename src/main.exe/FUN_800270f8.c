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
 * Matching source facts:
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
 *  - `model->object[i++]` is the narrow postincremented subscript that makes
 *    cc1 preserve the target's explicit working copy of `i` around the scale.
 *  - The pointer and value temporaries are block-local to EACH loop. Sharing
 *    them across the two disjoint loops merges their allocnos, raises the
 *    value's priority, and swaps the target's pointer `$a0` / value `$v0`.
 */

void FUN_800270f8(Humanoid *human, s16 hide)
{
    ModelArchiveType *model;
    s16 last;
    s16 i;

    model = human->model;
    if (0xc < model->n) {
        last = 0xc;
    } else {
        last = model->n - 1;
    }
    if (hide != 0) {
        i = 7;
        while (i <= last) {
            u16 *attribute;
            int attr;

            attribute = (u16 *)&model->object[i++]->attribute;
            attr = *attribute;
            attr = attr | 1;
            *attribute = attr;
        }
        *(u16 *)&model->object[0]->attribute |= 1;
        return;
    }
    i = 7;
    while (i <= last) {
        u16 *attribute;
        int attr;

        attribute = (u16 *)&model->object[i++]->attribute;
        attr = *attribute;
        attr = attr & ~1;
        *attribute = attr;
    }
    *(u16 *)&model->object[0]->attribute &= 0xfffe;
}
