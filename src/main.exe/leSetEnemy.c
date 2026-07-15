#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leSetEnemy(int type, short think, long x, long y, long z, int r);
 *     WORLD.C:1134, 22 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a0       int type
 *     param $a1       short think
 *     param $a2       long x
 *     param $a3       long y
 *     param stack+16  long z
 *     param stack+20  int r
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 * END PSX.SYM */

/*
 * leSetEnemy (0x8003cb7c, 0x8c bytes) — `le`=layout-enemy family (see
 * leResetPath.c for TEnemyLayout, recovered from the Ghidra type export):
 * finds the first dead slot (type == -1) in the enemy-layout table and, if
 * one exists, fills it in from the six parameters and returns its index;
 * returns -1 if the table is full.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - The search loop is a plain do-while with an early `goto found` (cc1
 *    strength-reduces `enemy[idx].type` to a walking pointer automatically,
 *    same as DrawEffect.c/leResetEnemyLayout.c).
 *  - The post-loop merge needs a SEPARATE `result` variable (not reusing
 *    `idx` itself for the found-vs-not-found sentinel): the cookbook's
 *    "pool-search cursor and its post-found continuation can be a separate
 *    pseudo" rule — this reproduces the target's fresh `li -1` at the merge
 *    (rather than CSE-ing the loop's own -1 sentinel register), fixing a
 *    4-byte/1-instruction length gap.
 *  - The guard-clause-with-two-returns exception applies in its LITERAL
 *    `== -1` sense here (`if (result == -1) return -1;` first, success
 *    falls through to its own `return result;` at the very end) — the
 *    opposite (Ghidra's literal `if (result != -1) {...} return -1;`)
 *    relocates the success block to a branch target and is 79 bytes off.
 *
 * STATUS: NON_MATCHING — 33 of 140 bytes differ, all in the `enemy[result]`
 * address computation for the field stores. The draft is arithmetically
 * correct, the right length, and every field STORE matches (same values,
 * same order) — only the base/offset register choice for `&enemy[result]`
 * differs: the target puts the `idx*0x88` offset chain in $v1 (which becomes
 * the final address register, `addu v1,v1,a0`) and the `enemy` base in $a0;
 * this compile swaps them ($a0=chain/result, $v1=base), same two
 * instructions/operand shape (chain register reused as destination, chain
 * operand first), only the hard-register NAMES differ.
 *
 * RTL-confirmed (rtldump --pass combine,greg,lreg): the `.combine` dump's
 * RTL tree already has the CORRECT abstract shape — `(plus offset-chain
 * base)`, offset first — so this is not a fold/expand tree-order bug. The
 * divergence is `local-alloc`'s per-block register assignment: the `enemy`
 * base (`high`+`lo_sum` of the symbol) is emitted at a LOWER insn UID than
 * the `idx*0x88` shift/add chain (base is computed for `&enemy[result]`
 * before the multiply, even though the address is for a plain single-index
 * `arr[idx]`, not a compound `arr[a+b*c]` subscript — the cookbook's
 * EXPAND_SUM mult-first special case is for the LATTER shape only; a bare
 * `SYMBOL + reg*CONST` address for a top-level array goes through
 * ARRAY_REF's own base-first expansion instead, confirmed by the UID order
 * in the dump), so local-alloc's per-block scan hands the base pseudo FIRST
 * pick of the low scratch registers ($v1) and the chain gets what's left
 * ($a0) — the reverse of the target. All 7 field stores CSE onto this ONE
 * computed address (the address pseudo has 8 refs across the block), so the
 * computation is genuinely shared, not per-store.
 *
 * Tried and failed to move it: the `(&enemy[0])[result]` index-first
 * respelling that fixed leAddPath.c's identical-looking tie (no effect here
 * — that rule is specific to struct-member array access through a pointer,
 * not a top-level array, consistent with the ARRAY_REF-vs-PLUS_EXPR
 * distinction above); a cached `TEnemyLayout *e = &enemy[result];` pointer
 * (identical tree post-fold to the direct form, no effect); reordering the
 * `result` copy-for-return relative to the field stores. autorules.py found
 * no mechanical win. tools/permute.py ran two bounded passes (an earlier
 * ~285-iteration/10-min run, and a fresh 300s/~36,000-iteration run this
 * session) and both plateaued at score 60 with no candidate beating this
 * residual — below the C level, a local-alloc ordering tie baked into how
 * this cc1 legitimizes `SYMBOL + reg*CONST` addresses under
 * -msplit-addresses for a single-index top-level array, not a source shape
 * reachable from here. Do not retry without a NEW lever (e.g. a way to make
 * the front end route the address through a genuine PLUS_EXPR/EXPAND_SUM
 * path instead of ARRAY_REF's own expansion).
 */

typedef struct
{
    s16 type;
    s16 ThinkType;
    s16 nPath;
    s32 x;
    s32 y;
    s32 z;
    s16 r;
    s16 pad;
    VECTOR path[7];
} TEnemyLayout; /* 0x88 */

extern TEnemyLayout enemy[0x1E];

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/leSetEnemy", leSetEnemy);
#else
s32 leSetEnemy(s32 type, s16 think, s32 x, s32 y, s32 z, s16 r)
{
    s32 idx;
    s32 result;

    idx = 0;
    do
    {
        if (enemy[idx].type == -1)
        {
            result = idx;
            goto found;
        }
        idx++;
    } while (idx < 0x1E);
    result = -1;
found:
    if (result == -1)
        return -1;
    enemy[result].type = (s16)type;
    enemy[result].ThinkType = think;
    enemy[result].nPath = 0;
    enemy[result].x = x;
    enemy[result].y = y;
    enemy[result].r = r;
    enemy[result].z = z;
    return result;
}
#endif
