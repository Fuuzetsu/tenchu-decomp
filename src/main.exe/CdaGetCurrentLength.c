#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int CdaGetCurrentLength(void);
 *     OPAUDIO.C:29, 13 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCdaStatus CdaStatus;
 * END PSX.SYM */

/*
 * CdaGetCurrentLength (0x8004fb80, 0x64 bytes) — returns the elapsed
 * CD-audio playback length (CurPos - StartPos) once the drive is ready;
 * -1 if CdaReady() reports not-ready, or 1 (a truthy placeholder) if
 * playback isn't active (flag bit0 clear). Same proven TCdaStatus struct
 * as CdaStop.c/FUN_8004fbf4.c/FUN_8004fc08.c.
 *
 * Matching notes: write as three independent early returns, not Ghidra's
 * single shared `iVar1` funneled through one `return iVar1;`. Each arm's
 * result really computes straight into $v0 at its own tail (the inactive
 * arm's `addiu $v0,$zero,1` even lands in the outer guard branch's delay
 * slot); a shared `iVar1` variable forces a return-copy-chain that
 * coalesces the literal `1` used both as the default value AND inside the
 * `& 1` mask test into ONE pseudo, turning the mask test's `andi` into a
 * register-register `and` (cookbook's "mask literal spelling" rule, but
 * triggered by an inadvertent CSE merge rather than immediate-width).
 */

extern TCdaStatus CdaStatus;
extern s32 CdaReady(void);

int CdaGetCurrentLength(void)
{
    if ((CdaStatus.flag & 1) == 0) {
        return 1;
    }
    if (CdaReady() == 0) {
        return -1;
    }
    return CdaStatus.CurPos - CdaStatus.StartPos;
}
