#include "common.h"
#include "main.exe.h"
#include "misc.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcMiscFire(struct tag_TMisc *m, enum TMiscMessage msg);
 *     MISC.C:249, 41 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s2       struct tag_TMisc * m
 *     param $a1       enum TMiscMessage msg
 *     stack sp+16     struct SVECTOR vec
 *     stack sp+24     struct VECTOR pos
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 10 of 356 bytes differ. Residual is the named
 * permuter-immune "la reload tie" (cookbook, Iteration protocol): the
 * address of the unnamed SVECTOR constant `D_80097C48` builds as
 * `lui v0,%hi / addiu t3,v0,%lo` (two registers) in the target, with an
 * unrelated independent instruction (`addiu s1,sp,24`, materializing
 * `&vec`) scheduled between the two halves; our build instead puts both
 * halves in ONE register (`lui t3,%hi / addiu t3,t3,%lo`) with the
 * independent instruction moved after. Tried and reverted: swapping the
 * `vec`/`pos` local declaration order (worse, 40 bytes) and swapping the
 * `vec = D_80097C48;` / `pos.v{x,y,z} = m->{x,y,z};` statement order
 * (worse, 22 bytes — reorders the visible store sequence too). Per the
 * cookbook, this exact tie class (PrepareAccess, cd_open) has never been
 * cracked by the permuter or by source-shaping; parked rather than
 * spending a bounded run on a known-flat class.
 *
 * ProcMiscFire (0x8004d570, 0x164 bytes) — MISC_FIRE's ProcMisc* handler:
 * MM_CREATE arms a 10-tick fuse; every later message decrements it (once
 * per call, gated on `mode == 0`) and, when it reaches 0, detonates: an
 * explosion + a burning-embers burst + a smoke puff at m's position, then
 * rearms the fuse to a random 0..149 tick count and plays the bang sound.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `msg > MM_RESUME` is the same unsigned-enum dispatch as ProcMiscSprite
 *    (`enum TMiscMessage msg`, not plain `s32`, is what gets cc1 to emit
 *    `sltiu` instead of `slti`).
 *  - `--m->count` (or equivalently `m->count = m->count - 1;`) stores
 *    unconditionally in the branch's delay slot even on the "still armed"
 *    path — ordinary statement order, no special shape needed.
 *  - `vec = D_80097C48;` is a whole-SVECTOR struct assignment (align-2
 *    `lwl/lwr`+`swl/swr` block move, cookbook: cast type's alignment drives
 *    copy code) from an unnamed 8-byte rodata/data constant — NOT the
 *    Ghidra-rendered `local_28._4_4_ = (uint)(ushort)local_28.pad << 0x10;`
 *    later in the function, which is a decompiler artifact: the raw .s has
 *    three plain adjacent `sh` stores (vx=0, vy=-200, vz=0), no pad touched
 *    at all (cookbook: trust the assembly over Ghidra's rendering).
 *  - `m->count = rand() % 150;` keeps the call inline so the magic-multiply
 *    divide operates directly on $v0.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcMiscFire", ProcMiscFire);
#else
/* Draft — turn this into matching C, then delete the #ifndef/#else/#endif
   guards. Reference: */

extern SVECTOR D_80097C48;
extern void SetExplosion(VECTOR *pos, SVECTOR *vel);
extern void SetHinoko(VECTOR *pos, SVECTOR *vel, s32 n);
extern void SetSmoke(VECTOR *pos, SVECTOR *vel, s32 n, s32 time);
extern s16 SoundEx(VECTOR *loc, s32 id);

void ProcMiscFire(tag_TMisc *m, enum TMiscMessage msg)
{
    SVECTOR vec;
    VECTOR pos;

    if (msg == MM_CREATE)
        goto do_create;
    if (MM_RESUME < msg)
        goto do_check;
    return;

do_create:
    m->mode = 0;
    m->count = 10;
    return;

do_check:
    if (m->mode != 0)
        return;
    m->count = m->count - 1;
    if (m->count < 1)
    {
        vec = D_80097C48;
        pos.vx = m->x;
        pos.vy = m->y;
        pos.vz = m->z;
        SetExplosion(&pos, &vec);
        vec.vx = 0x4B;
        vec.vy = 0xB4;
        vec.vz = 0x4B;
        SetHinoko(&pos, &vec, 10);
        vec.vx = 0;
        vec.vy = -200;
        vec.vz = 0;
        SetSmoke(&pos, &vec, 0x14, 6);
        m->count = rand() % 150;
        SoundEx(&pos, 0x28);
    }
}
#endif /* NON_MATCHING */
