#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackBowControl(void);
 *     MOTION.C:800, 28 src lines, frame 72 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+56     struct SVECTOR vect
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

/*
 * AttackBowControl (0x8001f5a8, 0x110 bytes) — bow attack-frame callback.
 * PSX.SYM's demo build took NO parameters at all (and never touched
 * FieldIndex/MotionUpdateMode); retail's `.s` proves a real `s16 n` PARAMETER
 * (the `(n<<16)>>14` fused sign-extend+×4-scale on the raw incoming $a0,
 * before anything is stored) selecting a `{min,max}` trigger-frame window —
 * ActATTACK's own (still-unmatched) call sites don't show an argument either
 * (same earlier-build/retail signature drift, on both ends of the call).
 *
 * `D_80097714`/Ghidra's separately-invented `DAT_80097716` are the SAME
 * byte-addressed table read through ONE base (`&D_80097714 + idx`, `idx`
 * already scaled ×4): `min` at +0, `max` at +2 — a `{s16 min, max;}` struct
 * array, not two parallel arrays (the aggregate-splits-into-drifted-D_-
 * symbols pattern, cookbook's gp section). Bound the sole symbol
 * (`D_80097714`) in config/symbols.main.exe.txt since this was its only
 * `.s` referencer.
 *
 * `n` (the parameter) survives THREE calls (Sound/UpdateOrnament/
 * DrawOrnament) in the first half, so it's cached into a callee-saved
 * register at entry and re-read (re-extended via the same shift pair) both
 * inside the first if/else and again for the wholly separate second
 * `if (dtM->count == …max)` check — two independent, unrelated statements,
 * not one cached value.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - **Ghidra's own literal form — `iVar2 = (n<<16)>>14;` as its OWN
 *    statement, THEN `*(s16*)((u8*)D_80097714 + iVar2)` — is load-bearing,
 *    not just a reference style.** Writing the equivalent `D_80097714[n]`
 *    array subscript inline computes the SAME address but in the OPPOSITE
 *    instruction order (base lui/addiu, then the idx sll/sra, then addu);
 *    only when `idx` is a NAMED local assigned as its own prior statement
 *    does cc1 emit the idx shift-pair FIRST (from the preceding statement)
 *    and the base address second (when evaluating the pointer-arithmetic
 *    expression that consumes it) — matching the target exactly. Same
 *    value, same final instructions, pure source-shape-driven ORDER lever.
 *  - **`idx`/`p` are TWO variables per site, not one shared pair** (19 -> 15
 *    bytes). The target holds the table pointer in `$v1` at the range-check
 *    site but `$v0` at the merge site; since cc1 2.8.1 never splits a live
 *    range, one C variable = one hard register for its whole life, so two
 *    registers for one logical value PROVES two variables. Sharing them
 *    built one mega-pseudo whose conflict set was the union of both
 *    fragments: it hard-conflicted with `v0`+`v1`, fell back to `$a0`, and
 *    exiled `count` to `$v1`. Splitting per site (a pure decomposition
 *    change — not one instruction moved) let `p`(85) take `$v1` and `count`
 *    (89) take `$a1`, both matching. NB `idx` is `$v0` at *both* target
 *    sites, which looks like one shared variable but is not: re-sharing it
 *    regressed to 17 and re-exiled `count`. Two disjoint split halves simply
 *    fall back onto the same free register (cookbook: "Splitting a shared
 *    pseudo per site is a PRIORITY lever").
 *  - **The last 15 bytes were a `local_alloc` quantity-ORDER tie, broken by
 *    the redundant-looking `idx2 = n;` copy.** After the split, site 2's
 *    `idx2`/`p2` live in ONE basic block, so `local_alloc` — not
 *    `global_alloc` — colours them. Its two quantities there are the idx
 *    chain (`sll`->`sra`->`addu`->`lh`) and the table base (`lui`->`addiu`,
 *    dead at the `addu`). `local_alloc` orders the SHORTER-lived quantity
 *    first and it takes `$v0` (search order v0,v1,a0,...), so the base won
 *    `$v0` and the idx chain got `$v1` — the mirror of the target. Seeding
 *    `idx2` with a plain copy of `n` before the shift pair BIRTHS the idx
 *    quantity earlier, flipping the order so the idx chain claims `$v0`;
 *    the copy itself is coalesced away, leaving 68/68 identical
 *    instructions. The `$v1`->`$v0` flip also fixes the delay slot at
 *    0x8001f600 for free: reorg duplicates the merge block's leading `sll`
 *    into all four incoming edges, which is only legal when its destination
 *    is dead on the fallthrough — `$v0` is (the fallthrough `lh $v0,2($v1)`
 *    overwrites it), but `$v1` was live there as `p`, forcing our `nop`.
 *    Found by `tools/permute.py`; 13 hand-written respellings of site 2
 *    (inlining `p2`/`idx2`, `n*4`, struct-array subscripts, `s16*` views,
 *    naming the loaded max, a `count2` temp, compare-swap, declaration
 *    order) all plateaued at exactly 15 — the copy is the only lever.
 */

extern s16 D_80097714[]; /* byte-addressed; {min,max} pairs, stride 4 */
extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern void Sound(Humanoid *h, int id);
extern void bow_shoot_logic(s16 kind, VECTOR *start);
extern void UpdateOrnament(OrnamentType *objp, short ry);
extern short DrawOrnament(OrnamentType *objp);

void AttackBowControl(s16 n)
{
    s16 count;
    VECTOR *pos;
    PARAM_ITEM_LAUNCH item; /* PSX.SYM's "PARAM_ITEM_LAUNCH item" (unused here —
                            same 48-byte dead-local frame lever as
                            AttackGunControl; item.h's proven 0x28-byte struct) */
    SVECTOR vect;         /* PSX.SYM's "struct SVECTOR vect" (also unused) */
    s32 idx;
    u8 *p;
    s32 idx2;
    u8 *p2;

    count = dtM->count;
    if (count == 1)
    {
        Sound(Me_MOTION_C, 2);
    }
    else
    {
        idx = (n << 16) >> 14;
        p = (u8 *)D_80097714 + idx;
        if (*(s16 *)p <= count && count < *(s16 *)(p + 2))
        {
            UpdateOrnament(Me_MOTION_C->weapon[2], 0);
            DrawOrnament(Me_MOTION_C->weapon[2]);
        }
    }
    idx2 = n;
    idx2 = (idx2 << 16) >> 14;
    p2 = (u8 *)D_80097714 + idx2;
    if (dtM->count == *(s16 *)(p2 + 2))
    {
        pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xD], 0, 0, 0);
        bow_shoot_logic(0x15, pos);
        Sound(Me_MOTION_C, 3);
    }
}
