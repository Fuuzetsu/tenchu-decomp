#include "common.h"
#include "main.exe.h"
#include <psxsdk/libcd.h>

/*
 * STATUS: MATCH.
 *
 * Two fixes closed this from a 23-byte, 3-cluster residual:
 *
 * 1. The header used to claim "two distinct retry destinations": `CdReady`
 *    failure restarts from the very top (re-seek), `CdGetSector` failure
 *    restarts only from the cursor-reinit + mode-6 re-prime point,
 *    skipping the seek. That reading was WRONG. Reading the target .s
 *    directly: the `CdGetSector`-fail branch (`beqz v0,.L8005F3CC`) has
 *    `li v0,0xA0` in its OWN delay slot — the exact same constant the
 *    `CdReady`-fail path (`bne v0,v1,.L8005F3C8`) materialises via a real
 *    instruction one label earlier (`.L8005F3C8`, which just falls
 *    through into `.L8005F3CC`). Both paths land at `.L8005F3CC`, which
 *    calls `CdIntToPos` with `$s6` — the ORIGINAL `sector` parameter, not
 *    the running `curSector` — and re-stores `param[0]=0xA0`. So a
 *    `CdGetSector` failure re-seeks and discards ALL progress exactly like
 *    a `CdReady` failure; `.L8005F3C8` vs `.L8005F3CC` is purely the
 *    compiler choosing to fill the `beqz`'s own delay slot with the
 *    shared prefix instruction for ONE of the two incoming edges, not two
 *    different source labels. Fix: both `goto full_retry;` (no separate
 *    `reseek_retry:` label) — this alone took the residual from 23 to 8
 *    bytes (fixed BOTH the `CdIntToPos`-delay-slot-order cluster and the
 *    mode-0xE back-edge's `li v0,0xA0` rematerialisation cluster, since
 *    both were downstream of the wrong control-flow graph, not separate
 *    sub-C-level ties as originally guessed).
 * 2. The last cluster (copy loop's `chunk>0` entry test vs its `i=0` init,
 *    swapped one instruction relative to each other — a delay-slot-filler
 *    tie between two independent, harmless candidates) was NOT
 *    permuter-immune: `tools/permute.py` found a zero-score candidate in
 *    638 iterations once run against the corrected 8-byte baseline (the
 *    EARLIER 260s run had been searching around the wrong 23-byte
 *    baseline, which is presumably why it never found this). The fix:
 *    move `src = data + off;` to be the FIRST statement INSIDE the `for`
 *    loop body instead of immediately before it. This makes `src` a
 *    genuine per-iteration recomputation of a LOOP-INVARIANT value, which
 *    `loop.c` hoists back out to the preheader — but the resulting
 *    preheader placement schedules differently than a source-level
 *    pre-loop statement does, and picks the same delay-slot filler
 *    (`i=0`, not `src=data+off`) the target does. (Cross-reference: this
 *    is the same "loop.c hoisting is a threshold economy" family as
 *    `leResetEnemyLayout`'s pre-assigned invariant, just via placement
 *    inside the loop body rather than a named local before it.)
 *
 * FUN_8005f380 (0x8005f380, 0x1D8 bytes) — raw CD sector reader: reads
 * `length` bytes starting at byte `byteOffset` of sector `sector` into
 * `buffer` (see cd_read.c / FUN_8005f7d0.c, both of which already declare
 * `extern void FUN_8005f380(void *buffer, int sector, int byteOffset,
 * int byteLength);`). Below the 0x80060000 PsyQ/CRT boundary, so it's a
 * game-TU function even though every callee it makes is a BIOS libcd
 * primitive (CdControlB/CdReady/CdGetSector/CdPosToInt/CdIntToPos/VSync).
 *
 * Shape: seek to `sector` (CdControlB mode 0xE, retried while busy), then
 * "read" mode (CdControlB mode 6, retried while busy) to prime streaming,
 * then loop: wait for a ready sector (CdReady), pull it whole
 * (CdGetSector, one 0x203-word/2060-byte raw sector: a 12-byte subheader
 * + 2048 data bytes), confirm its position (CdPosToInt) matches the
 * expected running sector — if not, the drive drifted: re-seek (mode 6)
 * to the expected sector and keep waiting. On a match, copy the wanted
 * slice (clamped to one sector, i.e. `min(off+remaining, 0x800) - off`
 * bytes starting `off` bytes into the sector's DATA portion, skipping the
 * 12-byte subheader) into the caller's buffer, advance, and repeat until
 * `remaining` runs out — then a last CdControlB mode-9 "stop" (retried
 * while busy) and return. If `CdReady` itself ever reports "not ready",
 * the WHOLE sequence restarts from the top (re-seek included); if only
 * `CdGetSector` fails, just the mode-6 re-read priming restarts (the
 * original seek position is unchanged).
 *
 * Buffer layout (Ghidra's own locals, all proven by the raw offsets +
 * frame math: 0x810 + 8 + 8 = 0x820 locals, + 0x28 (10 saved regs) + 0x10
 * (own call args) = 0x858, the exact frame):
 *  - Ghidra's `aCStack_848 [3]` (a CdlLOC[3], 12 bytes) immediately
 *    followed by `auStack_83c [2052]` is ONE 0x810-byte buffer (12+2052 =
 *    0x810 exactly) — the raw CdGetSector DMA target (its own 12-byte
 *    subheader, then the 2048-byte payload); Ghidra's split into two
 *    differently-typed locals is just its own per-access-pattern
 *    heuristic, not two real buffers (cookbook: "overlapping stack
 *    locals... the original really used one buf[N] + casts").
 *  - `local_38 [8]` (only byte 0 used, as CdControlB mode 0xE's own
 *    scratch param — a fresh 8-byte local, not read back).
 *  - `aCStack_30 [2]` (only element 0 used; CdIntToPos's output and every
 *    CdControlB mode-6/9 call's own param, always addressed at its OWN
 *    offset 0 — Ghidra's `&aCStack_30[0].minute` is offset 0 of the
 *    struct, i.e. just `(u8 *)loc`, not `.second`).
 *
 * Matching notes:
 *  - `dst`/`curSector`/`remaining`/`off` (Ghidra's puVar8/iVar2/iVar7/
 *    iVar6) are (re)initialised ONCE, right after `CdIntToPos`, straight
 *    line, before either retry loop (mode 0xE then mode 6) — Ghidra's own
 *    rendering nests them inside the mode-6 retry `do {} while`, which is
 *    a decompiler placement artifact (cookbook: "trust the assembly over
 *    Ghidra's statement order").
 *  - ONE retry destination, not two: BOTH `CdReady` reporting not-ready
 *    AND `CdGetSector` failing restart from the exact same point
 *    (`full_retry:` — re-`CdIntToPos` with the ORIGINAL `sector`
 *    parameter, mode 0xE seek again, full cursor reinit), discarding any
 *    progress made on a partially-successful multi-sector read. Ghidra's
 *    two differently-shaped `goto`s (and an earlier draft of this header)
 *    read as two distinct destinations because the compiler fills the
 *    `CdGetSector`-fail branch's OWN delay slot with the shared prefix
 *    instruction (`li v0,0xA0`) instead of jumping to where the
 *    `CdReady`-fail path materialises it as a real instruction — same
 *    target label either way (see STATUS above).
 *  - `src = data + off;` sits as the FIRST statement inside the copy
 *    loop's body, not immediately before it — even though it is loop
 *    invariant (recomputes the same value every iteration) and `loop.c`
 *    hoists it straight back out. Writing it before the loop as a normal
 *    pre-loop statement compiles to the identical hoisted instruction but
 *    picks the WRONG delay-slot filler for the loop's entry test (found
 *    by permuter, see STATUS above; cookbook: "loop.c hoisting is a
 *    threshold economy").
 */

extern int VSync(int mode);

void FUN_8005f380(u8 *buffer, s32 sector, s32 byteOffset, s32 length)
{
    u8 sectorBuf[0x810];
    u8 param[8];
    CdlLOC loc[2];
    u8 *raw;
    u8 *data;
    u8 *dst;
    u8 *src;
    s32 curSector;
    s32 remaining;
    s32 off;
    s32 n;
    s32 vsyncArg;
    s32 chunk;
    s32 i;

    raw = sectorBuf;
    data = sectorBuf + 0xC;

    if (length < 1) return;

full_retry:
    param[0] = 0xA0;
    CdIntToPos(sector, loc);

    dst = buffer;
    remaining = length;
    curSector = sector;
    off = byteOffset;

    while (CdControlB(0xE, param, 0) == 0) {
        VSync(0);
    }

    vsyncArg = 3;
    do {
        VSync(vsyncArg);
        n = CdControlB(6, (u8 *)loc, 0);
        vsyncArg = 0;
    } while (n == 0);

    while (remaining >= 1) {
        n = CdReady(0, 0);
        if (n != 1) goto full_retry;
        n = CdGetSector(sectorBuf, 0x203);
        if (n == 0) goto full_retry;

        n = CdPosToInt((CdlLOC *)raw);
        if (n != curSector) {
            CdIntToPos(curSector, loc);
            while (CdControlB(6, (u8 *)loc, 0) == 0) {
                VSync(0);
            }
        } else {
            chunk = off + remaining;
            if (chunk > 0x800) chunk = 0x800;
            chunk = chunk - off;
            for (i = 0; i < chunk; i++) {
                src = data + off;
                dst[i] = src[i];
            }
            dst = dst + chunk;
            remaining = remaining - chunk;
            off = 0;
            curSector = curSector + 1;
        }
    }

    while (CdControlB(9, (u8 *)loc, 0) == 0) {
        VSync(0);
    }
}
