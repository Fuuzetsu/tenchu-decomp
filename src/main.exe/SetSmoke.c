#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSmoke(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:835, 21 src lines, frame 56 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
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
 *     param $s5       struct VECTOR * pos
 *     param $s6       struct SVECTOR * vect
 *     param $s7       short n
 *     param $s3       short time
 *     reg   $s4       short i
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct SmokeType * smoke
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * Matching notes (all verified against the raw .s; the loop.c mechanics were
 * root-caused in the gcc-2.8.1 sources with -da RTL dumps — see the cookbook's
 * "loop.c invariant motion is a THRESHOLD economy" section, which this
 * function established):
 *  - Same EffectSlot[200] pool search shape as SetExplosion (do{...}while
 *    (count<200), ef=&dmy AFTER the loop, count++ BEFORE the proc test),
 *    wrapped in an OUTER `do { ... } while (1);` spawning n particles.
 *  - `base = EffectSlot;` is the FIRST statement of the outer loop body,
 *    BEFORE the `count = 0;` and the guard. Position is load-bearing twice
 *    over: (a) a set of a USER variable inside a loop is only hoistable by
 *    loop.c when it is guaranteed to execute once the loop is entered
 *    (scan_loop's third eligibility case), i.e. it must precede the
 *    conditional `return`; being a user variable it unifies BOTH EffectSlot
 *    uses (`base + idx` and the wrap reset) into one long-lived pseudo whose
 *    savings*lifetime clears the move threshold, landing `lui/addiu` in the
 *    prologue with base cached in $s7 for the whole function. (b) The two
 *    moves (high + lo_sum) decay loop.c's move threshold by 3 each
 *    (move_movables: `threshold -= 3` per move), which is EXACTLY what keeps
 *    the `time<<16` chain of the bright line un-hoisted later (29 → 23 after
 *    base, then the %100 magic; 23*2*3 < 153). Writing `base = EffectSlot;`
 *    ABOVE the loop leaves only one -3 decay before that decision and the
 *    time<<16 chain hoists into the prologue, spilling n/time to the stack.
 *  - The guard is `if (i >= n) return;` with `i` FIRST. Operand order controls
 *    which extension is emitted first (op0 then op1): i-first puts n's
 *    sign-extension pair immediately before the slt, giving it lifetime 2 —
 *    under loop.c's move formula (threshold*savings*lifetime >= insn_count,
 *    29*2*2=116 < 153) it stays IN the loop, where combine folds both
 *    extensions into the no-sra `sll v0,i,16 / sll v1,n,16 / slt` compare and
 *    `n` stays RAW in $fp. The reversed spelling `if (n <= i)` gives n's pair
 *    lifetime 4 (29*2*4=232 >= 153): loop.c hoists the widening into a
 *    callee-saved register and the compare degrades to sll/sra/slt.
 *  - `count = 0;` sits between `base = ...` and the guard; it lands in the
 *    guard's branch delay slot (`beqz / addu a1,zero,zero`).
 *  - `i` must be `short` (PSX.SYM: `reg $s4 short i`) for the double-shift
 *    HImode compare idiom.
 *  - SmokeType reuses ExplosionType's vec@0x0/pos@0x8/rotate@0x18/scale@0x1c
 *    layout plus mode@0x20/bright@0x21/unk22@0x22.
 *  - `scale = rand() % 0x2000 + 0x1000;` stored FIRST, before `rotate`.
 *  - Each `vec.v{x,y,z}` jitter is `vect->v{x,y,z} + (rand() % 100 - 50)`
 *    (fold-reassociation rule: `A + (B - C)` reassociates into the compiled
 *    `(A - C) + B` order).
 *  - `m = smoke->mode - 1;` MUST be its own statement: inlined as
 *    `(smoke->mode - 1) - (time/2 + r%time)`, fold reassociates the literal
 *    onto the sum (`mode - (sum + 1)`, an `addiu +1` AFTER the addu); the
 *    temp hides the -1 from fold so the target's `addiu v0,v0,-1` on the
 *    reloaded mode survives. The `smoke->mode` reference itself reloads via
 *    a fresh lbu (do NOT cache mode's computed value).
 *  - `rand() % time` is the only variable-divisor division (needs
 *    `--expand-div`: Build.hs/permute.py `SetSmoke` entries).
 */
extern void DrawSmoke(TEffectSlot *ef);

void SetSmoke(VECTOR *pos, SVECTOR *vect, short n, short time)
{
    short i;
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    SmokeType *smoke;
    int r;
    int m;

    i = 0;
    do
    {
        base = EffectSlot;
        count = 0;
        if (i >= n)
        {
            return;
        }
        idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
        slot = base + idx;
        do
        {
            idx = idx + 1;
            slot = slot + 1;
            if (199 < idx)
            {
                slot = base;
                idx = 0;
            }
            count = count + 1;
            if (slot->proc == 0)
            {
                CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = idx + 1;
                if (199 < idx + 1)
                {
                    CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_ = 0;
                }
                ef = slot;
                goto found;
            }
        } while (count < 200);
        ef = &dmy;
found:
        smoke = &ef->param.smoke;
        r = rand();
        smoke->scale = r % 0x2000 + 0x1000;
        smoke->rotate = (rand() % 360) * 0x1000;
        smoke->pos.vx = pos->vx;
        smoke->pos.vy = pos->vy;
        smoke->pos.vz = pos->vz;
        smoke->vec.vx = vect->vx + (rand() % 100 - 50);
        smoke->vec.vy = vect->vy + (rand() % 100 - 50);
        smoke->vec.vz = vect->vz + (rand() % 100 - 50);
        smoke->mode = time + rand() % 0xa0;
        r = rand();
        i = i + 1;
        smoke->unk22 = 0;
        m = smoke->mode - 1;
        smoke->bright = m - (time / 2 + r % time);
        ef->proc = (void (*)())DrawSmoke;
    } while (1);
}

// triage: MEDIUM — 197 insns, mul/div, 1 loop, 1 callees, ~0.09 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
