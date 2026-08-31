#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DeleteConflict(struct ModelType *model);
 *     CONFLICT.C:310, 15 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t3       struct ModelType * model
 *     reg   $t1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

/*
 * DeleteConflict (0x8001a57c) — remove every conflict box owned by `model`
 * from the ConflictObject pool (see ProcItemMakibishi.c / ProcItemDrop.c, which
 * register boxes into this pool). A swap-remove compaction: scan the live
 * range [0, ConflictObjects); when a slot's `.model` matches, overwrite it
 * with the last live element, shrink the count, and rewrite the moved
 * element's `.model->id` to its new slot. The matched slot is re-examined
 * (i is NOT advanced on a hit) so duplicates are all removed. Finally the
 * model is marked free (id = -1) and its CONFLICT/COLLIDE attribute bits are cleared.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `ConflictObject[i] = ConflictObject[count];` is a 0x78-byte, word-aligned STRUCT
 *    ASSIGNMENT — cc1's emit_block_move copies MAX_MOVE_BYTES (4 words = 0x10)
 *    per loop iteration for the aligned bulk (0x70) then the 8-byte remainder
 *    as two lw/sw (the cookbook's "0x50-byte struct assignment becomes the
 *    16-bytes-per-iteration copy loop"). Ghidra renders it as a hand do-while
 *    over invented `position`/`offset` fields and shows the tail as `sh`s —
 *    both wrong; access.py --order proves every copy insn is a full word.
 *  - `count = ConflictObjects - 1;` at the loop TOP: the narrowing
 *    subtraction reads ConflictObjects with `lhu` (only the low 16 bits reach
 *    the s16 result), distinct from the signed `lh` the `i < ConflictObjects`
 *    comparison needs — two un-CSE'd loads of the same gp global, the lhu one
 *    reused for both the `ConflictObjects = count` store and the source index.
 *  - `short i` (signed): sign-extended (sll/sra) at every array-index and at
 *    the `< ConflictObjects` compare; stored raw (sh) into `.model->id`.
 *  - `while (i < ConflictObjects)` → cc1's top-guard (blez) + bottom-test
 *    do-while (duplicate_loop_exit_test). i advances only on the miss branch.
 */

void DeleteConflict(ModelType *model)
{
    short i;
    short count;

    if (model->id != -1)
    {
        i = 0;
        while (i < ConflictObjects)
        {
            count = ConflictObjects - 1;
            if (ConflictObject[i].model == model)
            {
                ConflictObjects = count;
                ConflictObject[i] = ConflictObject[count];
                ConflictObject[i].model->id = i;
            }
            else
            {
                i++;
            }
        }
        model->id = -1;
        model->attribute = model->attribute & ~(MODEL_ATTR_CONFLICT | MODEL_ATTR_COLLIDE);
    }
}
