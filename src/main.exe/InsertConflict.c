#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short InsertConflict(struct ModelType *model);
 *     CONFLICT.C:283, 23 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s2       struct ModelType * model
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct tag_TItem items[30];
 *     extern struct VECTOR UnitVector2;
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

/*
 * InsertConflict (0x8001a444) — append `model` to the ConflictObject conflict pool
 * (the inverse of DeleteConflict.c; the pool is also filled by ProcItem* via
 * ProcItemMakibishi.c). If `model` is already registered (id != -1) its id is
 * returned unchanged. Otherwise a fresh slot is claimed: abort via SystemOut
 * if the pool is full (> 0x4f live), then store the model, zero `.common`,
 * copy the identity `.position` from UnitVector2 (UnitVector2, a VECTOR) and
 * `.offset`/`.size` from UnitVector (an SVECTOR), memset the result area, and
 * stamp the model's id (= new slot) and attribute (set bit 14, clear bit 15).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - EARLY-RETURN, no shared `ret` variable — this IS the match. Funnelling both
 *    the existing id and the new index through one `int ret` gives `id` a
 *    copy-preference toward the index's callee-saved $s0 (idx is live across
 *    memset), so `id` lands in $s0 too; the target keeps `id` in caller-saved
 *    $v1. `if (id != -1) return id; ... return idx;` breaks that copy-chain so
 *    id's live range never joins the index's. `tools/regalloc.py` named the
 *    $s0->$v0 return copy-chain; breaking it was a 26-byte -> 0 fix (this was
 *    parked NON_MATCHING before the diagnoser existed).
 *  - ConflictObjects (gp-relative s16) is read TWICE, un-CSE'd: `lh` (signed) for
 *    the `>= 0x50` compare, `lhu` (narrowing) for the count capture. Increment
 *    BEFORE the sign-extend (`cnt = ConflictObjects; ConflictObjects = cnt + 1;
 *    idx = (short)cnt;`): the store breaks the memory equivalence so `(short)cnt`
 *    sign-extends the register (sll/sra) instead of CSE-reloading a second `lh`.
 *  - `idx`/`id` are `int` so the short return is a plain move of an already-
 *    sign-extended value (no return sll/sra); the raw u16 `cnt` (its own reg)
 *    feeds `model->id = cnt` (sh) and `cnt + 1`.
 *  - `.position = UnitVector2` is a 16-byte word-aligned VECTOR copy (4x lw/sw);
 *    `.offset`/`.size = UnitVector` are align-2 SVECTOR copies (lwl/lwr+swl/swr).
 *    access.py --order proves every position word is a full `sw`.
 */

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see DeleteConflict.c). */
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

/* Identity constants copied into a fresh slot. UnitVector2 is Ghidra's
 * UnitVector2 (the VECTOR position); UnitVector is the SVECTOR offset/size. */
extern VECTOR UnitVector2;
extern SVECTOR UnitVector;

extern char D_800111F8[];        /* "CONFLICT REGIST FAILURE" */
extern void SystemOut(char *);

short InsertConflict(ModelType *model)
{
    u16 cnt;
    int idx;
    int id;

    id = model->id;
    if (id != -1)
    {
        return id;
    }
    if (ConflictObjects >= 0x50)
    {
        SystemOut(D_800111F8);
    }
    cnt = ConflictObjects;
    ConflictObjects = cnt + 1;
    idx = (short)cnt;
    ConflictObject[idx].model = model;
    ConflictObject[idx].common = 0;
    ConflictObject[idx].position = UnitVector2;
    ConflictObject[idx].offset = UnitVector;
    ConflictObject[idx].size = UnitVector;
    memset(ConflictObject[idx].result, 0, 0x50);
    model->id = cnt;
    model->attribute = (model->attribute | 0x4000) & 0x7fff;
    return idx;
}
