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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * model is marked free (id = -1) and its top two attribute bits are cleared.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `ConflictObject[i] = ConflictObject[count];` is a 0x78-byte, word-aligned STRUCT
 *    ASSIGNMENT — cc1's emit_block_move copies MAX_MOVE_BYTES (4 words = 0x10)
 *    per loop iteration for the aligned bulk (0x70) then the 8-byte remainder
 *    as two lw/sw (the cookbook's "0x50-byte struct assignment becomes the
 *    16-bytes-per-iteration copy loop"). Ghidra renders it as a hand do-while
 *    over invented `position`/`offset` fields and shows the tail as `sh`s —
 *    both wrong; access.py --order proves every copy insn is a full word.
 *  - `short count = ConflictObjects - 1;` at the loop TOP: the narrowing
 *    subtraction reads ConflictObjects with `lhu` (only the low 16 bits reach
 *    the s16 result), distinct from the signed `lh` the `i < ConflictObjects`
 *    comparison needs — two un-CSE'd loads of the same gp global, the lhu one
 *    reused for both the `ConflictObjects = count` store and the source index.
 *  - `short i` (signed): sign-extended (sll/sra) at every array-index and at
 *    the `< ConflictObjects` compare; stored raw (sh) into `.model->id`.
 *  - `while (i < ConflictObjects)` → cc1's top-guard (blez) + bottom-test
 *    do-while (duplicate_loop_exit_test). i advances only on the miss branch.
 */

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see ProcItemDrop.c). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];               /* 0x28 */
    u8 pad[0x10];                /* 0x68 */
} ConflictObjectType;            /* 0x78 */

/* The conflict pool + live count (Ghidra: ConflictObject / ConflictObjects). */
extern ConflictObjectType ConflictObject[];
extern s16 ConflictObjects;

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
        model->attribute = model->attribute & 0x3fff;
    }
}
