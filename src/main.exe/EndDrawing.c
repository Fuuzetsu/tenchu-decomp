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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sync
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern unsigned char Packet[2][65536];
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
 * above; `time`/PacketUsed confirmed the same TU-local %gp_rel treatment as
 * GameClock/SkipFrame/DrawingPage/OTablePt via the raw `.s`).
 *
 * Body, in order:
 *  1. Every 30th frame (`GameClock % 30 == 0`, lowered by GCC to a quotient
 *     and re-multiply), while not already
 *     mid-skip (`SkipFrame == 0`), snapshot how much of the current GPU
 *     packet buffer is left by subtracting the current typed page base from
 *     `GsGetWorkBase()`, clamped to PACKET_PAGE_SIZE.
 *     Two independent `sw`s (one per branch) store the clamped
 *     and unclamped values — plain if/else, no eager-store-then-override
 *     idiom needed (each arm's stored VALUE differs, so cc1 has nothing to
 *     cross-jump-merge).
 *  2. A genuine `switch` over the captured `sk` value for exactly {0,1,2},
 *     with no default. expand_case emits the target's 1, <2, ==0 (nested),
 *     ==2 test order while retaining the case bodies in plain 0,1,2 source
 *     order. Values outside that set reach the shared tail unchanged.
 *     - case 0: if the frame is overrunning its budget
 *       (`VSync(1) > -sync * SCREEN_H - 10`), start skipping
 *       (`SkipFrame=1`) and return immediately — this return, not a
 *       fallthrough, is what skips the shared tail below. VSync reports
 *       elapsed scanlines, so each requested wait contributes one screen
 *       height to the budget, less a ten-line margin. The condition stays
 *       call-first so cc1 evaluates VSync before forming the budget. Writing
 *       the negation on `sync` is significant to this compiler: the natural
 *       `-sync * SCREEN_H` emits the target's subtraction-and-shift constant
 *       multiply, whereas `sync * -SCREEN_H` adds a separate negation.
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
 *       dominating case dispatch and, when `sk` is `s32`
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
 * `time` is PSX.SYM's exact 3DCTRL.C static `int`; PacketUsed remains an
 * unnamed retail neighbour. Both are scalar-only here, so PacketUsed stays
 * a plain `u32` rather than inheriting Ghidra's unverified `PACKET *` guess.
 * Both are %gp_rel here, same as GameClock/SkipFrame/
 * DrawingPage/OTablePt (config/symbols.main.exe.txt already pins `time` at
 * 0x800976bc from a prior session; PacketUsed gets a splat auto-name at
 * 0x800976b8, directly between SkipFrame and `time`).
 */

/* Leave only the PSX.SYM-proven outer bound incomplete: this retains the
 * absolute symbol access while preserving the 64 KiB page type. */
extern u8 Packet[][PACKET_PAGE_SIZE];
extern u32 PacketUsed;
extern s32 time;

extern s32 VSync(s32 mode);

void EndDrawing(short sync)
{
    s16 sk;
    u32 t;
    u32 val;
    s32 dp;

    if ((GameClock % 30 == 0) && (SkipFrame == 0))
    {
        val = GsGetWorkBase() - Packet[DrawingPage];
        if (val > PACKET_PAGE_SIZE)
            PacketUsed = PACKET_PAGE_SIZE;
        else
            PacketUsed = val;
    }

    sk = SkipFrame;
    switch (sk)
    {
    case SKIPFRAME_NONE:
        if (VSync(1) > -sync * SCREEN_H - 10)
        {
            SkipFrame = SKIPFRAME_SKIPPED;
            return;
        }
        break;

    case SKIPFRAME_SKIPPED:
        t = sync;
        sync = t << 1;
        SkipFrame = 0;
        dp = sk - (u16)DrawingPage;
        DrawingPage = dp;
        OTablePt = &OTable[DrawingPage];
        break;

    case SKIPFRAME_AFTER_LOAD:
        SkipFrame = 0;
        break;
        }

        OTablePt->org[0x7FE] = OTablePt->org[DEPTH_LIMIT];

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
