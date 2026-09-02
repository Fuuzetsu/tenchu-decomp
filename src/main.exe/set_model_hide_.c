#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * set_model_hide_ (0x800270f8, 0x118 bytes) — toggles bit 0 of ModelType.attribute
 * (a proven s16 field, read/written by item.h's shared body-part macros
 * through their u16 flag view) across a Humanoid's model part list: always
 * part 0 (root), plus parts 7..min(model->n,13)-1 when that range is
 * non-empty. `hide` (param_2, Ghidra's naming) selects clear-bit0 (0) vs
 * set-bit0 (nonzero). Callers ProcItemNinken and SwimCheck suggest this
 * hides/shows body-part models (e.g. legs) while swimming or using a rope
 * item — no confirmed original name.
 *
 * Matching source facts:
 *  - The `if/else` polarity for `last` IS the opposite of Ghidra's literal
 *    "assign-then-override" rendering: compare against
 *    `MODEL_PART_BODY_LAST` first, then use `model->n - 1` in the other arm.
 *    This is what actually forces the
 *    genuine TWO separate loads (lh signed for the compare, lhu unsigned
 *    for the narrowing subtract) the target has; Ghidra's literal order
 *    (subtract first, then override) lets cc1 CSE both into one lhu +
 *    sign-extend, one instruction pair short.
 *  - The `hide` dispatch is ALSO opposite-polarity from Ghidra: the target
 *    falls through into the `hide != 0` (set-bit) body first and branches
 *    AWAY to the `hide == 0` (clear-bit) body — write `if (hide != 0) {
 *    ...; return; } ...` (both fixes verified against matchdiff).
 *  - The clear-bit AND uses a register mask (`li $t0,-2; and`), not `andi
 *    0xfffe`, ONLY inside the loop. SHOW_HUMANOID_BODY_PARTS keeps the
 *    loaded u16 promoted to int, which produces that non-truncating form
 *    (the compound form `x &= ~1;`/`x &= 0xfffe;` folds the mask
 *    to 16 bits and emits andi instead) — the one-shot "entry 0" epilogue
 *    store, by contrast, DOES want the plain truncating `&= 0xfffe;` form
 *    (andi). Both confirmed against matchdiff.
 *
 *  - `model->object[i++]` is the narrow postincremented subscript that makes
 *    cc1 preserve the target's explicit working copy of `i` around the scale.
 *  - SHOW_HUMANOID_BODY_PARTS and HIDE_HUMANOID_BODY_PARTS deliberately
 *    declare their pointer and value temporaries inside each expansion.
 *    Sharing them across the two disjoint loops merges their allocnos,
 *    raises the value's priority, and swaps the target's pointer `$a0` /
 *    value `$v0`.
 */

void set_model_hide_(Humanoid *human, s16 hide)
{
    ModelArchiveType *model;
    s16 last;
    s16 i;

    model = human->model;
    if (model->n > MODEL_PART_BODY_LAST)
    {
        last = MODEL_PART_BODY_LAST;
    }
    else
    {
        last = model->n - 1;
    }
    if (hide != 0)
    {
        HIDE_HUMANOID_BODY_PARTS(model, last, i);
        return;
    }
    SHOW_HUMANOID_BODY_PARTS(model, last, i);
}
