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
 *  - The identical-arm fences are NOT about "preserving source identities",
 *    and NOT (as this file previously claimed) about reference counting for
 *    global_alloc.  Their real and only job is to give `pad`/`shock` a SECOND
 *    ASSIGNMENT, which defeats sched1's birthing_insn_p sink.  See the
 *    residual analysis below before touching them.
 *  - field8 (act1) is 0 in EVERY release-branch outcome (zero or nonzero
 *    D_8001005D) and only becomes 1 in the attack branch's nonzero case —
 *    easy to mis-transcribe as symmetric with field8=1 in both branches.
 *
 * ---- The residual (28 bytes), measured and traced to cc1's own source ----
 *
 * 16 of the 28 bytes are two `mflo`/`lui %hi(PadPort)` swaps (one per branch);
 * the other 12 are single register-field bytes from one rotation per branch.
 *
 * THE MECHANISM (was "a still-open sched2 priority question"; it is neither
 * still-open nor sched2).  It is sched1's birthing_insn_p / adjust_priority,
 * read straight out of gcc-2.8.1 sched.c and confirmed in the ready lists:
 *
 *   birthing_insn_p(pat): returns 0 if reload_completed == 1  -> SCHED1 ONLY.
 *   Otherwise, for `(set (REG dest) ...)` with dest live in bb_live_regs,
 *   it returns REG_N_SETS(dest) == 1.
 *   adjust_priority(prev): n_deaths is ALWAYS 0 (a comment in the file admits
 *   the REG_DEAD scan "has no effect, because REG_DEAD notes are removed
 *   before we ever get here"), so it always reaches `case 0:` and raises a
 *   birthing insn to max_priority — LAUNCH_PRIORITY 0x7f000001 while an insn
 *   is being scheduled (sched.c sets that on the current insn around the
 *   schedule_insn call).  Its stated intent: "Prefer scheduling insns which
 *   make registers live" — and sched is BACKWARD, so "prefer scheduling" means
 *   PLACE AS LATE AS POSSIBLE, shortening the live range.
 *
 * So: a SINGLE-ASSIGNMENT local SINKS toward its use.  With the fence removed,
 * the block merges and `tools/schedtrace.py PadProc --pass sched` shows all
 * four address insns bumped and therefore sunk below the div:
 *   ;; ready list at T-2: 53 (7f000001) 74 (d), now 53 74      <- 74 = div
 *   ;; ready list at T-3: 74 (d) 52 (7f000001) 57 (7f000001)
 *   ;; ready list at T-4: 52 (7f000001) 74 (d) 55 (7f000001)
 * (52 = lui %hi(PadPort), 53 = addiu pad, 55 = lui %hi(D_8001005D),
 *  57 = lbu shock.)  The lbu then lands against the `beqz` -> load-delay nop
 * -> 372.  The fence's ONLY job is to give pad/shock a second assignment so
 * REG_N_SETS != 1 and the bump never fires.  Nothing to do with refcounts.
 *
 * PROOF the sink is the whole 16 bytes: declare pad/product/shock at FUNCTION
 * scope (each branch then assigns the same variable -> REG_N_SETS == 2, no
 * bump, no branch, no block split).  `lui v0,0x800c` immediately moves into
 * the mult shadow and matches retail byte-for-byte at 0x8001ae00, as does
 * `lui v0,0x8001` at 0x8001ae0c.
 *
 * WHY THAT IS NOT THE ANSWER — the dilemma, measured (sweep over which locals
 * are function-scope; scratch script kept out of tree):
 *   (none shared) 372 | pad 356 | product 372 | shock 368/169B
 *   pad+product 356 | pad+shock 356 | product+shock 368/170B | all three 356
 * Retail needs `pad` BOTH ways at once:
 *   - not sinking  => REG_N_SETS(pad) >= 2 => the two branches assign ONE
 *     variable => ONE pseudo => ONE hard register;
 *   - but retail's attack-pad is a2 and release-pad is a1 => TWO pseudos.
 *   With one register the two `sb zero; sb zero` tails become identical,
 *   jump2 cross-jumps them and the function loses 3 insns -> 356.
 * A block-scoped pad with two straight-line assignments is not reachable: the
 * second is redundant and cse folds it before flow computes REG_N_SETS, and
 * any construct that stops cse folding it (a label) re-splits the block and
 * re-loses the mult shadow.  Sharing only `shock` keeps the exact 368 length
 * but rotates 56 insns (169B) because `pad` still sinks.
 *
 * So the 28 bytes remain one coupled choice, but the coupling is NOT the
 * global_alloc a1 tie-break this file used to claim — it is REG_N_SETS: retail
 * wants pad to have two sets and two registers, and C gives you one or the
 * other.  The unexplored lever is a source shape where `pad` is not a variable
 * at all (retail's third PadPort address, motor_off's, lives in v0 and is
 * built from a `lui` parked in the blez delay slot — three distinct address
 * pseudos, which is what a no-`pad`-variable source would produce).  PSX.SYM
 * lists exactly ONE local for this function (`int ct`), which is evidence for
 * that shape and against the pad/product/shock locals this draft uses.
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
