#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackBowControl(void);
 *     MOTION.C:800, 28 src lines, frame 72 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * `BowTiming`/Ghidra's separately-invented `DAT_80097716` are the SAME
 * byte-addressed table read through ONE base (`&BowTiming + idx`, `idx`
 * already scaled ×4): `min` at +0, `max` at +2 — a `{s16 min, max;}` struct
 * array, not two parallel arrays (the aggregate-splits-into-drifted-D_-
 * symbols pattern, cookbook's gp section). Bound the sole symbol
 * (`BowTiming`) in config/symbols.main.exe.txt since this was its only
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
 *  - The two-stage address construction is load-bearing, but the cast need
 *    not be repeated at each read. Assign each byte offset as its own
 *    statement, then pass it to `BowTimingFromByteOffset`. The inlined
 *    helper preserves the index shift pair before the table base while
 *    exposing a const `BowTimingEntry *` at both use sites. Natural
 *    element-scaled spellings (`BowTiming + n`, `&BowTiming[n]`, and
 *    `&BowTiming[idx >> 2]`) all reverse those instruction groups.
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

typedef struct BowTimingEntry
{
    s16 min;
    s16 max;
} BowTimingEntry;

extern BowTimingEntry BowTiming[];
extern Humanoid *Me_MOTION_C;
extern void bow_shoot_logic(s16 kind, VECTOR *start);
extern void UpdateOrnament(OrnamentType *objp, short ry);
extern short DrawOrnament(OrnamentType *objp);

static inline const BowTimingEntry *BowTimingFromByteOffset(s32 byte_offset)
{
    return (const BowTimingEntry *)((const u8 *)BowTiming + byte_offset);
}

void AttackBowControl(s16 n)
{
    s16 count;
    VECTOR *pos;
    PARAM_ITEM_LAUNCH item; /* PSX.SYM's "PARAM_ITEM_LAUNCH item" (unused here —
                            same 48-byte dead-local frame lever as
                            AttackGunControl; item.h's proven 0x28-byte struct) */
    SVECTOR vect;           /* PSX.SYM's "struct SVECTOR vect" (also unused) */
    s32 byte_offset;
    const BowTimingEntry *p;
    s32 byte_offset2;
    const BowTimingEntry *p2;

    count = dtM->count;
    if (count == 1)
    {
        Sound(Me_MOTION_C, CHAR_SE_ATTACK);
    }
    else
    {
        byte_offset = n << 2;
        p = BowTimingFromByteOffset(byte_offset);
        if (p->min <= count && count < p->max)
        {
            UpdateOrnament(Me_MOTION_C->weapon[2], 0);
            DrawOrnament(Me_MOTION_C->weapon[2]);
        }
    }
    byte_offset2 = n;
    byte_offset2 = (s16)byte_offset2 << 2;
    p2 = BowTimingFromByteOffset(byte_offset2);
    if (dtM->count == p2->max)
    {
        pos = GetAbsolutePosition(Me_MOTION_C->model->object[0xD], 0, 0, 0);
        bow_shoot_logic(ITEM_ARROW, pos);
        Sound(Me_MOTION_C, CHAR_SE_ATTACK_ALT);
    }
}
