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

extern u8 DemoBackupArea[64];

void DemoPatchInit(void)
{
    RECT rc;

    setRECT(&rc, 0x3f0, 0x1ff, 0x10, 1);
    StoreImage2(&rc, (u_long *)DemoBackupArea);
    DrawSync(0);
}
