#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DemoPatchInit(void);
 *     INFOVIEW.C:1196, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     stack sp+16     struct RECT rc
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char DemoBackupArea[64];
 * END PSX.SYM */

/*
 * DemoPatchInit (0x8004c044) — snapshot a small VRAM rect (x=0x3f0,y=0x1ff,
 * w=0x10,h=1 — a single 16-pixel row, i.e. a 16-colour CLUT) into
 * DemoBackupArea via the PSYQ libgpu StoreImage2, then DrawSync(0) waits for
 * the transfer to finish.
 */
extern u8 DemoBackupArea[64];

void DemoPatchInit(void)
{
    RECT rc;

    rc.x = 0x3f0;
    rc.y = 0x1ff;
    rc.w = 0x10;
    rc.h = 1;
    StoreImage2(&rc, (u_long *)DemoBackupArea);
    DrawSync(0);
}
