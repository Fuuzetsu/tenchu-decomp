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
 *    scratch SVECTOR t@sp+0x28 — and the position jitter is built as a
 *    VECTOR THROUGH `(VECTOR *)&v` (spanning v AND t's slots, 0x20..0x2F,
 *    memset(&v,0,sizeof(VECTOR)) included), then copied out with
 *    `npos = *(VECTOR *)&v;` (the first 16-byte block copy). The velocity
 *    jitter is then built in `t` and copied with `v = t;` (the second,
 *    lwl/lwr unaligned 8-byte copy). That aliasing reproduces the "compute
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
 *    shadow scopes) — the tail copies `ef->param.bleed.pos = *pos;`
 *    REGISTER-INDIRECT through $a3 (writing `npos` directly would compile
 *    sp-relative), and `int time` is why btime joins in $v0 then copies to
 *    $t0. The scan itself is SetBleed.c's hand-rolled `goto loop;` shape
 *    verbatim; a single `n = n - 1;` sits right after `found:` (reorg
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
    SVECTOR v;
    SVECTOR t;
    int grange2;
    int srange2;
    long b;
    int g;
    int z2, z3;
    int half;
    int rem;
    int btime;

    g = grange;
    grange2 = g * 2;
    srange2 = srange * 2;
    z2 = 0;
    z3 = 0;
    do
    {
        if (n <= 0)
        {
            return;
        }
        memset(&v, 0, sizeof(VECTOR));
        b = pos->vx;
        if (grange2 > 0)
        {
            ((VECTOR *)&v)->vx = b + (rand() % grange2 - g);
        }
        else
        {
            ((VECTOR *)&v)->vx = b - g;
        }
        b = pos->vy;
        if (grange2 > 0)
        {
            ((VECTOR *)&v)->vy = b + (rand() % grange2 - g);
        }
        else
        {
            ((VECTOR *)&v)->vy = b - g;
        }
        b = pos->vz;
        if (grange2 > 0)
        {
            ((VECTOR *)&v)->vz = b + (rand() % grange2 - g);
        }
        else
        {
            ((VECTOR *)&v)->vz = b - g;
        }
        npos = *(VECTOR *)&v;
        memset(&t, 0, sizeof(SVECTOR));
        if (srange2 > 0)
        {
            t.vx = rand() % srange2 - srange;
        }
        else
        {
            t.vx = -srange;
        }
        if (srange2 > 0)
        {
            t.vy = rand() % srange2 - srange;
        }
        else
        {
            t.vy = z2 - srange;
        }
        if (srange2 > 0)
        {
            t.vz = rand() % srange2 - srange;
        }
        else
        {
            t.vz = z3 - srange;
        }
        v = t;
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
            TEffectSlot *base;
            TEffectSlot *slot;
            int count;
            TEffectSlot *ef;
            BleedType *param;
            u8 r;

            idx = CURRENT_OFFSET_INTO_SOME_SELF_CALL_STRUCT_AREA_;
            count = 0;
            base = EffectSlot;
            slot = base + idx;
        loop:
            idx = idx + 1;
            slot = slot + 1;
            if (199 < idx)
            {
                slot = base;
                idx = 0;
            }
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
            count = count + 1;
            if (199 < count)
            {
                ef = &dmy;
                goto found;
            }
            goto loop;
        found:
            n = n - 1;
            param = &ef->param.bleed;
            r = col >> 16;
            ef->param.bleed.pos = *pos;
            ef->param.bleed.vec = v;
            param->r = r;
            param->g = col >> 8;
            param->time = time;
            param->b = col;
            param->mode = 0;
            ef->proc = (void (*)())DrawBleed;
        }
    } while (1);
}

// triage: MEDIUM — 277 insns, mul/div, 1 loop, 2 callees, ~0.08 to InsertConflict
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
