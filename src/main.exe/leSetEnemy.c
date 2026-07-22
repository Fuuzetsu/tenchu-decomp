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
 * STATUS: MATCHING — exact 140-byte pure C. The final 33-byte base/offset
 * swap came from top-level `enemy[result]`: ARRAY_REF expands the symbol
 * base before the scaled index, so local allocation gave the base `$v1`
 * and the offset `$a0`, opposite the target. Name the signed byte offset,
 * then form the pointer through INTEGER addition:
 *
 *     offset = result * 0x88;
 *     e = (TEnemyLayout *)(offset + (s32)enemy);
 *
 * This bypasses ARRAY_REF, emits the full scale chain first into `$v1`,
 * materializes `enemy` afterward in `$a0`, and preserves the written
 * offset-first `addu v1,v1,a0`. All seven field stores share `e`.
 *
 * One scheduling detail remains source-significant: write `e->z = z`
 * before `e->r = r`. That places the stack-parameter load before the
 * halfword assignment; sched2 then moves the independent `r` store ahead
 * of the final `z` store, producing the target's physical field order
 * type/ThinkType/nPath/x/y/r/z. Writing the stores lexically in that final
 * order sinks the `z` load after `r` and leaves a 6-byte residual. The two
 * assignments target distinct nonvolatile fields, so both spellings have
 * the same C-visible final layout; only the `z`-before-`r` spelling schedules
 * exactly.
 */


s32 leSetEnemy(s32 type, TThinkType think, s32 x, s32 y, s32 z, s16 r)
{
    s32 idx;
    s32 result;
    s32 offset;
    TEnemyLayout *e;

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
    offset = result * 0x88;
    e = (TEnemyLayout *)(offset + (s32)enemy);
    e->type = (s16)type;
    e->ThinkType = think;
    e->nPath = 0;
    e->x = x;
    e->y = y;
    e->z = z;
    e->r = r;
    return result;
}
