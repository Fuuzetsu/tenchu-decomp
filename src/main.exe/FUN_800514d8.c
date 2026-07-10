#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 * END PSX.SYM */

/*
 * FUN_800514d8 (0x800514d8, 0xd8 bytes) — the per-frame input-poll loop for
 * some end-of-level/results screen: if the just-cleared stage's uid is
 * higher than this character's best-so-far (a byte at PersistentState+0x60,
 * indexed by CHOSEN_CHARACTER — not yet a named field; game_types.h's
 * `field_0x5f[0x3AD]` swallows it as anonymous padding, so it's reached via
 * a raw offset off the same proven `PSTATE` pointer FUN_800566c0.c uses,
 * per the cookbook's "reach a divergent/unnamed field via an offset cast"
 * convention rather than guessing its true array extent), bumps the record.
 * Then loops StartDrawing/GetRealPad/EndDrawing every frame, feeding
 * FUN_8005a7a4 only NEWLY-pressed pad bits (edge-triggered: `pad != 0` on a
 * frame where the PREVIOUS frame's pad READ 0), until FUN_8005a7a4 returns
 * non-zero (presumably "player confirmed/dismissed").
 *
 * `lastpad` is a genuine UNINITIALIZED local on the very first loop
 * iteration — Ghidra's "unaff_s1" is exactly this: $s1 is callee-saved and
 * this function never writes it before the loop, so iteration 1 reads
 * whatever garbage the CALLER happened to leave there. This is the
 * original source's own behaviour (not a decompilation artifact to paper
 * over): the do-while's persisted "last frame's pad" variable is simply
 * declared without an initializer.
 *
 * Not in the demo's PSX.SYM; no candidate name in
 * reference/psxsym-candidates.tsv. reference/psxsym-unplaced.tsv's
 * un-located demo names don't obviously fit either (nothing named for a
 * results/stage-clear input loop specifically).
 *
 * STATUS: NON_MATCHING — 60 of 216 bytes differ, all in the loop body's
 * pad/lastpad/input register shuffle + two scheduling ties. LENGTH matches
 * exactly (54 instructions). The `best[0x60]` record-bump and every field
 * offset/width/type is byte-correct; the residual is confined to three
 * sub-C-level ties, confirmed permuter-immune:
 *
 *  1. The target keeps `input` in a persistent callee-saved $s0 (set to 0
 *     in StartDrawing's delay slot at the loop top, live across BOTH the
 *     StartDrawing and GetRealPad calls, updated by an explicit `move
 *     s0,s1`, then passed as `move a0,s0`). Our cc1 instead COLLAPSES
 *     `input` — since its only consumer is the FUN_8005a7a4 argument, it
 *     rematerialises the value straight into $a0 and drops the persistent
 *     $s0 copy, saving the `move s0,s1` but re-deriving `input` through
 *     caller-saved regs. Every source spelling tried keeps this collapse:
 *     natural `if (lastpad==0 && pad!=0) input=pad; lastpad=pad;` (this
 *     file); a `prev = lastpad; lastpad = pad; if (prev==0 && lastpad!=0)
 *     input = lastpad;` temp-copy that DID reproduce the target's
 *     "update lastpad early in the first branch's delay slot" shape but
 *     added its own extra `move` (55 vs 54); operand-swapping the `&&`.
 *     regalloc.py confirms input's pseudo has no copy-chain forcing it to a
 *     callee-saved reg — the target's persistence is a coloring choice our
 *     cc1's allocator does not make from any equivalent C.
 *  2. `GetRealPad(0)`'s literal-0 argument: target emits a fresh `move
 *     a0,zero`; ours reuses `input`'s just-set 0 (`move a0,s0`) — the same
 *     constant-CSE-vs-rematerialise tie, downstream of (1).
 *  3. The `(s16)` sign-extend of FUN_8005a7a4's return (`sll/sra`) schedules
 *     one slot later in ours (after the SkipFrame store / EndDrawing arg
 *     setup) than in the target (immediately after the call). `result`
 *     spelled `s16` vs `s32` only moves which register the `sll` writes
 *     (autorules scored s16 and s32 within one line of each other, neither
 *     eliminating the tie); `s32` gives the smaller residual and the exact
 *     54-instruction length, so it is kept.
 *
 * A full bounded `tools/permute.py FUN_800514d8` run (~5 min, 900+ scored
 * candidates) never dropped below internal score 340 (no zero) — the
 * cookbook's named permuter-immune signature for a reload/coloring +
 * delay-slot-schedule tie with no C-level lever (same class as the la/reload
 * and guard-delay-slot ties). Parked per the sub-C-level early-stop.
 */
typedef struct
{
    u8 uid;     /* 0x0 — only field this function needs; full 0x1C-byte
                 * layout (name/path/px/py/pz/pr) proven by PlayerOption.c,
                 * repeated here so StageConfig[] indexes with the right
                 * stride. */
    char *name; /* 0x4 */
    char *path; /* 0x8 */
    s32 px;     /* 0xC */
    s32 py;     /* 0x10 */
    s32 pz;     /* 0x14 */
    s32 pr;     /* 0x18 */
} StageConfigEntry; /* 0x1C */

#define PSTATE ((PersistentState *)0x80010000)

extern s16 D_8008EA78;
extern s16 SkipFrame;
extern StageConfigEntry StageConfig[];
extern void StartDrawing(void);
extern void EndDrawing(s32 mode);
extern s32 GetRealPad(s32 port);
extern s16 FUN_8005a7a4(s32 input);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_800514d8", FUN_800514d8);
#else
void FUN_800514d8(void)
{
    s32 pad;
    s32 lastpad;
    s32 input;
    s32 result;
    u8 *best;

    if (PSTATE->stage != D_8008EA78)
    {
        best = (u8 *)PSTATE + PSTATE->chr;
        if (best[0x60] < StageConfig[PSTATE->stage].uid)
        {
            best[0x60] = StageConfig[PSTATE->stage].uid;
        }
    }
    do
    {
        input = 0;
        StartDrawing();
        pad = GetRealPad(0);
        if (lastpad == 0 && pad != 0)
        {
            input = pad;
        }
        lastpad = pad;
        result = FUN_8005a7a4(input);
        SkipFrame = 2;
        EndDrawing(2);
    } while (result == 0);
}
#endif

