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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
