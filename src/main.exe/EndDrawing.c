#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void EndDrawing(short sync);
 *     3DCTRL.C:151, 48 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       short sync
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern short DrawingPage;
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT *OTablePt;
 *     extern struct GsFOGPARAM Fog;
 * END PSX.SYM */

/*
 * EndDrawing (0x80017030, 0x218 bytes) — per-frame draw finalize: called
 * with `sync`, the caller's requested VSync wait mode. Retail's actual
 * locals outnumber the demo's PSX.SYM record (which shows only the
 * parameter) — the dispatch below needs its own captured copy of
 * SkipFrame, so treat the demo's "1 local" as an upper-bound artifact of a
 * differently-compiled demo build, not a spec (see the PSX.SYM caveat
 * above; `time`/D_800976B8 confirmed the same TU-local %gp_rel treatment as
 * GameClock/SkipFrame/DrawingPage/OTablePt via the raw `.s`).
 *
 * Body, in order:
 *  1. Every 30th frame (`GameClock == GameClock/30*30`, NOT `%` — the raw
 *     asm computes the quotient then re-multiplies and compares, matching
 *     Ghidra's literal rendering, not a modulo idiom), while not already
 *     mid-skip (`SkipFrame == 0`), snapshot how much of the current GPU
 *     packet buffer is left: `GsGetWorkBase() - D_80098040` (the buffer's
 *     base, same absolute symbol as StartDrawing.c's page stride constant)
 *     minus the current page's byte offset (`DrawingPage << 16`), clamped
 *     to 0x10000. Two independent `sw`s (one per branch) store the clamped
 *     and unclamped values — plain if/else, no eager-store-then-override
 *     idiom needed (each arm's stored VALUE differs, so cc1 has nothing to
 *     cross-jump-merge).
 *  2. An explicit `if (cond) goto L;` ladder over `sk` (captured from
 *     SkipFrame before any case body can clear the global) for exactly
 *     {0,1,2}, no default: `if (sk==1) goto case1; if (sk<2) { if (sk==0)
 *     goto case0; goto join; } if (sk==2) goto case2; goto join;` — tests
 *     fire 1, <2, ==0 (nested), ==2 while the case BODIES sit in memory in
 *     plain 0,1,2 source order (this is what m2c's own reconstruction
 *     rendered as a `switch`, and a genuine `switch` statement over the
 *     same 3 values compiles to byte-IDENTICAL code here — verified by
 *     swapping the two forms with no diff — so either spelling works; the
 *     ladder is kept for its self-documenting test/body-order split).
 *     Two values with NO matching case (sk<0 or sk>2) fall straight to the
 *     join with no code, exactly the no-default semantics.
 *     - case 0: if the frame is overrunning its budget
 *       (`VSync(1) > ((sync - (sync<<4))<<4) - 0xa`), start skipping
 *       (`SkipFrame=1`) and return immediately — this return, not a
 *       fallthrough, is what skips the shared tail below. The condition is
 *       written call-first (`VSync(1) > ...`) so cc1 evaluates the call
 *       before the multiply (matching the target's instruction order), and
 *       the multiply-by-(-0xf0) is spelled as the explicit strength
 *       reduction `(x - (x<<4)) << 4` rather than `x * -0xf0`: the literal
 *       multiply gets const-multiply-synthesized as "compute +240*x, then
 *       negate" (an extra `subu $0,...`/negate instruction), while the
 *       explicit form directly computes `x - 16x = -15x` (sign built into
 *       the subtraction order, no separate negate) — same value, one
 *       instruction shorter, matching the target exactly.
 *     - case 1: `sync` itself gets overwritten — `(u16)sync << 1` widened
 *       through the classic double-shift (`sll 16`/`srl 15`, net shift +1,
 *       the unsigned analogue of the sign-extension idiom) — an explicit
 *       `(unsigned short)` cast is required: a bare `sync << 1` promotes
 *       signed short via `sra` (arithmetic), but the target uses `srl`
 *       (logical), so the cast to unsigned is load-bearing, not
 *       decorative. The intermediate MUST be a `u32` local (`t`), not
 *       `u16`: with a `u16` intermediate cc1's combine/peephole collapses
 *       the double-shift into a single `sll $s0,$s0,1` (still numerically
 *       equal, since only the low 16 bits of the result ever matter, but
 *       one instruction SHORTER than the target). Then
 *       `dp = sk - (u16)DrawingPage; DrawingPage = dp;` reuses the SAME
 *       captured dispatch value `sk` (still holding the pre-dispatch
 *       value, here 1) instead of the literal constant 1 — confirmed by
 *       the raw asm reusing `$a0` (sk's register) rather than the
 *       separately-live `$s1` (which holds a genuine materialized
 *       constant 1 for the dispatch's own equality test). The extra `s32
 *       dp` intermediate (rather than assigning `DrawingPage` directly)
 *       is THE lever that keeps `sk`'s own register alive here: cc1's cse
 *       (record_jump_equiv in cse.c) recognizes "sk == 1" from the
 *       dominating `if (sk==1) goto case1;` test and, when `sk` is `s32`
 *       (matching the SImode the comparison itself is done in), directly
 *       SUBSTITUTES the constant 1 for `sk` at this later use (`li
 *       $v0,1`) instead of reusing the live register — one instruction
 *       longer than the target, which reuses the register with none of
 *       cse's help. Declaring `sk` as `s16` (a narrower HImode pseudo that
 *       doesn't match the SImode compare's own recorded equivalence) and
 *       routing the result through a same-width `s32 dp` before the final
 *       narrowing store to `DrawingPage` (rather than writing
 *       `DrawingPage = sk - (u16)DrawingPage;` directly, which reads as a
 *       *narrowing* use and lets cc1 satisfy it with a fresh, wrongly-
 *       unsigned `lhu` reload instead) is what reproduces the target's
 *       zero-cost register reuse — both other spellings are one
 *       instruction off, in opposite directions. `OTablePt =
 *       &OTable[DrawingPage]` reuses the just-stored value with no reload
 *       (same idiom as StartDrawing.c).
 *     - case 2: just clears SkipFrame.
 *  3. Shared tail (reached by every case, or directly if SkipFrame was
 *     none of {0,1,2}): swap the sort table's wrap-around bucket
 *     (`OTablePt->org[0x7fe] = OTablePt->org[0x4e2]`, both indices scaled
 *     by `struct GsOT_TAG`'s 4 bytes), then wait for vsync — either a
 *     plain `DrawSync(0); VSync(-sync);` (`sync <= 0`) or, tracking overrun
 *     against the global `time`, an extra `VSync(sync)` before
 *     `time = VSync(-1); ResetGraph(1);` — then flip buffers and submit the
 *     sort table (`GsSwapDispBuff`/`GsSortClear`/`GsDrawOt`).
 *
 * D_800976B8 and `time` are TU-local statics with no PSX.SYM record (not
 * exported) — declared here as plain `u32`/`s32` rather than inventing an
 * unverified `PACKET *` (Ghidra's guess): neither is ever dereferenced in
 * this function, only stored/loaded as scalars, so a pointer type would be
 * unverified per the cookbook's "existing Ghidra POINTER type may be a
 * guess" rule. Both are %gp_rel here, same as GameClock/SkipFrame/
 * DrawingPage/OTablePt (config/symbols.main.exe.txt already pins `time` at
 * 0x800976bc from a prior session; D_800976B8 gets a splat auto-name at
 * 0x800976b8, directly between SkipFrame and `time`).
 */

extern s32 GameClock;
extern s16 SkipFrame;
extern GsOT OTable[];
extern s16 DrawingPage;
extern GsOT *OTablePt;
extern GsFOGPARAM Fog;
extern u8 D_80098040[];
extern u32 D_800976B8;
extern s32 time;

extern s32 VSync(s32 mode);

void EndDrawing(s16 sync)
{
    s16 sk;
    u32 t;
    u32 val;
    s32 dp;

    if ((GameClock == (GameClock / 30) * 30) && (SkipFrame == 0))
    {
        val = (u32)GsGetWorkBase() - (u32)D_80098040 - (DrawingPage << 16);
        if (val > 0x10000)
            D_800976B8 = 0x10000;
        else
            D_800976B8 = val;
    }

    sk = SkipFrame;
    if (sk == 1)
        goto case1;
    if (sk < 2)
    {
        if (sk == 0)
            goto case0;
        goto join;
    }
    if (sk == 2)
        goto case2;
    goto join;

case0:
    if (VSync(1) > ((sync - (sync << 4)) << 4) - 0xA)
    {
        SkipFrame = 1;
        return;
    }
    goto join;

case1:
    t = sync;
    sync = t << 1;
    SkipFrame = 0;
    dp = sk - (u16)DrawingPage;
    DrawingPage = dp;
    OTablePt = &OTable[DrawingPage];
    goto join;

case2:
    SkipFrame = 0;

join:
    OTablePt->org[0x7FE] = OTablePt->org[0x4E2];

    if (sync <= 0)
    {
        DrawSync(0);
        VSync(-sync);
    }
    else
    {
        if (VSync(-1) - time < sync)
            VSync(sync);
        time = VSync(-1);
        ResetGraph(1);
    }

    GsSwapDispBuff();
    GsSortClear(Fog.rfc, Fog.gfc, Fog.bfc, OTablePt);
    GsDrawOt(OTablePt);
}
