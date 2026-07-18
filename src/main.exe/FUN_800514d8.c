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
 * non-zero (presumably "player confirmed/dismissed"). The `prev`/capture
 * shape mirrors the real matched sibling update_pressed_buttons.c's
 * `previously_pressed = buttons->currently_pressed;` idiom: capture the old
 * cross-iteration value into a loop-local temp immediately before it is
 * overwritten, and test the captured temp rather than re-reading the
 * about-to-change persistent local.
 *
 * `lastpad` is a genuine UNINITIALIZED local on the very first loop
 * iteration — Ghidra's "unaff_s1" is exactly this: $s1 is callee-saved and
 * this function never writes it before the loop, so iteration 1 reads
 * whatever garbage the CALLER happened to leave there. This is the
 * original source's own behaviour (not a decompilation artifact to paper
 * over): the do-while's persisted "last frame's pad" variable is simply
 * declared without an initializer. `pad`/`input`/`prev` are genuinely
 * loop-iteration-local (never read across a back-edge) and are declared
 * inside the `do { }` body accordingly; only `lastpad` (persists across
 * iterations) and `result` (tested by the trailing `while`, which is outside
 * the loop body's own scope) need function-level declarations.
 *
 * Not in the demo's PSX.SYM; no candidate name in
 * reference/psxsym-candidates.tsv. reference/psxsym-unplaced.tsv's
 * un-located demo names don't obviously fit either (nothing named for a
 * results/stage-clear input loop specifically).
 *
 * STATUS: NON_MATCHING — 12 of 216 bytes differ (was 60; two independent,
 * fully-verified fixes closed 48 of those 60 bytes this round):
 *
 *  A. (60->43, MECHANICAL — tools/autorules.py) `D_8008EA78` retyped
 *     `extern s16 D_8008EA78;` -> `extern s16 D_8008EA78[];` (`[0]` at the
 *     one use site). Both spellings emit the IDENTICAL `lui/lh` pair for the
 *     read itself (splat labels the extern-array split-vs-unsplit lever,
 *     cookbook §3.12) — the array typing does not change that instruction,
 *     it changes cc1's SCHEDULING PRIORITY for materialising its address
 *     relative to the four unrelated prologue register saves, closing what
 *     the previous park called "two scheduling ties" (cluster 0, a pure
 *     `lui` reorder around the `sw ra/s0-s2`) completely — 17 bytes, zero
 *     residual in that range afterward.
 *  B. (43->12, HUMAN RESPELLING found via a corrected permuter, verified by
 *     hand-porting the semantic delta and re-measuring — never by score)
 *     GetRealPad's return routes through `result` (already declared, for
 *     FUN_8005a7a4's return, and dead at this point in the loop) as a
 *     transient scratch for the `lastpad` update, AND `lastpad`'s OLD value
 *     is captured into a real, referenced `prev` local — the same
 *     "capture-before-overwrite" idiom update_pressed_buttons.c uses for an
 *     unrelated field — instead of testing `lastpad` in place. Neither half
 *     alone reaches this: `pad = GetRealPad(0);` with no `result` hop is 4
 *     bytes SHORT (212 vs 216 — `result` is load-bearing for length, not
 *     just bytes); `prev = lastpad;` with no `result` hop scores the same 13
 *     as no `prev` at all (a plain copy of an unrelated variable is not
 *     ballast — cse folds it, nullcheck-confirmed). The natural PLACEMENT
 *     also matters: `result = pad;`/`prev = lastpad;` must both sit BEFORE
 *     the `if`, not after (after gives 212, 4 bytes short, confirmed
 *     directly) — this is the "position, not just presence" lever
 *     (cookbook §3.8 item 4) applied to real, referenced locals rather than
 *     synthetic ballast.
 *
 * The remaining 12 bytes are the SAME sub-C-level allocation tie the
 * original park (see git history) diagnosed, now cornered to its irreducible
 * core: the target keeps `input` in a persistent callee-saved $s0 (set to 0
 * in StartDrawing's delay slot at the loop top, live across BOTH the
 * StartDrawing and GetRealPad calls, updated by an explicit `move s0,s1`,
 * then read ONCE at the call via `move a0,s0`). Our cc1 instead COLLAPSES
 * `input` — since its only consumer is the FUN_8005a7a4 argument and it is
 * defined AFTER both calls in every spelling that keeps the correct length,
 * cc1 rematerialises the value straight into $a0 at each defining site
 * (`move a0,zero` / `move a0,s0`) rather than reading back a persisted copy.
 * `input = 0;` moved to be the loop's FIRST statement (before StartDrawing,
 * matching the target's literal delay-slot position) was retried fresh in
 * THIS round, standalone and combined with the working `prev`/`result`
 * structure: both give 224 bytes (8 over budget, 2 extra instructions) —
 * confirmed directly, not inherited from the old park. regalloc.py confirms
 * `input`'s pseudo has an explicit `pref=a0` (donated by its own copy into
 * the call argument) and no copy-chain forcing a callee-saved home; the
 * target's persistence is a coloring choice this cc1 does not make from any
 * length-correct equivalent C found so far.
 *
 * TWO fresh, properly RE-SEEDED bounded permuter rounds this session (from
 * the 15-byte and 13-byte checkpoints; a prior park's permuter claim is VOID
 * — it predates the -fno-builtin fix) both independently converged on the
 * SAME lower-scoring candidate and NOTHING ELSE below 12: `input = 0; input
 * = lastpad;` then testing `input == 0` instead of `lastpad == 0` (9 bytes,
 * and an 11-byte cousin). REJECTED, twice, as a semantic bug, not adopted:
 * traced by hand AND confirmed by building it — on a frame where the button
 * is already held (`lastpad != 0`), `input` keeps `lastpad`'s nonzero value
 * instead of falling back to 0 (`bnez a0,...` taken, `a0` never reset), so
 * FUN_8005a7a4 would wrongly receive a nonzero "newly pressed" argument
 * every frame a button stays held, not just on the true rising edge — and it
 * STILL fails to reproduce the semantically load-bearing `move s0,zero`
 * default. A same-scoring 12-byte candidate reaching this same floor via
 * `result++; result--;` ballast was also found and REJECTED in favor of the
 * `prev`-capture spelling above: both measure identically, but the ballast
 * is exactly the "weird byte-chasing construct" this round's brief asks to
 * avoid, while `prev` is a real, referenced local matching a real sibling's
 * idiom. Two rounds landing on the identical reject with no new safe
 * candidate is treated as the search having covered this neighbourhood;
 * parked evidence-complete at the sub-C-level early-stop.
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

extern s16 D_8008EA78[];
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
    s32 lastpad;
    s32 result;
    u8 *best;

    if (PSTATE->stage != D_8008EA78[0])
    {
        best = (u8 *)PSTATE + PSTATE->chr;
        if (best[0x60] < StageConfig[PSTATE->stage].uid)
        {
            best[0x60] = StageConfig[PSTATE->stage].uid;
        }
    }
    do
    {
        s32 pad;
        s32 input;
        s32 prev;

        StartDrawing();
        pad = GetRealPad(0);
        input = 0;
        prev = lastpad;
        result = pad;
        if (prev == 0 && pad != 0)
        {
            input = pad;
        }
        lastpad = result;
        result = FUN_8005a7a4(input);
        SkipFrame = 2;
        EndDrawing(2);
    } while (result == 0);
}
#endif

