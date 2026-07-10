#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ComPad(int port, unsigned char *rxbuf);
 *     PADCMD.C:136, 100 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       int port
 *     param $s2       unsigned char * rxbuf
 *     reg   $s0       struct TPadPort * pad
 *     reg   $s1       int initlevel
 *     reg   $s0       int i
 *     reg   $s3       int port
 *     reg   $v1       int i
 *     reg   $v1       int i
 *     reg   $a1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 63 of 480 bytes differ, all in TWO reorg
 * delay-slot-fill ties (see below); every field offset/type/branch this
 * function establishes is otherwise correct.
 *
 * ComPad (0x8001abc4) — reads one controller's raw report and folds it into
 * PadPort[port>>4][port&3] (main.exe.h's `controller_input`, 14 bytes/entry
 * — see GetRealPad.c/PadShock.c, same TU). rxbuf[1]>>4==8 is the multitap
 * header: recurse over its 4 sub-ports (rxbuf+2, +10, +18, +26). Otherwise
 * rxbuf[0]!=0 (no pad / error) zeroes the held bits + x/y + fAnalog and
 * returns; the normal path folds the raw button bytes into `held`
 * (bit-inverted, matching the pad's active-low wire convention), derives a
 * digital-style x/y from the dpad bits (0x2000/0x8000 -> x, 0x4000/0x1000
 * -> y), records analog-mode (rxbuf[1]>>4==7) in fAnalog, and drives
 * PadSetAct/PadSetActAlign through the act1/act2/actbuf/Send fields based on
 * PadInfoMode's return and the state machine in PadGetState's result
 * (1/2/6).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `pad[6] = 1;` (Send) runs UNCONDITIONALLY right after PadGetState —
 *    it is the branch's own delay-slot fill, not inside the `if`.
 *  - Ghidra's `uVar3 = 0x2d; if (cond1 || (uVar3 = -0x2d, cond2)) store;`
 *    comma-OR is a plain `if (cond1) store = 0x2D; else if (cond2)
 *    store = -0x2D;` — the asm has both bodies reached by a forward
 *    branch/fallthrough into the SAME store instruction (cross-jump
 *    merges the two paths' identical `sh`), not the literal comma form.
 *  - The `held` reload before the second (0x4000/0x1000) test is a REAL
 *    fresh `lhu` in the target (not the register already holding the
 *    just-computed value) — re-read it explicitly rather than reusing a
 *    cached local.
 *  - PadInfoMode's guard needs Ghidra's LITERAL polarity INVERTED
 *    (`if (PadInfoMode(...) != 0) {act9/act8} else {0x40/act8}`) — the
 *    natural `==0` if/else inlines the wrong body first; matches the
 *    "opposite polarity" dispatch rule.
 *
 * THE RESIDUAL (both tried and reverted; no source lever found):
 *  1. Right after computing `held` (the inverted rxbuf[2]/[3] word) and
 *     storing it, the target does an explicit register copy (`move v1,v0`)
 *     before testing `held & 0x2000` — freeing v0 for the 0x2D/-0x2D
 *     constants. Our compile keeps `held` in one register and skips the
 *     copy (1 fewer instruction there), *and* independently reorg steals
 *     the `*(u16*)pad = held;` store into the FIRST bit-test branch's own
 *     delay slot instead of leaving it anchored before the test — visible
 *     in cc1's own pre-schedule output (`nor $3,$3,$2` / `andi $2,$3,0x2000`
 *     / `beq $2,$0,L9` / delay-slot `sh $3,0($16)`). Naming the copy
 *     explicit (`u16 tmp = held;`, with or without a `do{}while(0)` fence)
 *     had zero effect — cc1 copy-propagates the temp away before scheduling.
 *  2. The final `if (initlevel != 6) return;` guard's delay slot: the
 *     target leaves a genuine wasted `nop` there (reorg found nothing safe
 *     to hoist), while our compile finds `move a0,s3` (the next statement's
 *     PadSetActAlign port argument) ready and hoists it in, saving the
 *     `nop` — 1 fewer instruction, then the freed argument setup lets the
 *     `addiu $a1,$a1,%lo(D_800976F0)` land in the following `jal`'s OWN
 *     delay slot instead of executing before it (target keeps `addiu`
 *     before the call and `move a0,s3` in the delay slot instead — the
 *     opposite pairing). This is the named "guard's delay-slot fill tie"
 *     class (StickonCheck) — permuter-immune, not attempted further.
 * `tools/permute.py` was not run on this residual (length-off-by-one, and
 * both mechanisms above are cc1/reorg scheduling choices with no visible
 * source lever, the same signature as the already-documented delay-slot
 * ties elsewhere in this TU).
 */

extern int PadInfoMode(int port, int mode, int unused);
extern int PadGetState(int port);
extern void PadSetAct(int port, u8 *data, int len);
extern void PadSetActAlign(int port, void *data);
extern u8 D_800976F0[];

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ComPad", ComPad);
#else
void ComPad(int port, u8 *rxbuf)
{
    u8 *pad;
    int i;
    u16 held;
    int initlevel;
    int t1, t2;

    if ((rxbuf[1] >> 4) == 8)
    {
        for (i = 0; i < 4; i++)
        {
            ComPad(port + i, rxbuf + 2 + i * 8);
        }
        return;
    }

    pad = (u8 *)&PadPort[port >> 4][port & 3];

    if (rxbuf[0] != 0)
    {
        *(u16 *)pad = 0;
        *(u16 *)(pad + 2) = 0;
        *(u16 *)(pad + 4) = 0;
        pad[6] = 0;
        return;
    }

    t1 = rxbuf[2];
    t2 = rxbuf[3];
    *(u16 *)(pad + 4) = 0;
    *(u16 *)(pad + 2) = 0;
    held = ~((t1 << 8) | t2);
    *(u16 *)pad = held;

    if (held & 0x2000)
        *(s16 *)(pad + 2) = 0x2D;
    else if (held & 0x8000)
        *(s16 *)(pad + 2) = -0x2D;

    held = *(u16 *)pad;
    if (held & 0x4000)
        *(s16 *)(pad + 4) = 0x2D;
    else if (held & 0x1000)
        *(s16 *)(pad + 4) = -0x2D;

    if ((rxbuf[1] >> 4) == 7)
        pad[7] = 1;
    else
        pad[7] = 0;

    if (PadInfoMode(port, 2, 0) != 0)
    {
        pad[0xA] = pad[8];
        pad[0xB] = pad[9];
    }
    else
    {
        pad[0xA] = 0x40;
        pad[0xB] = pad[8];
    }

    initlevel = PadGetState(port);
    pad[6] = 1;
    if (initlevel == 1)
        pad[0xC] = 0;

    if (pad[0xC] == 0)
    {
        PadSetAct(port, pad + 0xA, 2);
        if (initlevel != 2)
        {
            if (initlevel != 6)
                return;
            PadSetActAlign(port, D_800976F0);
        }
        pad[0xC] = 1;
    }
}
#endif
