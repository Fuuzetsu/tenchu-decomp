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
 * STATUS: NON_MATCHING — exact 368-byte / 92-instruction extent, with 172
 * differing linked bytes and fuzzy 73.63 (up from 52.46).  The raw aligned
 * residual is 36 lines in 14 blocks.  The previous 208-byte claim came from
 * a 364-byte draft and was therefore only a carved-window estimate; this is
 * the first authoritative exact-length checkpoint.
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
 *  - Named `time` and `attack` snapshots recover the target's load order and
 *    let the first `time + 1` store fill the top-level branch delay slot.  A
 *    SECOND increment at the function's end remains skipped by the deep
 *    motor-off path, preserving retail's "+1 always, +1 unless off" split.
 *  - Both guarded divisions execute BEFORE D_8001005D is tested in retail.
 *    Computing the quotient only in the nonzero arm changes both behavior
 *    and the expanded-div CFG.  Hoisting each quotient and spelling the
 *    active arm first (`D_8001005D != 0`) recovers the target branch polarity.
 *  - Distinct branch-scoped `pad` pointers prevent cc1 from cross-jumping the
 *    attack and release zero-store tails.  The release arm first adds
 *    `release` to `ct`, branches to the late `motor_off` label on `ct <= 0`,
 *    and otherwise keeps the quotient in the shared `ct` carrier.
 *  - The identical `ct / release` arms selected by initialized, nonnull
 *    `pad` are a safe zero-code CFG fence.  jump2 erases the condition, while
 *    the earlier pass boundary preserves the exact extent and closest
 *    release scheduling.  A 140-candidate depth-two guided resweep found no
 *    composing exact-length improvement beyond this checkpoint.
 *  - field8 (act1) is 0 in EVERY release-branch outcome (zero or nonzero
 *    D_8001005D) and only becomes 1 in the attack branch's nonzero case —
 *    easy to mis-transcribe as symmetric with field8=1 in both branches.
 *
 * The leading residual remains fold/scheduling: the split pure-C spelling
 * still becomes `nop; subu attack,time`, where retail has
 * `negu time; addiu time,1; addu attack`.  Eliminated diamonds can preserve
 * the negate/add pair but force a second PadArrange base and lose exact
 * length.  The other residuals are the two branch-local PadPort/shock-load
 * schedules around the expanded divides and the final absolute motor-off
 * address form; volatile and surviving-branch variants were rejected.
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
    int time;
    int attack;

    ComPad(0, ComBuf[0]);
    ComPad(0x10, ComBuf[1]);

    time = PadArrange.time;
    attack = PadArrange.attack;
    ct = -time;
    PadArrange.time = time + 1;
    ct += attack;
    if (ct >= 1)
    {
        u8 *pad;

        ct = PadArrange.pow * (PadArrange.attack - ct);
        pad = (u8 *)PadPort;
        ct = ct / PadArrange.attack;
        if (D_8001005D != 0)
        {
            pad[8] = 1;
            pad[9] = ct;
        }
        else
        {
            pad[8] = 0;
            pad[9] = 0;
        }
    }
    else
    {
        u8 *pad;

        ct += PadArrange.release;
        if (ct <= 0)
            goto motor_off;

        ct = PadArrange.pow * ct;
        pad = (u8 *)PadPort;
        if (pad != 0)
        {
            ct = ct / PadArrange.release;
        }
        else
        {
            ct = ct / PadArrange.release;
        }
        if (D_8001005D != 0)
        {
            pad[8] = 0;
            pad[9] = ct;
        }
        else
        {
            pad[8] = 0;
            pad[9] = 0;
        }
    }
    PadArrange.time = PadArrange.time + 1;
    goto done;

motor_off:
    ((u8 *)PadPort)[8] = 0;
    ((u8 *)PadPort)[9] = 0;
done:
    ;
}
#endif
