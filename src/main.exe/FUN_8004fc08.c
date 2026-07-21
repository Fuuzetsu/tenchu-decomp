#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

/*
 * FUN_8004fc08 (0x8004fc08, 0x68 bytes) — CD-audio serial-attribute setter:
 * re-asserts SsSetSerialAttr(0,0,1) either way, then feeds SsSetSerialVol
 * either the persisted CdaStatus volume (voll/volr, same TCdaStatus struct
 * as FUN_8004fbf4.c) when arg0 is non-null, or (0,0) when it's null.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Opposite-polarity branch: Ghidra/m2c render `if (arg0 == 0) {zeros}
 *    else {CdaStatus}`, but the asm's `beqz a0,ELSE` branches AWAY to the
 *    zeros block (laid out physically SECOND) while the CdaStatus block is
 *    the fallthrough (physically FIRST) — the real source is the inverted
 *    `if (arg0 != 0) {CdaStatus} else {zeros}` with bodies swapped
 *    accordingly (the general opposite-polarity rule; this isn't the
 *    null-guard-with-two-returns exception since neither arm returns here,
 *    both merge into the shared SsSetSerialVol call).
 *  - SsSetSerialAttr/SsSetSerialVol are precompiled PsyQ SPU-library calls
 *    (0x8006ec04/0x800701c4, both > 0x80060000) — only the call site's
 *    argument setup is source-shaped, not their bodies.
 *  - The two SsSetSerialVol calls are written OUT SEPARATELY, one per
 *    branch (not merged into one call after the if/else through shared
 *    voll/volr locals): each branch's full 3-argument setup differs (one
 *    reads CdaStatus, one is all-zero), so cc1 first compiles two
 *    independent call sequences, then jump.c's cross-jump pass merges only
 *    the byte-identical SUFFIX (`jal SsSetSerialVol` + its empty delay
 *    slot) into one shared copy — the divergent `a0=0` argument-loading
 *    instruction just before it is NOT identical in both (one is a fresh
 *    `addu $a0,zero,zero`, the other free-rides in the `then` branch's own
 *    unconditional jump's delay slot, reorg-stolen from what would
 *    otherwise be the merge point's first instruction), so it stays
 *    per-branch (cookbook's "Shared tails" section — same mechanic as the
 *    per-loop-exit call+return tail). A single post-merge call with
 *    voll/volr temps compiles 4 bytes short (missing this per-branch `a0`
 *    materialization).
 */

extern TCdaStatus CdaStatus;
extern void SsSetSerialAttr(u8 a, u8 b, u8 c);
extern void SsSetSerialVol(u8 a, u8 voll, u8 volr);

void FUN_8004fc08(s32 arg0)
{
    if (arg0 != 0)
    {
        SsSetSerialAttr(0, 0, 1);
        SsSetSerialVol(0, CdaStatus.voll, CdaStatus.volr);
    }
    else
    {
        SsSetSerialAttr(0, 0, 1);
        SsSetSerialVol(0, 0, 0);
    }
}
