#include "common.h"
#include "main.exe.h"

/*
 * ProcItemGun (0x80046528) — the gun item processor. mode 0: muzzle flash —
 * spray a grey (0x7F7F7F) SetBleeds burst from the item's position along a
 * backward direction vector (D_80097B0C, rotated by the owner's facing);
 * mode 1: fire — aim from the item at the launch target stored in
 * item->param (GetVectorRotation), trace the shot (SearchItemTarget2), snap
 * the item to the hit point, register a 100-unit conflict box there, and
 * splash: a big impact + red bleed + hit sound if a Humanoid was hit, a
 * smaller impact + yellow bleed + ricochet sound otherwise; mode 2: dispose.
 *
 * Matching notes (all verified against the original bytes; shares
 * ProcItemKawarimi's dispatch/dispose shape and ProcItemLightningBolt's
 * conflict-box insert block — same store order, `n = InsertConflict` as s32):
 *  - `param = (VECTOR *)item->param;` + `ff = 0xff;` before the entry test:
 *    param's addiu fills the entry branch's delay slot, and BOTH stay
 *    caller-saved here (param → $a1: its only use is GetVectorRotation's 2nd
 *    argument, reached with no intervening call; ff → $v1, used by the entry
 *    compare and case 2's `item->mode = ff`).
 *  - The switch INDEX register ($s2) is callee-saved and reused inside case 1
 *    as the source of every `= 1` store (common/size.pad/coll_mode): cse's
 *    record_jump_equiv on the `beq index,1` taken edge knows the pseudo == 1,
 *    and the constant-register equivalence survives the calls. Plain literal
 *    `1`s in the source produce it — do NOT hand-substitute a variable.
 *  - `vec = D_80097B0C[0];` / `dir = D_80097B14[0];` — the two 8-byte SVECTOR
 *    globals MUST be declared as unknown-size arrays: that makes them
 *    non-small (-G8), so their address builds as split HIGH/LO_SUM through
 *    TWO registers (`lui $v0,%hi / addiu $t3,$v0,%lo`) whose lui reorg/sched
 *    can hoist (into the dispatch delay slot for case 0, into the collision
 *    store run for case 1). A plain `extern SVECTOR D_…;` is -G8-small and
 *    collapses to a fused one-register `la` — one instruction long (the
 *    cookbook's AddItem2 smoke-vector rule, same D_80097Bxx table).
 *  - Case 1's block declares `SVECTOR dir;` then `s32 ry; s32 rx;` — slot
 *    order dir@sp+0x30, ry@0x38, rx@0x3C after the function-scope vec@0x18 +
 *    target@0x20 (scope-entry allocation order).
 *  - `vec.vx = ry;` etc. narrow the s32 out-params: combine folds each
 *    load+truncate to a single lhu.
 *  - Cases 0 and 1 end in duplicated `item->mode = item->mode + 1; return;`
 *    tails, cross-jumped into case 1's copy (Kawarimi's layout lever).
 */

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemGun(struct tag_TItem *item);
 *     ITEM.C:2938, 72 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       struct tag_TItem * item
 *     reg   $s2       struct param_gun * param
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct VECTOR target
 *     reg   $s3       struct Humanoid * IsHuman
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v0       int t
 *     stack sp+48     struct SVECTOR vec
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

#include "item.h"

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see ProcItemDrop.c). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];                /* 0x28 */
    u8 pad[0x10];                 /* 0x68 */
} ConflictObjectType;             /* 0x78 */

extern void GetVectorRotation(VECTOR *from, VECTOR *to, s32 *ry, s32 *rx);
extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot, VECTOR *launch, VECTOR *out);
extern s16 InsertConflict(ModelType *m);
extern void SetImpact(VECTOR *pos, s32 a, s32 b);
extern void SetBleeds(VECTOR *pos, s32 a, s32 b, s32 c, s32 d, s32 col);
extern void SetBleedsDir(VECTOR *pos, SVECTOR *dir, s32 a, s32 b, s32 c, s32 col);
extern void RotateVectorS(SVECTOR *v, s32 rx, s32 ry, s32 rz);
extern ConflictObjectType ConflictObject[];
extern SVECTOR D_80097B0C[];
extern SVECTOR D_80097B14[];

void ProcItemGun(tag_TItem *item)
{
    VECTOR *param;
    u8 ff;
    SVECTOR vec;
    VECTOR target;

    param = (VECTOR *)item->param;
    ff = 0xff;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }
    switch (item->mode)
    {
    case 0:
        vec = D_80097B0C[0];
        RotateVectorS(&vec, item->owner->model->rotate.vx, item->owner->model->rotate.vy, 0);
        SetImpact((VECTOR *)item->locate->locate.coord.t, 0x2000, 0);
        SetBleeds((VECTOR *)item->locate->locate.coord.t, 100, 10, 10, 10, 0x7F7F7F);
        item->mode = item->mode + 1;
        return;

    case 1:
    {
        SVECTOR dir;
        s32 ry;
        s32 rx;
        Humanoid *IsHuman;
        s32 n;

        GetVectorRotation((VECTOR *)item->locate->locate.coord.t, param, &ry, &rx);
        vec.vx = ry;
        vec.vy = rx;
        vec.vz = 0;
        IsHuman = SearchItemTarget2(item->owner, &vec, (VECTOR *)item->locate->locate.coord.t, &target);
        item->locate->locate.coord.t[0] = target.vx;
        item->locate->locate.coord.t[1] = target.vy;
        item->locate->locate.coord.t[2] = target.vz;
        DeleteConflict(item->locate);
        n = InsertConflict(item->locate);
        ConflictObject[n].offset.vx = 0;
        ConflictObject[n].offset.vz = 0;
        ConflictObject[n].offset.vy = 0;
        ConflictObject[n].size.vz = 100;
        ConflictObject[n].size.vy = 100;
        ConflictObject[n].size.vx = 100;
        ConflictObject[n].common = (void *)1;
        ConflictObject[n].size.pad = 1;
        item->coll_size = 100;
        item->coll_ofsY = 0;
        item->coll_mode = 1;
        item->coll_pause = 0;
        dir = D_80097B14[0];
        RotateVectorS(&dir, item->owner->model->rotate.vx, item->owner->model->rotate.vy, 0);
        if (IsHuman != 0)
        {
            SetImpact(&target, 0x6000, 0);
            SetBleedsDir(&target, &dir, 100, 15, 10, 0xFF0000);
            SoundEx(&target, 0x34);
        }
        else
        {
            SetImpact(&target, 0x4000, 0);
            SetBleedsDir(&target, &dir, 100, 15, 10, 0xFFFF00);
            SoundEx(&target, 0x35);
        }
    }
        item->mode = item->mode + 1;
        return;

    case 2:
        if (item->proc == 0)
            return;
        item->mode = ff;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;
    }
}

