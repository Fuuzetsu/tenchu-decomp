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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *    (count<200), slot=&dmy AFTER the loop, count++ BEFORE the proc test),
 *    wrapped in an OUTER `do { ... } while (1);` spawning n particles.
 *  - The source indexes `EffectSlot[idx]` directly. Loop strength reduction
 *    creates one long-lived array-base pseudo, and loop.c hoists its
 *    `lui/addiu` pair into the prologue with the base cached in $s7. The old
 *    source-level `base = EffectSlot` alias was redundant: removing the
 *    declaration, assignment, and every use together preserves the bytes.
 *    The two generated address moves still decay loop.c's move threshold by 3
 *    each
 *    (move_movables: `threshold -= 3` per move), which is EXACTLY what keeps
 *    the `time<<16` chain of the bright line un-hoisted later (29 → 23 after
 *    address pair, then the %100 magic; 23*2*3 < 153). Partially rewriting
 *    the scan changes this economy; the whole direct-array graph does not.
 *  - The guard is `if (i >= n) return;` with `i` FIRST. Operand order controls
 *    which extension is emitted first (op0 then op1): i-first puts n's
 *    sign-extension pair immediately before the slt, giving it lifetime 2 —
 *    under loop.c's move formula (threshold*savings*lifetime >= insn_count,
 *    29*2*2=116 < 153) it stays IN the loop, where combine folds both
 *    extensions into the no-sra `sll v0,i,16 / sll v1,n,16 / slt` compare and
 *    `n` stays RAW in $fp. The reversed spelling `if (n <= i)` gives n's pair
 *    lifetime 4 (29*2*4=232 >= 153): loop.c hoists the widening into a
 *    callee-saved register and the compare degrades to sll/sra/slt.
 *  - `count = 0;` is the first statement before the guard; it lands in the
 *    guard's branch delay slot (`beqz / addu a1,zero,zero`).
 *  - `i` must be `short` (PSX.SYM: `reg $s4 short i`) for the double-shift
 *    HImode compare idiom.
 *  - SmokeType reuses ExplosionType's vec@0x0/pos@0x8/rotate@0x18/scale@0x1c
 *    PSX.SYM layout plus retail's sprite selector at +0x22.
 *  - `scale = rand() % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;` stored FIRST, before `rotate`.
 *  - Each `vec.v{x,y,z}` jitter is `vect->v{x,y,z} + (rand() % 100 - 50)`
 *    (fold-reassociation rule: `A + (B - C)` reassociates into the compiled
 *    `(A - C) + B` order).
 *  - `m = smoke->time - 1;` MUST be its own statement: inlined as
 *    `(smoke->time - 1) - (time/2 + r%time)`, fold reassociates the literal
 *    onto the sum (`mode - (sum + 1)`, an `addiu +1` AFTER the addu); the
 *    temp hides the -1 from fold so the target's `addiu v0,v0,-1` on the
 *    reloaded time survives. The `smoke->time` reference itself reloads via
 *    a fresh lbu (do NOT cache mode's computed value).
 *  - `rand() % time` is the only variable-divisor division (needs
 *    `--expand-div`: Build.hs/permute.py `SetSmoke` entries).
 */
extern void DrawSmoke(TEffectSlot *ef);

void SetSmoke(VECTOR *pos, SVECTOR *vect, short n, short time)
{
    short i;
    int idx;
    TEffectSlot *slot;
    int count;
    SmokeType *smoke;
    int r;
    int m;

    i = 0;
    do
    {
        count = 0;
        if (i >= n)
        {
            return;
        }
        idx = EFFECT_CURSOR_;
        do
        {
            idx++;
        if (idx >= N_EFFECT_SLOTS)
            {
                idx = 0;
            }
            count++;
            if (EffectSlot[idx].proc == 0)
            {
                EFFECT_CURSOR_ = idx + 1;
                if (EFFECT_CURSOR_ >= N_EFFECT_SLOTS)
                {
                    EFFECT_CURSOR_ = 0;
                }
                slot = &EffectSlot[idx];
                goto found;
            }
        } while (count < N_EFFECT_SLOTS);
        slot = &dmy;
    found:
        smoke = &slot->param.smoke;
        r = rand();
        smoke->scale = r % SMOKE_SCALE_SPREAD + SMOKE_SCALE_MIN;
        smoke->rotate = (rand() % 360) * FIXED_ONE;
        smoke->pos.vx = pos->vx;
        smoke->pos.vy = pos->vy;
        smoke->pos.vz = pos->vz;
        smoke->vec.vx = vect->vx + (rand() % 100 - 50);
        smoke->vec.vy = vect->vy + (rand() % 100 - 50);
        smoke->vec.vz = vect->vz + (rand() % 100 - 50);
        smoke->time = time + rand() % 160;
        r = rand();
        i++;
        smoke->sprite = SMOKE_SPRITE_NORMAL;
        m = smoke->time - 1;
        smoke->evtime = m - (time / 2 + r % time);
        slot->proc = DrawSmoke;
    } while (1);
}
