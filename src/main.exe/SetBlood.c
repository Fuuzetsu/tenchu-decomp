#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBlood(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:745, 45 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s3       struct VECTOR * pos
 *     param $s5       struct SVECTOR * vect
 *     param $s7       short n
 *     param $fp       short time
 *     reg   $s4       short i
 *     reg   $s6       struct AreaNodeType * hint
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct BloodType * blood
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

/*
 * MATCHED. The pool-scan cursor's initial address computation `slot = base +
 * idx;` needed the INTEGER-SUM spelling, not pointer arithmetic:
 *     slot = (TEffectSlot *)(idx * sizeof(TEffectSlot) + (int)base);
 * Plain `base + idx` pointer arithmetic folds to a base-first `addu $a0,
 * $s6,$v0` regardless of source spelling (`base+idx`, `idx+base`,
 * `&base[idx]` all identical post-fold); the target has index-first `addu
 * $a0,$v0,$s6`. Cookbook rule confirmed here: "Pointer arithmetic normalises
 * to base+index; only INTEGER addition keeps operand order" — spelling the
 * same address as an explicit `idx*sizeof + (int)base` integer sum (instead
 * of `base[idx]`/`base+idx`) reaches the index-first `addu` because integer
 * PLUS doesn't get the pointer-arithmetic canonicalization fold does to
 * array/pointer addition. This was previously (wrongly) parked as
 * "permuter-immune, sub-C-level, no source lever" — it had a lever, just not
 * the one tried (reordering statements/loop shape instead of the operand's
 * own type). SetHinoko (same TU, identical residual) fixed the same way.
 *
 * Matching notes (all verified against the original bytes):
 *  - The demo's PSX.SYM records a FOUR-argument prototype (pos, vect, n,
 *    time). Retail's three callers (ActDAMAGE, CVAupdate, DamageControl x2)
 *    each set only $a0/$a1/$a2 before `jal SetBlood`, and the callee itself
 *    never reads $a3 — the SVECTOR* "spread" parameter was dropped between
 *    the demo and retail builds (every per-axis jitter below now comes
 *    straight from rand(), with no live use of a spread vector anywhere in
 *    the body). Retail SetBlood is a plain THREE-argument
 *    `void SetBlood(VECTOR *pos, short n, short time)` — verified from the
 *    callers' own register setup, not assumed from PSX.SYM.
 *  - GetAreaMapLevel's return is discarded (called for a side effect only);
 *    `hint = FieldArea;` is a separate global read right after, not
 *    GetAreaMapLevel's result. Ghidra drops GetAreaMapLevel's stack-passed
 *    5th arg (the `0` mode flag) — cookbook: Ghidra undercounts stack args.
 *  - The whole per-particle fill (spawn `n` particles) is a HAND-ROLLED
 *    `outer: if (!(i < n)) goto end; ...; i++; goto outer; end:` — not a
 *    real while/for and not even while(1)+break. A genuine loop here lets
 *    loop.c hoist the shared %120/%60 magic-multiply constant AND the
 *    `time/2` split (both loop-invariant across outer iterations, neither
 *    depends on `i`) all the way to the function's prologue; the target
 *    recomputes both fresh every outer iteration, right where they're first
 *    used. Only the hand-rolled goto form (no loop notes at all) suppresses
 *    that invariant motion (cookbook: "a top-test loop that never hoists its
 *    invariants is a hand-rolled goto loop, not while(1)+break").
 *  - The inner EffectSlot[200] search is the same round-robin do-while as
 *    SetExplosion/SetImpact (`ef = &dmy;` sits AFTER the loop). Verified
 *    from THIS function's own asm (don't assume a sibling's shape): the
 *    "occupied" branch's delay slot unconditionally increments `count`
 *    regardless of outcome, so `count = count + 1;` sits BEFORE the
 *    `if (slot->proc == 0)` test — SetExplosion's order, not SetImpact's.
 *  - Field store order follows Ghidra's own rendering exactly: unk22,
 *    scale, rotate, px, py, pz, vx, vy, vz, time (branch), `i++`, a combined
 *    mode+bright halfword store, hint, unk23, and `proc` last (it lands in
 *    the closing loop-jump's delay slot).
 *  - `mode`/`bright` are both compile-time constants here (0x80 and 0), so
 *    the two adjacent bytes compile as ONE `sh` — write it via the same
 *    cast Ghidra shows (`*(u16 *)&fp->mode = 0x80;`), not two `sb`s (that
 *    only happens elsewhere in this family when one of the two fields is a
 *    runtime value — see SetImpact/SetHinoko).
 *  - `time`'s jitter needs a genuine variable division (`rand() % half2`),
 *    guarded by ASPSX's break 7/break 6 — needs `--expand-div`
 *    (Build.hs/permute.py).
 */
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z,
                            int mode);
extern void DrawBlood(TEffectSlot *ef);

void SetBlood(VECTOR *pos, short n, short time)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    BloodType *fp;
    struct AreaNodeType *hint;
    short i;
    int half;
    int half2;

    GetAreaMapLevel(GlobalAreaMap, pos->vx, pos->vy, pos->vz, 0);
    hint = FieldArea;
    base = EffectSlot;
    i = 0;
outer:
    if (!(i < n))
    {
        goto end;
    }
    {
        count = 0;
        idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
        slot = (TEffectSlot *)(idx * sizeof(TEffectSlot) + (int)base);
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
        fp = &ef->param.blood;
        do
        {
            fp->unk22 = rand() % 2;
            fp->scale = rand() % 4096 + 0x2000;
            fp->rotate = (rand() % 360) * 0x1000;
            fp->px = pos->vx;
            fp->py = pos->vy;
            fp->pz = pos->vz;
            fp->vx = rand() % 120 - 60;
            fp->vy = rand() % 60 - 120;
            fp->vz = rand() % 120 - 60;
            half = time / 2;
            half2 = time - half;
            if (0 < half2)
            {
                fp->time = rand() % half2 + half;
            }
            else
            {
                fp->time = half;
            }
            i = i + 1;
            *(u16 *)&fp->mode = 0x80;
            fp->hint = hint;
            fp->unk23 = 0;
            ef->proc = (void (*)())DrawBlood;
        } while (0);
    }
    goto outer;
end:
    return;
}
