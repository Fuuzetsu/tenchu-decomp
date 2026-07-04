#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * InsertConflict (0x8001a444) — append `model` to the D_800BC108 conflict pool
 * (the inverse of DeleteConflict.c; the pool is also filled by ProcItem* via
 * ProcItemMakibishi.c). If `model` is already registered (id != -1) its id is
 * returned unchanged. Otherwise a fresh slot is claimed: abort via SystemOut
 * if the pool is full (> 0x4f live), then store the model, zero `.common`,
 * copy the identity `.position` from UnitVector2 (D_80086764, a VECTOR) and
 * `.offset`/`.size` from UnitVector (an SVECTOR), memset the result area, and
 * stamp the model's id (= new slot) and attribute (set bit 14, clear bit 15).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - ConflictObjects (gp-relative s16) is read TWICE, un-CSE'd: `lh` (signed,
 *    full value) for the `>= 0x50` full-pool compare, and `lhu` (narrowing) for
 *    the count capture — the DeleteConflict lhu-vs-lh split. The capture must be
 *    incremented BEFORE the sign-extend (`cnt = ConflictObjects;
 *    ConflictObjects = cnt + 1; idx = (short)cnt;`): the store to ConflictObjects
 *    breaks the memory equivalence so `(short)cnt` sign-extends the register
 *    (sll/sra) instead of CSE-reloading it as a second `lh`.
 *  - The count keeps two live forms (both callee-saved across memset): the raw
 *    u16 `cnt` (s1) feeds `model->id = cnt` (sh) and `cnt + 1`; the widened
 *    `int idx` (s0) is the array index AND the return value. This is m2c's
 *    temp_s1 (u16) / temp_s0 (s16). `idx`/`ret`/`id` must be `int` so the short
 *    return is a plain move of an already-sign-extended value (no return sll/sra).
 *  - `.position = D_80086764` is a 16-byte word-aligned VECTOR copy (4× lw/sw);
 *    `.offset`/`.size = UnitVector` are align-2 SVECTOR copies (lwl/lwr+swl/swr),
 *    even though the field offsets happen to be word-aligned. access.py --order
 *    proves every position word is a full `sw`.
 *
 * STATUS: NON_MATCHING — 26 bytes (a pure register-allocation tie, below the C
 * level). The draft is arithmetically correct and the exact right length; only
 * the register of the `model->id` load (`id`) differs and cascades into the
 * epilogue schedule. The target keeps `id` in caller-saved $v1 (separate from
 * the sign-extended index in $s0); cc1 here pins `id` to callee-saved $s0,
 * where it shares the slot with the index. The greg dump shows why: `ret = id`
 * plus `ret = idx` make cc1's local-alloc give `id` a transitive preference for
 * $s0 (idx's callee-saved home), and `id` conflicts with $v0 (ret) so it can't
 * take ret's register. The two requirements are mutually exclusive from C: an
 * `int` return value (needed so the short return is a bare move with no extra
 * sll/sra) forces the ret↔idx copy that pins `id` to $s0, while a `short` return
 * value frees `id` to $v1 but re-sign-extends the result (one insn too long).
 * Every structural form (two-return, single-var, idx-inline/-named, u16-vs-short
 * raw, do{}while(0) re-weighting, first-use/decl reordering, attribute-temp
 * reuse) reproduces the same 26-byte swap; a ~70k-iteration decomp-permuter run
 * never beat the base (its only improvements were semantically invalid, leaving
 * `ret` uninitialised on the id != -1 path). It's a combine/allocation-order tie
 * the source cannot steer.
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
extern ConflictObjectType D_800BC108[];
extern s16 ConflictObjects;

/* Identity constants copied into a fresh slot. D_80086764 is Ghidra's
 * UnitVector2 (the VECTOR position); UnitVector is the SVECTOR offset/size. */
extern VECTOR D_80086764;
extern SVECTOR UnitVector;

extern char D_800111F8[];        /* "CONFLICT REGIST FAILURE" */
extern void SystemOut(char *);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/InsertConflict", InsertConflict);
#else
short InsertConflict(ModelType *model)
{
    u16 cnt;
    int idx;
    int id;
    int ret;

    id = model->id;
    ret = id;
    if (id == -1)
    {
        if (ConflictObjects >= 0x50)
        {
            SystemOut(D_800111F8);
        }
        cnt = ConflictObjects;
        ConflictObjects = cnt + 1;
        idx = (short)cnt;
        D_800BC108[idx].model = model;
        D_800BC108[idx].common = 0;
        D_800BC108[idx].position = D_80086764;
        D_800BC108[idx].offset = UnitVector;
        D_800BC108[idx].size = UnitVector;
        memset(D_800BC108[idx].result, 0, 0x50);
        ret = idx;
        model->id = cnt;
        model->attribute = (model->attribute | 0x4000) & 0x7fff;
    }
    return ret;
}
#endif

// Ghidra decompilation (reference):
//
// short InsertConflict(ModelType *model)
// {
//   sVar4 = ConflictObjects;
//   sVar6 = model->id;
//   if (model->id == -1) {
//     if (0x4f < ConflictObjects) {
//       SystemOut((uchar *)"CONFLICT REGIST FAILURE");   /* does not return */
//     }
//     iVar7 = (int)ConflictObjects;
//     ConflictObjects = ConflictObjects + 1;
//     ConflictObject[iVar7].model = model;
//     ConflictObject[iVar7].common = (undefined *)0x0;
//     ConflictObject[iVar7].position.vx = UnitVector2.vx;  /* ... (VECTOR copy) */
//     ConflictObject[iVar7].offset.vx = UnitVector.vx;     /* ... (SVECTOR copy) */
//     ConflictObject[iVar7].size.vx  = UnitVector.vx;      /* ... (SVECTOR copy) */
//     memset(ConflictObject[iVar7].result,'\0',0x50);
//     model->id = sVar4;
//     model->attribute = model->attribute & 0x7fffU | 0x4000;
//     sVar6 = sVar4;
//   }
//   return sVar6;
// }
//
// m2c (register temps straight from the asm):
//
// s16 InsertConflict(void *arg0) {
//     temp_v1 = arg0->unk58;              // id
//     var_v0 = temp_v1;                   // ret = id
//     if (temp_v1 == -1) {
//         if (ConflictObjects >= 0x50) SystemOut(&D_800111F8);
//         temp_s1 = (u16) ConflictObjects;   // cnt (lhu)
//         temp_s0 = (s16) temp_s1;           // idx (sll/sra)
//         ConflictObjects = temp_s1 + 1;
//         (index temp_s0*0x78 + &D_800BC108) fields set; memset; ...
//         var_v0 = temp_s0;                  // ret = idx
//         arg0->unk58 = (s16) temp_s1;       // model->id = cnt
//         arg0->unk5A = (u16)((arg0->unk5A | 0x4000) & 0x7FFF);
//     }
//     return var_v0;
// }
