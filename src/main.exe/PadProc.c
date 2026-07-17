#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadProc(void);
 *     PADCMD.C:249, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $a0       int ct
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char ComBuf[2][34];
 *     extern struct tag_TItem items[30];
 *     extern struct PADCMD__141fake PadArrange;
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — exact 368-byte / 92-instruction extent, 28 of 368
 * linked bytes differ.  (Was 30; `extern u8 D_8001005D[]` — an unknown-size
 * ARRAY rather than a scalar — is worth 2 bytes: it makes cc1 split the
 * address itself, emitting `lui v0,%hi` + `lbu $d,%lo(...)(v0)` with the %hi
 * in v0 as retail has, instead of handing a whole fixed address to the
 * assembler to expand through one register.)
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
 *  - Keep `attack` named, but spell the first update as the single expression
 *    `ct = -PadArrange.time++`.  PSX.SYM's per-address line records put the
 *    negate/increment on original line 257 and the attack add on line 258;
 *    that spelling recovers retail's `negu; addiu; addu` sequence and its
 *    delay-slot store.  A SECOND increment at the function's end remains
 *    skipped by the deep motor-off path, preserving retail's "+1 always, +1
 *    unless off" split.
 *  - The identical-arm fences are NOT about "preserving source identities";
 *    measured, their whole job is REFERENCE COUNTING for global_alloc.  See
 *    the residual analysis below before touching them.
 *  - field8 (act1) is 0 in EVERY release-branch outcome (zero or nonzero
 *    D_8001005D) and only becomes 1 in the attack branch's nonzero case —
 *    easy to mis-transcribe as symmetric with field8=1 in both branches.
 *
 * ---- The residual (28 bytes), measured and traced to the RTL ----
 *
 * 16 of the 28 bytes are two `mflo`/`lui %hi(PadPort)` swaps (one per branch);
 * the other 12 are single register-field bytes from one rotation per branch.
 *
 * REFUTED: the `mflo` displacement is NOT downstream of the D_8001005D load's
 * register, and the two are not one question.  A control build in which the
 * attack branch's `shock` DID land in v0 (matching retail) still emitted
 * `mult; mflo; lui %hi(PadPort)` — the `mflo` did not move.  The gap-filler is
 * the PadPort address `high` insn, not the global read.
 *
 * WHY `shock` cannot reach v0 while it is a global pseudo (from .greg):
 *   ;; 94 conflicts: 80 81 92 93 94 2 29     <- 94 = shock(attack) -> "94 in 7"
 * EVERY global allocno conflicts with hard reg 2 (v0), because local_alloc
 * runs first and hands v0 to the whole chain of block-local %hi scratches
 * (89,100,102,104,105,107,109,111,112,113 "in 2").  So global_alloc can never
 * colour anything v0.  Retail's `lbu v0,%lo(D_8001005D)(v0)` reuses the dying
 * address scratch as the destination — only local_alloc does that.  `shock`
 * must therefore be a LOCAL pseudo: defined and dead inside one cc1 block.
 *
 * WHY making it local costs more than it buys (all three measured):
 *  a) `shock` local needs the lbu, the div and the `beqz` in ONE block, i.e.
 *     no divisor fence.  With the fence, the lbu sinks below the div and lands
 *     against the `beqz`; maspsx must insert a load-delay nop -> 372 bytes.
 *  b) Without the divisor fence, `attack` loses the two duplicated
 *     `product / attack` refs.  global_alloc orders allocnos by
 *     floor_log2(refs)*refs/live_length*10000:
 *        base28   p81 attack 8/21 -> 11428 , p92 base_hi 8/21 -> 11428  (TIE,
 *                 broken by allocno number, 81 < 92, so attack takes a1)
 *        unfenced p81 attack 7/19 ->  7368 , p92 base_hi 7/16 ->  8750
 *     so base_hi wins a1, attack is pushed to a2, and BOTH branches' `pad`
 *     collapse onto a1 — retail keeps attack-pad in a2 and release-pad in a1.
 *  c) With both `pad`s in one register the two `sb zero; sb zero` motor-off
 *     tails become identical, jump2 cross-jumps them, and the function loses
 *     3 instructions -> 356 bytes.  Retail's tails survive ONLY because the
 *     registers differ; no source-level donor prevents the merge without
 *     re-extending `shock` into the second block (which re-globalises it).
 *
 * Attempts to buy `attack` its 8th ref (the floor_log2 cliff at 8 is what
 * creates the tie) without re-splitting the block all failed at the LENGTH
 * gate: re-keying either fence to `attack` adds only one ref (356); wrapping
 * shock+div+test in a divisor fence stops the arms cross-jumping (428);
 * duplicating the mult into the arms reaches the 7/22-vs-7/22 tie but leaves
 * the duplicate in (396); duplicating only the `pow` load reaches 368 but
 * re-rotates the whole frame (102).
 *
 * So the 28 bytes are one coupled choice — `shock` in v0 REQUIRES a block
 * merge that costs `attack` the a1 tie-break — and the exact 368-byte extent
 * is only reachable on the losing side of it.  The `mflo` swap is a separate,
 * still-open sched2 priority question (both sides' v0 dependency chains are
 * the same length, so it is not the anti-dep chain).
 */

extern void ComPad(int port, u8 *rxbuf);
extern u8 D_8001005D[];
extern u8 ComBuf[2][34];

typedef struct
{
    s32 pow;
    s32 time;
    s32 attack;
    s32 release;
} PadArrangeStruct;

typedef struct
{
    u16 held;
    s16 x;
    s16 y;
    u8 active;
    u8 analog;
    u8 act1;
    u8 act2;
    u8 actbuf[2];
    u8 send;
    u8 pad;
} PadProcPort;

extern PadArrangeStruct PadArrange;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PadProc", PadProc);
#else
void PadProc(void)
{
    int ct;
    int attack;

    ComPad(0, ComBuf[0]);
    ComPad(0x10, ComBuf[1]);

    ct = -PadArrange.time++;
    attack = PadArrange.attack;
    ct += attack;
    if (ct >= 1)
    {
        PadProcPort *pad;
        int product;
        int shock;

        product = PadArrange.pow * (PadArrange.attack - ct);
        if (product != 0)
        {
            pad = (PadProcPort *)PadPort;
            shock = D_8001005D[0];
        }
        else
        {
            pad = (PadProcPort *)PadPort;
            shock = D_8001005D[0];
        }
        if (attack != 0)
        {
            ct = product / attack;
        }
        else
        {
            ct = product / attack;
        }
        if (shock != 0)
        {
            if (pad != 0)
            {
                pad->act1 = 1;
            }
            else
            {
                pad->act1 = 1;
            }
            pad->act2 = ct;
        }
        else
        {
            pad->act1 = shock;
            pad->act2 = shock;
        }
    }
    else
    {
        PadProcPort *pad;
        int product;
        int release;
        int shock;

        release = PadArrange.release;
        ct += release;
        if (ct <= 0)
            goto motor_off;

        product = PadArrange.pow * ct;
        if (product != 0)
        {
            pad = (PadProcPort *)PadPort;
            shock = D_8001005D[0];
        }
        else
        {
            pad = (PadProcPort *)PadPort;
            shock = D_8001005D[0];
        }
        ct = product / release;
        if (shock != 0)
        {
            pad->act1 = 0;
            pad->act2 = ct;
        }
        else
        {
            pad->act1 = 0;
            pad->act2 = 0;
        }
    }
    PadArrange.time = PadArrange.time + 1;
    goto done;

motor_off:
    ((PadProcPort *)PadPort)->act1 = 0;
    ((PadProcPort *)PadPort)->act2 = 0;
done:
    ;
}
#endif
