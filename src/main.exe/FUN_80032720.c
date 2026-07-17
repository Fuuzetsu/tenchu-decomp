#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 *
 * PSX.SYM suggests this may be `SetPadState` (LOW confidence, EFFECT.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — exact target extent (560 bytes / 140 instructions),
 * 25 differing bytes, 6 clusters / 16 differing insns (all operands-kind;
 * `matchdiff --clusters` reports NO branch-retarget here).
 *
 * READ THIS BEFORE RESUMING: the `volatile u32 scrollYShifted` below is a
 * DEAD END, not a near-miss. It cannot reach 0, for a structural reason that
 * is proven by measurement — and the previous note's claim that the target's
 * scrollY stack round-trip is "otherwise-unreachable" is FALSE.
 *
 * Why the volatile can never work. The target's frame holds three values above
 * the 0x10 outgoing-arg boundary: y@sp+0x10, z@sp+0x12, (scrollY<<16)@sp+0x14.
 * cc1 assigns frame slots in three phases, in this order: assign_parms
 * (ADDRESSABLE params) -> expand_decl (DECLARED locals, in declaration order)
 * -> reload's alter_reg (SPILLED pseudos, in pseudo-number order). A declared
 * local is therefore ALWAYS given a lower slot than any reload spill. y/z are
 * reload spills here (params get the lowest pseudo numbers, hence 0x10/0x12),
 * so any DECLARED local — volatile or not — necessarily squats on 0x10 and
 * pushes y/z to 0x14/0x16. That IS the 25-byte residual: swapped offsets, plus
 * the volatile's pinned `lw` landing 2 insns early and its register cascade.
 *
 * Confirmed in both directions. Making y/z `volatile short` params moves them
 * into assign_parms and the offsets snap to the target's exactly (sh a1,16(sp)
 * / sh a2,18(sp) / lw ...,20(sp)) — but volatile also pins those accesses
 * against sched, shattering the prologue and tail schedules: 107 bytes, 8
 * clusters (this reproduces the "107+" the earlier note reported). So the
 * target's y/z are NOT addressable, and (scrollY<<16) MUST be an ordinary
 * pseudo that reload spills.
 *
 * A NATURAL spill IS reachable. This shape builds at the exact 560-byte extent
 * with the natural reload spill at the CORRECT slot (sw a0,20(sp) / lw
 * a3,20(sp)) and all 140 mnemonics matching — matchdiff reports 7 clusters,
 * every one `operands`-only, i.e. a pure register permutation (56 bytes: worse
 * than 25 by the byte metric, but the only shape that can reach 0):
 *
 *     u32 scrollYShifted; s32 signedScrollY; int sx;
 *     ...
 *     sx = scrollX;
 *     scrollYShifted = (u32)(u16)scrollY << 16;
 *     j = 0;
 *   grid:
 *     for (i = 0; i < 2; i++)
 *         if ((0xF >> (j * 2 + i)) & 1) {
 *             signedScrollY = (s32)scrollYShifted >> 16;
 *             MoveImage((RECT *)&pp->bleed.vec, sx + im->pw * i,
 *                       signedScrollY + im->ph * j);
 *         }
 *     j++;
 *     if (j < 2) goto grid;
 *
 * Why each piece is load-bearing (from cc1's own movable log —
 * `tools/rtldump.py FUN_80032720 --pass loop --loop-log`):
 *  - The target computes `(short)scrollY` INSIDE the outer loop from a spilled
 *    `scrollY<<16` (sll before the loop @0x80032828, sra in-loop @0x80032848):
 *    the sign-extension is SPLIT BY THE SPILL, so the value live across the
 *    nest is the SHIFTED one. That extra live value is what forces the spill —
 *    9 s-regs already hold im/ef/pp/j/i/(short)scrollX/(short)j/j*2/
 *    (short)scrollY, and (scrollY<<16) is the 10th.
 *  - With an ordinary `for (j...)` outer loop, loop.c hoists BOTH halves out,
 *    only ONE value stays live, it fits in 9 s-regs, no spill happens, and the
 *    draft is 552 bytes (2 insns SHORT). The hoist gate is not close and cannot
 *    be tuned: threshold*savings*lifetime >= insn_count is 29*1*38 >> 37.
 *  - Hand-rolling the outer loop (`grid:`/`goto grid;`) emits no
 *    NOTE_INSN_LOOP_BEG, so loop.c does no invariant motion on it and the `sra`
 *    stays in — the same idiom this function already uses for its pool search.
 *  - The sra must sit INSIDE the inner `if`, not at the top of the outer body:
 *    loop.c(inner) then hoists it into the outer body while `i = 0` stays the
 *    body's first insn, which reorg duplicates into the outer back-edge delay
 *    slot (`move s0,zero` @0x80032834 + @0x800328d0). Written at the top of the
 *    outer body instead, the loop top becomes the `lw`, reorg fills the delay
 *    slot from the fall-through, and the draft is 556 bytes (1 insn SHORT).
 *
 * THE ONE OPEN QUESTION is a global s-register permutation (ours im/ef/pp/j/i/
 * sx/j*2/sy = s5/s4/s0/s3/s1/s8/s6/s7; target = s7/s8/s1/s5/s0/s6/s4/s3).
 * `tools/regalloc.py --order` shows cc1 allocating by
 * priority = floor_log2(refs)*refs/live, taking the lowest free reg each time.
 * The explicit `int sx` is the suspect: the target's (short)scrollX pseudo is
 * CREATED LATE by loop.c hoisting it out of a real `for`, whereas `sx` is an
 * early-numbered source local, so every priority slot shifts. Squaring that
 * circle — scrollX's sra hoisted out of the outer loop but scrollY's not, from
 * ONE loop form — is the remaining work. autorules: no improving edit among 52
 * candidates. The permuter could not be run to completion: it overruns the 600s
 * harness cap when piped, as its -j4 workers inherit the pipe and outlive the
 * inner `timeout`'s SIGTERM.
 *
 * FUN_80032720 (0x80032720, 0x230 bytes) — spawns an EffectSlot pool entry
 * that draws a small animated 2x2-cell water/warp tile grid from a texture
 * page (AddMisc.c: `extern void FUN_80032720(GsIMAGE *im, short y, short
 * z);`, called with the just-uploaded TIM's own GsIMAGE). Same round-robin
 * EffectSlot[200] pool search as SetSplash/SetFrame/SetBleed/SetSmoke (see
 * SetSplash.c for the shared idiom writeup — goto loop instead of
 * while(1)+break so loop.c doesn't hoist `&dmy`'s address, idx-computed-
 * before-slot, cursor-update store living inside `if (slot->proc==0)
 * {...break;}` for the right branch polarity).
 *
 * The found slot's `param` union is written through FIVE different
 * overlapping views (proven by `tools/access.py --order`, not Ghidra's own
 * struct-name guesses):
 *  - smoke.vec.vy/vx (offsets 2/0) zeroed;
 *  - splash.sx/sy (offsets 0xc/0xe) get the pool's own scroll cursor
 *    (`D_80097F30`/`D_80097F32`, a shared animated texture-scroll offset
 *    also advanced at the very end);
 *  - offsets 8/0xa get `im->px`/`im->py` (the GsIMAGE's own texture-page
 *    position) through a RAW OFFSET CAST — this does NOT line up with any
 *    proven named field at that width (BleedType.pos.vz is a 4-byte field
 *    there, not two shorts): a genuinely divergent union member, per the
 *    cookbook's "reach it via an explicit offset cast off the SAME proven
 *    pointer, don't invent a new named struct" rule;
 *  - bleed.vec.vx/vy/vz/pad (offsets 0x10/0x12/0x14/0x16) get the SAME
 *    im->px/im->py plus im->pw/im->ph (the tile's pixel size) — this is
 *    the actual source RECT the 2x2 grid's MoveImage calls read from;
 *  - smoke.vec.vz/pad (offsets 4/6) get this call's own `y`/`z` params,
 *    written only at the very end (right before `ef->proc` is set),
 *    matching the target's own late spill-reload from their stack homes.
 *
 * Matching notes:
 *  - The 2x2 grid loop uses `short` counters (`j`,`i`), not `int` — a
 *    `short` loop counter suppresses loop.c's strength reduction and keeps
 *    the target's own `(x<<0x10)>>0x10`-style recompute-from-base shape
 *    (cookbook: "a short loop counter suppresses strength reduction").
 *  - The bit test `(0xF >> (j*2+i)) & 1` is always true for j,i in {0,1}
 *    (indices 0-3, all set in 0xF), but the target still computes it. Ghidra
 *    renders an extra `& 0x1F` because MIPS variable shifts mask their count
 *    in hardware; retaining that decompiler artifact emits a real `andi`
 *    which is absent from the target.
 *  - `D_80097F30`/`D_80097F32` are read ONCE into named locals right
 *    after the slot is found (not hoisted to the top the way Ghidra's own
 *    SSA rendering shows `sVar2 = DAT_80097f32; sVar3 = DAT_80097f30;` as
 *    the first two statements) — the raw .s doesn't read them until deep
 *    into the found-body, immediately before the splash.sx/sy stores
 *    (cookbook: "trust the assembly over Ghidra's statement order").
 */

typedef struct RECT RECT;
struct RECT
{
    short x, y, w, h;
};

extern void MoveImage(RECT *rect, int x, int y);
extern void UpdateTexScroll(void);
extern s16 D_80097F30;
extern s16 D_80097F32;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80032720", FUN_80032720);
#else

void FUN_80032720(GsIMAGE *im, short y, short z)
{
    int idx;
    TEffectSlot *base;
    TEffectSlot *slot;
    int count;
    TEffectSlot *ef;
    union EffectParam *pp;
    s16 scrollX;
    short scrollY;
    short px;
    short py;
    short j;
    short i;

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
{
    volatile u32 scrollYShifted;

    pp = &ef->param;
    pp->smoke.vec.vy = 0;
    ef->param.smoke.vec.vx = 0;

    scrollX = D_80097F30;
    scrollY = D_80097F32;
    pp->splash.sx = scrollX;
    pp->splash.sy = scrollY;

    px = im->px;
    *(short *)((u8 *)pp + 8) = px;
    pp->bleed.vec.vx = px;
    py = im->py;
    *(short *)((u8 *)pp + 0xA) = py;
    pp->bleed.vec.vy = py;
    pp->bleed.vec.vz = im->pw;
    pp->bleed.vec.pad = im->ph;
    scrollYShifted = (u32)(u16)scrollY << 16;

    for (j = 0; j < 2; j++)
    {
        s32 signedScrollY = (s32)scrollYShifted >> 16;

        for (i = 0; i < 2; i++)
        {
            if ((0xF >> (j * 2 + i)) & 1)
            {
                MoveImage((RECT *)&pp->bleed.vec, scrollX + im->pw * i,
                          signedScrollY + im->ph * j);
            }
        }
    }

    D_80097F32 = D_80097F32 + 0x40;
    pp->smoke.vec.vz = y;
    pp->smoke.vec.pad = z;
    ef->proc = (void (*)())UpdateTexScroll;
    if (0x200 < D_80097F32)
    {
        D_80097F32 = 0x100;
        D_80097F30 = D_80097F30 + 0x40;
    }
}
}
#endif
