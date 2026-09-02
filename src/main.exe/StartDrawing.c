#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StartDrawing(void);
 *     3DCTRL.C:140, 6 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short DrawingPage;
 *     extern unsigned char Packet[2][65536];
 *     extern struct GsOT OTable[2];
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 * END PSX.SYM */

/* Leave only the PSX.SYM-proven outer bound incomplete: this retains the
 * absolute symbol access while preserving the 64 KiB page type. */
extern u8 Packet[][PACKET_PAGE_SIZE];

void StartDrawing(void)
{
    short newPage;

    newPage = 1 - DrawingPage;
    DrawingPage = newPage;
    GsSetWorkBase(Packet[newPage]);

    OTablePt = &OTable[DrawingPage];
    GsClearOt(0, 0, OTablePt);

    GameClock++;
}
