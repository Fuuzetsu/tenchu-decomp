#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetExplosion(struct VECTOR *pos, struct SVECTOR *vect);
 *     EFFECT.C:1236, 18 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s2       struct VECTOR * pos
 *     param $s3       struct SVECTOR * vect
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct ExplosionType * param
 *     reg   $v1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * Matching notes (all verified against the original bytes):
 *  - Same EffectSlot[200] pool search as SetImpact (see its header): a
 *    real `do { ... } while (count < 200);`, not a hand-rolled goto — the
 *    give-up path's `ef = &dmy;` sits AFTER the loop, not inside it, so
 *    loop.c doesn't get a chance to hoist that address.
 *  - UNLIKE SetImpact, `count = count + 1;` here comes BEFORE the
 *    `if (slot->proc == 0)` test, not after (both Ghidra's own rendering
 *    and the raw asm's delay-slot fill agree: the branch testing
 *    `slot->proc` has `count++` in its delay slot, executed regardless of
 *    outcome — only possible if count++ is the statement immediately
 *    preceding the if in source). Each EffectSlot-pool inserter in this TU
 *    apparently wrote this test/increment order slightly differently;
 *    don't assume one sibling's shape for another without checking.
 *  - `ef->param` is `ExplosionType` (see DrawExplosion.c/DrawHinoko.c):
 *    Ghidra's `blood.py/pz/scale` and `smoke.*` names are its own wrong
 *    union guess for the same proven offsets (pos@0x8, vec@0x0, time@0x20,
 *    mode@0x21).
 *  - `param->scale = 0x1000;` is a plain independent constant store that
 *    floats into the `jal rand`'s delay slot (unrelated to the call);
 *    write it as the first statement, before `rand()`, matching Ghidra.
 *  - `vect->vz` is captured into a temp and stored to `param->vec.vz`
 *    LAST (after time/mode), the same delayed-store idiom as
 *    SetFrame/SetSplash/SetBleed's `pos->vz` capture.
 *  - `vect->vx/vy/vz` read via `lhu` despite SVECTOR's fields being
 *    signed `short`: a pure narrowing copy (read then immediately written
 *    back at the same 16-bit width, no arithmetic) doesn't need the sign
 *    bits, so cc1 picks the cheaper unsigned load.
 *  - `SetBleeds` takes SIX arguments (`VECTOR *pos, short grange, short
 *    srange, short n, int time, long col` — reference/psxsym-protos.h);
 *    Ghidra's call rendering drops the two stack-passed args past the
 *    4 register ones (cookbook: Ghidra under-counts stack args). The raw
 *    `.s` shows all six: pos, 200, 0x96, 0x14, 0x1e, 0xFFFF00.
 */
extern void DrawExplosion(TEffectSlot *ef);
extern void SetBleeds(VECTOR *pos, short grange, short srange, short n, int time, long col);

void SetExplosion(VECTOR *pos, SVECTOR *vect)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    ExplosionType *param;
    int r;
    short vz;

    count = 0;
    base = EffectSlot;
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
    param = &ef->param.explosion;
    param->scale = 0x1000;
    r = rand();
    param->rotate = (r % 360) * 0x1000;
    param->pos.vx = pos->vx;
    param->pos.vy = pos->vy;
    param->pos.vz = pos->vz;
    param->vec.vx = vect->vx;
    param->vec.vy = vect->vy;
    vz = vect->vz;
    param->time = 5;
    param->mode = 0;
    param->vec.vz = vz;
    ef->proc = (void (*)())DrawExplosion;
    SetBleeds(pos, 200, 0x96, 0x14, 0x1e, 0xFFFF00);
}

// triage: MEDIUM — 96 insns, mul/div, 1 loop, 2 callees, ~0.07 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetExplosion(VECTOR *pos,SVECTOR *vect)
//
// {
//   short sVar1;
//   int iVar2;
//   int iVar3;
//   tag_EffectSlot *ptVar4;
//
//   iVar3 = 0;
//   ptVar4 = EffectSlot + DAT_80097a3c;
//   iVar2 = DAT_80097a3c;
//   while( true ) {
//     iVar2 = iVar2 + 1;
//     ptVar4 = ptVar4 + 1;
//     if (199 < iVar2) {
//       iVar2 = 0;
//       ptVar4 = EffectSlot;
//     }
//     iVar3 = iVar3 + 1;
//     if (ptVar4->proc == (undefined **)0x0) break;
//     if (199 < iVar3) {
//       ptVar4 = &dmy;
// LAB_800367d8:
//       (ptVar4->param).smoke.scale = 0x1000;
//       iVar2 = rand();
//       (ptVar4->param).smoke.rotate = (iVar2 % 0x168) * 0x1000;
//       (ptVar4->param).blood.py = pos->vx;
//       (ptVar4->param).blood.pz = pos->vy;
//       (ptVar4->param).blood.scale = pos->vz;
//       (ptVar4->param).smoke.vec.vx = vect->vx;
//       (ptVar4->param).smoke.vec.vy = vect->vy;
//       sVar1 = vect->vz;
//       (ptVar4->param).blood.mode = '\x05';
//       (ptVar4->param).blood.bright = '\0';
//       (ptVar4->param).smoke.vec.vz = sVar1;
//       ptVar4->proc = (undefined **)DrawExplosion;
//       SetBleeds((short)pos,200,0x96);
//       return;
//     }
//   }
//   DAT_80097a3c = iVar2 + 1;
//   if (199 < DAT_80097a3c) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_800367d8;
// }
