#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void EndDrawing(short sync);
 *     3DCTRL.C:151, 48 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sync
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern unsigned char Packet[2][65536];
 *     extern short DrawingPage;
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT *OTablePt;
 *     extern struct GsFOGPARAM Fog;
 * END PSX.SYM */

/* Leave only the PSX.SYM-proven outer bound incomplete: this retains the
 * absolute symbol access while preserving the 64 KiB page type. */
extern u8 Packet[][PACKET_PAGE_SIZE];
extern u32 PacketUsed;
extern s32 time;

extern s32 VSync(s32 mode);

void EndDrawing(short sync)
{
    s16 sk;
    u32 t;
    u32 val;
    s32 dp;

    if ((GameClock % 30 == 0) && (SkipFrame == 0))
    {
        val = GsGetWorkBase() - Packet[DrawingPage];
        if (val > PACKET_PAGE_SIZE)
            PacketUsed = PACKET_PAGE_SIZE;
        else
            PacketUsed = val;
    }

    sk = SkipFrame;
    switch (sk)
    {
    case SKIPFRAME_NONE:
        if (VSync(1) > -sync * SCREEN_H - 10)
        {
            SkipFrame = SKIPFRAME_SKIPPED;
            return;
        }
        break;

    case SKIPFRAME_SKIPPED:
        t = sync;
        sync = t << 1;
        SkipFrame = 0;
        dp = sk - (u16)DrawingPage;
        DrawingPage = dp;
        OTablePt = &OTable[DrawingPage];
        break;

    case SKIPFRAME_AFTER_LOAD:
        SkipFrame = 0;
        break;
        }

        OTablePt->org[0x7FE] = OTablePt->org[DEPTH_LIMIT];

        if (sync <= 0)
        {
            DrawSync(0);
            VSync(-sync);
        }
        else
        {
            if (VSync(-1) - time < sync)
                VSync(sync);
            time = VSync(-1);
            ResetGraph(1);
        }

        GsSwapDispBuff();
        GsSortClear(Fog.rfc, Fog.gfc, Fog.bfc, OTablePt);
        GsDrawOt(OTablePt);
    }
