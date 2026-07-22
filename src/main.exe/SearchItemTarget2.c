#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static struct Humanoid * SearchItemTarget2(struct Humanoid *owner, struct SVECTOR *rot, struct VECTOR *start, struct VECTOR *target);
 *     ITEM.C:352, 62 src lines, frame 136 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $fp       struct Humanoid * owner
 *     param $s1       struct SVECTOR * rot
 *     param $s5       struct VECTOR * start
 *     param $s7       struct VECTOR * target
 *     reg   $s1       int i
 *     reg   $s4       int dist
 *     reg   $s6       struct Humanoid * ret
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR rrot
 *     stack sp+56     struct SVECTOR v
 *     reg   $s0       struct VECTOR * gpv
 *     reg   $s0       struct Humanoid * human
 *     stack sp+64     struct VECTOR tv
 *     stack sp+80     struct VECTOR lv
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 * END PSX.SYM */

/*
 * SearchItemTarget2 (0x8003cde4, 0x2d8 bytes) — auto-aim target search for a
 * thrown/fired item (bow/gun/lightning bolt family: ProcItemGun,
 * ProcItemLightningBolt, ProcSightShot, ReqItemArrow, ReqItemHappou,
 * ReqItemUse all call this). Builds a world-space aim point in `*target`
 * (rotate a fixed muzzle-offset constant by `rot`, add `start`, then trace it
 * via FUN_80039ddc — likely a ground-following raycast) and its distance from
 * `start`, then scans every live Humanoid other than `owner` for the one
 * whose position, transformed into the aim-ray's local space (RotMatrix of
 * `-rot` applied via ApplyMatrixLV), sits within a narrow forward cone
 * (|dx|<500, |dy|<1000, dz>100) and is closer than the current best — if
 * found, overwrites `*target` with that Humanoid's own absolute position and
 * returns it (otherwise returns NULL, leaving `*target` as the traced aim
 * point).
 *
 * Matching notes:
 *  - The two VECTOR temps are reused for two different roles each: the first
 *    (`tv` here) holds the constant/rotated muzzle offset before the loop,
 *    then each candidate's own absolute position inside the loop (the value
 *    ultimately copied into `*target` on a hit); the second (`lv`) holds
 *    FUN_80039ddc's traced point before the loop, then the aim-space-relative
 *    delta used for the cone test inside the loop. Reusing the same two
 *    locals for both roles (rather than four distinct VECTORs) reproduces
 *    the asm reusing the same two stack slots.
 *  - Inside the loop, `lv.v* -= start->v*;` reads and writes lv's OWN slot
 *    (not `tv`'s), even though Ghidra's rendering shows the RHS as `local_48`
 *    (tv): `tv` and `lv` are value-equal at that point (`lv = tv;` just ran),
 *    so Ghidra's decompiler picked either name arbitrarily — the raw asm's
 *    load/store address is `lv`'s slot both times. Cookbook: "Trust the
 *    assembly over Ghidra's statement order."
 *  - `HumanGroup[i]` (array-of-pointers indexing, not a hand-rolled walking
 *    pointer) lets cc1's own loop strength reduction produce the target's
 *    parallel pointer-cursor induction variable (`ppHVar6`), same as
 *    ControlAllHumanoid's identical `HumanGroup[i]` idiom.
 *  - `while (1) { if (!(i < Humans)) break; ...; i = i + 1; }` — the top
 *    test is reached both from fall-in and from an unconditional back-jump
 *    (no duplicated entry test), the loop-rule-2 shape (leFindEnemy).
 *  - FUN_80039ddc's own Ghidra decompilation shows only 3 params, but this
 *    call site sets up 4 argument registers (a0-a3) — the trailing `u32`
 *    flag is forwarded unchanged to CGetLevel and not otherwise interpreted,
 *    an instance of the Ghidra-under-counts-trailing-args class (cookbook:
 *    "m2c and Ghidra disagree on a call's ARG COUNT").
 *  - The winning shape for the innermost guard needed a NAMED temp for the
 *    compared field PLUS a comma expression to keep it short-circuited:
 *    `if (cond && (z = lv.vz, z < dist)) { dist = z; *target = tv; ret =
 *    human; }`. A bare `if (cond && lv.vz < dist) { ...; dist = lv.vz; }`
 *    (no temp) forced a second reload of `lv.vz` from the stack for the
 *    `dist =` assignment (the compare's own register got clobbered by the
 *    slt overwriting it) — a plain unconditional `z = lv.vz;` BEFORE the
 *    `if` fixed the reload but broke `&&`'s short-circuit (the field got
 *    read even when `cond` was false, costing 2 extra instructions of its
 *    own). Only evaluating the assignment INSIDE the right-hand operand
 *    reproduces both the target's short-circuit and its register reuse.
 *    Statement order inside the hit body also matters: `dist = z;` must
 *    come BEFORE `*target = tv;`, not after (Ghidra renders it after) —
 *    the struct copy's own temps otherwise get scheduled first and the
 *    `move s5,v1`/`move s7,s1` pair comes out swapped.
 *  - `human->model->object[0]` (`*human->model->object`, item.h's proven
 *    `ModelArchiveType *model` @0x58 and its `ModelType **object` @0x68) and
 *    `human->life` (item.h's proven `s16 life` @0x08, read `lh` here since
 *    it's a plain signed comparison `0 < life`, unlike the narrowing `lhu`
 *    uses seen elsewhere) both check out against this function's own asm.
 */
extern VECTOR D_800121F0;

extern long abs(long x);

Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot, VECTOR *start, VECTOR *target)
{
    int i;
    int dist;
    Humanoid *ret;
    Humanoid *human;
    MATRIX mat;
    SVECTOR rrot;
    VECTOR tv;
    VECTOR lv;
    int cond;
    int z;

    ret = 0;
    tv = D_800121F0;
    RotateVector(&tv, rot->vx, rot->vy, rot->vz);
    tv.vx = tv.vx + start->vx;
    tv.vy = tv.vy + start->vy;
    tv.vz = tv.vz + start->vz;
    FUN_80039ddc(start, &tv, &lv, 0);
    target->vx = lv.vx;
    target->vy = lv.vy;
    target->vz = lv.vz;
    dist = GetVectorDistance(&lv, start);
    rrot.vx = -rot->vx;
    rrot.vy = -rot->vy;
    rrot.vz = -rot->vz;
    RotMatrix(&rrot, &mat);

    i = 0;
    while (1)
    {
        if (!(i < Humans))
            break;
        human = HumanGroup[i];
        tv = *GetAbsolutePosition(*human->model->object, 0, 0, 0);
        lv = tv;
        if (0 < human->life && human != owner)
        {
            lv.vx = lv.vx - start->vx;
            lv.vy = lv.vy - start->vy;
            lv.vz = lv.vz - start->vz;
            ApplyMatrixLV(&mat, &lv, &lv);
            lv.vz = -lv.vz;
            if (100 < lv.vz)
            {
                cond = 0;
                if (abs(lv.vx) < 500)
                {
                    cond = abs(lv.vy) < 1000;
                }
                if (cond && (z = lv.vz, z < dist))
                {
                    dist = z;
                    *target = tv;
                    ret = human;
                }
            }
        }
        i = i + 1;
    }
    return ret;
}
