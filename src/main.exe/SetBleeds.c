#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBleeds(struct VECTOR *pos, short grange, short srange, short n, int time, long col);
 *     EFFECT.C:963, 11 src lines, frame 88 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param stack+0   struct VECTOR * pos
 *     param $a1       short grange
 *     param $s5       short srange
 *     param $s6       short n
 *     param stack+16  int time
 *     param stack+20  long col
 *     reg   $fp       int time
 *     reg   $s7       long col
 *     stack sp+16     struct VECTOR npos
 *     stack sp+32     struct SVECTOR v
 *     reg   $a3       struct VECTOR * pos
 *     reg   $t0       int time
 *     reg   $s7       long col
 *     reg   $v1       struct BleedType * param
 *     reg   $a2       struct tag_EffectSlot * slot
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

/*
 * Matching notes (MATCHED — byte-identical; all verified against the raw .s,
 * the demo build's disassembly, and cc1 -da RTL dumps):
 *  - Outer loop is `do { if (n <= 0) return; ...; } while (1);` (the guard
 *    INSIDE, unconditional back-jump) — same rule as SetSmoke's outer loop.
 *  - STACK LAYOUT is the whole game (frame 0x58): npos@sp+0x10, v@sp+0x20,
 *    scratch SVECTOR t@sp+0x28 — and the 16-byte work vector at
 *    sp+0x20..0x2f is first copied out to npos. Its upper short-vector half
 *    then builds the velocity and is copied to the lower half (the second,
 *    lwl/lwr unaligned 8-byte copy).
 *    This overlap reproduces the "compute
 *    into throwaway stack scratch + second block-copy" residual both parked
 *    drafts fought: the values are STORED to stack per-arm and never need
 *    callee-saved registers, freeing $fp for `time`. The demo build has the
 *    IDENTICAL layout and PSX.SYM lists ONLY npos@sp+16 and v@sp+32 (no
 *    third var, arrays would print bounds), so the original really did use
 *    v's memory as VECTOR scratch. Frame math confirms: 16(args) + 16(npos)
 *    + 8(v) + 8(t) + 40(s0-s7,fp,ra) = 0x58.
 *  - Each jitter is an if/else with the STORE IN EACH ARM (not
 *    default-then-overwrite, which puts one store before the branch): the
 *    grange arms share `b = pos->vx;` loaded BEFORE the branch (b lives in
 *    $s0 across rand; pos itself stays in its HOME arg slot and is reloaded
 *    per field), then-arm `b + (rand() % grange2 - g)`, else-arm `b - g`.
 *    Retail ADDED the else-arm values: the demo stores b / 0 there (real
 *    source change between demo and retail).
 *  - `int g = grange;` makes ONE sign-extension pseudo shared by grange2
 *    AND the six arm uses (target: sll a1,16; sra s4,a1,16; sll s3,s4,1).
 *    Writing `grange` in the arms instead leaves 6 in-loop extension pairs
 *    that loop.c hoists and combines, leaving a stray `move s4,a1`.
 *    `srange` is the opposite: used RAW everywhere (halfword stores), and
 *    `srange * 2` direct gets the sll16/sra15 widen-and-scale fuse.
 *  - THE -srange ARMS ARE THE WHOLE BATTLE (see cookbook "loop.c hoisting
 *    is a threshold economy"): three identical `-srange` else arms expand
 *    to invariant `neg` movables that cse UNIFIES into one pseudo;
 *    combine_movables sums their savings (3*3*threshold >= 147 insns) and
 *    hoists one negu into the prologue, stealing the callee-saved reg that
 *    `time` needs. ONE such neg is fine (29*1*3 < 147 stays). So: arm1 is
 *    plain `-srange`; arms 2/3 use `z2 - srange` / `z3 - srange` where
 *    z2/z3 are SINGLE-SET SINGLE-USE `int` zeros initialized ABOVE the
 *    loop. Their minus insns are solo movables (no rtx match, stay), and
 *    local-alloc's update_equiv_regs / reload substitute the constant 0
 *    straight into subsi3's reg_or_0_operand ("dJ") — emitting the literal
 *    per-arm `negu v0,s5` with NO trace of z2/z3 in the binary.
 *  - `half = time/2; rem = time - half; if (rem > 0) btime = rand()%rem +
 *    half; else btime = half;` — if/else, not default-then-overwrite.
 *    `time` (stack param, never written) is RTX_UNCHANGING and rides in
 *    $fp for the whole function once the register exists to hold it.
 *  - The pool scan + store tail live in an INNER BLOCK shadowing
 *    `VECTOR *pos = &npos;` and `int time = btime;` (PSX.SYM lists these
 *    shadow scopes) — the tail copies `slot->param.bleed.pos = *pos;`
 *    REGISTER-INDIRECT through $a3 (writing `npos` directly would compile
 *    sp-relative), and `int time` is why btime joins in $v0 then copies to
 *    $t0. The scan itself is SetBleed.c's indexed do-while shape verbatim;
 *    a single `n = n - 1;` sits right after `found:` (reorg
 *    duplicates it into the wrap path's delay slot).
 *  - Store order into BleedType: pos, vec, r, g, time, b, mode, proc —
 *    same as SetBleed.c. `rand() % <variable>` divisions need --expand-div
 *    (Build.hs + permute.py) and the pool cursor is a gp-extern.
 */
extern void DrawBleed(TEffectSlot *ef);
extern void *memset(void *s, int c, u32 n);

void SetBleeds(VECTOR *pos, short grange, short srange, short n, int time, long col)
{
    VECTOR npos;
    VECTOR work;
    int grange2;
    long b;
    int g;
    int z2, z3;
    int half;
    int rem;
    int btime;

    g = grange;
    grange2 = g * 2;
    z2 = 0;
    z3 = 0;
    do
    {
        if (n <= 0)
        {
            return;
        }
        memset(&work, 0, sizeof(VECTOR));
        b = pos->vx;
        if (grange2 > 0)
        {
            work.vx = b + (rand() % grange2 - g);
        }
        else
        {
            work.vx = b - g;
        }
        b = pos->vy;
        if (grange2 > 0)
        {
            work.vy = b + (rand() % grange2 - g);
        }
        else
        {
            work.vy = b - g;
        }
        b = pos->vz;
        if (grange2 > 0)
        {
            work.vz = b + (rand() % grange2 - g);
        }
        else
        {
            work.vz = b - g;
        }
        npos = work;
        memset(&((SVECTOR *)&work)[1], 0, sizeof(SVECTOR));
        if (srange * 2 > 0)
        {
            ((SVECTOR *)&work)[1].vx = rand() % (srange * 2) - srange;
        }
        else
        {
            ((SVECTOR *)&work)[1].vx = -srange;
        }
        if (srange * 2 > 0)
        {
            ((SVECTOR *)&work)[1].vy = rand() % (srange * 2) - srange;
        }
        else
        {
            ((SVECTOR *)&work)[1].vy = z2 - srange;
        }
        if (srange * 2 > 0)
        {
            ((SVECTOR *)&work)[1].vz = rand() % (srange * 2) - srange;
        }
        else
        {
            ((SVECTOR *)&work)[1].vz = z3 - srange;
        }
        *(SVECTOR *)&work = ((SVECTOR *)&work)[1];
        half = time / 2;
        rem = time - half;
        if (rem > 0)
        {
            btime = rand() % rem + half;
        }
        else
        {
            btime = half;
        }
        {
            VECTOR *pos = &npos;
            int time = btime;
            int idx;
            TEffectSlot *slot;
            int count;
            BleedType *param;
            u8 r;

            FIND_EFFECT_SLOT(idx, count, slot, found);
        found:
            n--;
            param = &slot->param.bleed;
            r = col >> 16;
            slot->param.bleed.pos = *pos;
            slot->param.bleed.vec = *(SVECTOR *)&work;
            param->r = r;
            param->g = col >> 8;
            param->time = time;
            param->b = col;
            param->mode = 0;
            slot->proc = DrawBleed;
        }
    } while (1);
}
