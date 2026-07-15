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
 * STATUS: NON_MATCHING — exact 368-byte / 92-instruction extent, with 30
 * differing linked bytes and fuzzy 82.61 (up from 80.43, 73.63, and the first
 * draft's 52.46).  The raw aligned residual is 20 lines in 12 blocks; the
 * structural-only view is 8 lines in 6 blocks.  This supersedes the prior
 * authoritative 172-byte checkpoint without relying on a carved window.
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
 *  - Each branch keeps a named multiplication `product`, then uses a
 *    product-keyed identical-arm fence to capture the typed PadPort base and
 *    shock byte before assigning the quotient to `ct`.  jump2 erases the
 *    artificial condition, but the earlier CFG boundary preserves the
 *    numerator, address and flag as distinct source identities.  This is the
 *    broad recovery that moved the checkpoint from 172 to 52 bytes.
 *  - An attack-divisor identical-arm fence and a smaller identical `act1`
 *    store donor recover the target PadArrange/attack allocation without
 *    surviving in the output.  Storing the branch-proven `int shock` in the
 *    disabled attack arm prevents cross-jumping into the release zero tail;
 *    combine still emits the target's `sb zero` pair.  Keeping `shock` as u8
 *    instead left those stores sourced from its hard register (43 bytes).
 *  - Cache `release` before the release product fence.  Re-reading the member
 *    after that multi-predecessor join costs a fresh PadArrange base/load/nop
 *    triplet; the cache removes exactly those three instructions.
 *  - field8 (act1) is 0 in EVERY release-branch outcome (zero or nonzero
 *    D_8001005D) and only becomes 1 in the attack branch's nonzero case —
 *    easy to mis-transcribe as symmetric with field8=1 in both branches.
 *
 * The remaining residual is branch-local multiply-result/address scheduling
 * and hard-conflict rotations among release, product and shock.  Recreating
 * the likely same-TU PadShock inlining recovers the desired load schedule,
 * but old cc1 then cross-jumps identical zero-store tails and loses the exact
 * extent; a focused follow-up found no exact-length composition below 30
 * linked bytes.
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
            shock = D_8001005D;
        }
        else
        {
            pad = (PadProcPort *)PadPort;
            shock = D_8001005D;
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
            shock = D_8001005D;
        }
        else
        {
            pad = (PadProcPort *)PadPort;
            shock = D_8001005D;
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
