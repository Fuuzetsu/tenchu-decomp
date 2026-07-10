#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadProc(void);
 *     PADCMD.C:249, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $a0       int ct
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char ComBuf[2][34];
 *     extern struct tag_TItem items[30];
 *     extern struct PADCMD__141fake PadArrange;
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 208 of 368 bytes differ; the root cause is ONE
 * instruction-selection choice (see below) that cascades through the rest
 * of the function's scheduling. Every branch, field offset and condition
 * this function establishes is otherwise correct (independently proven).
 *
 * PadProc (0x8001ada4) — per-frame pad service: ComPad the direct port (0)
 * and the multitap header (0x10, ComBuf[1]), then run the rumble envelope
 * (PadArrange: pow/time/attack/release) and drive PadPort[0][0]'s act1/act2
 * bytes (offsets 8/9 — Ghidra's DAT_800be6d8/DAT_800be6d9, absolute since
 * they're PadPort's own base, not indexed by any port argument): while the
 * attack ramp hasn't finished (attack > time) scale by attack, otherwise by
 * the release ramp (attack-time+release), or turn the motor off entirely
 * once even the release ramp has elapsed. D_8001005D (PadShockAR.c's own
 * persistent-state byte) gates whether the motor fires at all this frame.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - This TU divides by a variable (attack/release) — needs
 *    maspsx --expand-div (Build.hs maspsxGpExterns).
 *  - **Opposite polarity from Ghidra's rendering**: Ghidra shows
 *    `if (attack-time < 1) {release-arm} else {attack-arm}`, but the target
 *    inlines the ATTACK arm at the branch's fallthrough and reaches the
 *    RELEASE arm only by a forward `blez` — write `if (ct >= 1)
 *    {attack-arm} else {release-arm}` to get the attack arm inlined first.
 *  - `PadArrange.time += 1;` right after computing `ct` (unconditional, not
 *    inside any guard) plus a SECOND `+= 1;` at the function's end (not
 *    inside the early-return path) reproduces the target's "+1 always,
 *    +1 more unless the deep guard returns early" split — Ghidra's own
 *    "+1 inside the guard" / "+2 at the end" rendering is the same total
 *    but the WRONG place for reorg to fold the first store into the
 *    top-level branch's delay slot.
 *  - `ct` is reused for the whole function (PSX.SYM's one local, reg $a0):
 *    `attack-time`, then reassigned to `pow*(...)` in each arm — a single
 *    variable, not per-branch temps, matching cc1's CSE of the unions
 *    `PadArrange.attack`/`.release` reads (each field read twice, same
 *    register both times, no intervening store).
 *  - field8 (act1) is 0 in EVERY release-branch outcome (zero or nonzero
 *    D_8001005D) and only becomes 1 in the attack branch's nonzero case —
 *    easy to mis-transcribe as symmetric with field8=1 in both branches.
 *  - `&PadPort` is NOT cached across the mutually-exclusive branches —
 *    the target recomputes `lui/addiu` separately in each of the 3 write
 *    sites too (verified: caching it in one named pointer variable made
 *    the diff WORSE, from 91 to 87 instructions vs target's 92 — an
 *    over-optimization the original didn't have either).
 *
 * THE RESIDUAL: `ct = PadArrange.attack - PadArrange.time;` compiles to a
 * single `subu` here, but the target computes it as `negu $a0,time` +
 * `addiu $v0,time,1` + `addu $a0,a0,attack` — i.e. negates `time` FIRST
 * (interleaved with materializing `time+1` for the later store) and adds
 * `attack` second, one instruction longer than the direct `subu`. Every
 * spelling tried compiles to the same `subu` (cc1's fold canonicalizes
 * `a - b`, `-b + a`, and `-(b - a)` identically — verified standalone with
 * cc1-281 directly, all 5 variants collapse to one `subu`): a plain
 * subtraction, two separate assignment statements, and a nested double
 * negation. This looks like a reload/scheduler-level interaction with the
 * immediately-following `time + 1` materialization (both derive from the
 * same loaded `time` register) rather than anything expressible as a
 * source-level rewrite of the subtraction itself; not resolved this
 * session. Every downstream diff is this single extra instruction
 * shifting the rest of the function's addresses and delay-slot fills.
 */

extern void ComPad(int port, u8 *rxbuf);
extern u8 D_8001005D;
extern u8 ComBuf[2][34];

typedef struct
{
    s32 pow;
    s32 time;
    s32 attack;
    s32 release;
} PadArrangeStruct;

extern PadArrangeStruct PadArrange;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PadProc", PadProc);
#else
void PadProc(void)
{
    int ct;

    ComPad(0, ComBuf[0]);
    ComPad(0x10, ComBuf[1]);

    ct = PadArrange.attack - PadArrange.time;
    PadArrange.time = PadArrange.time + 1;
    if (ct >= 1)
    {
        ct = PadArrange.pow * (PadArrange.attack - ct);
        if (D_8001005D == 0)
        {
            ((u8 *)PadPort)[8] = 0;
            ((u8 *)PadPort)[9] = 0;
        }
        else
        {
            ((u8 *)PadPort)[8] = 1;
            ((u8 *)PadPort)[9] = ct / PadArrange.attack;
        }
    }
    else
    {
        if (ct + PadArrange.release < 1)
        {
            ((u8 *)PadPort)[8] = 0;
            ((u8 *)PadPort)[9] = 0;
            return;
        }
        ct = PadArrange.pow * (ct + PadArrange.release);
        if (D_8001005D == 0)
        {
            ((u8 *)PadPort)[8] = 0;
            ((u8 *)PadPort)[9] = 0;
        }
        else
        {
            ((u8 *)PadPort)[8] = 0;
            ((u8 *)PadPort)[9] = ct / PadArrange.release;
        }
    }
    PadArrange.time = PadArrange.time + 1;
}
#endif
