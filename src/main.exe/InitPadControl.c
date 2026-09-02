#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitPadControl(void);
 *     PADCMD.C:82, 13 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char ComBuf[2][34];
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

extern void MemCardInit(int unit);
extern void MemCardStart(void);
extern void PadInitDirect(void *buf1, void *buf2);
extern void PadStartCom(void);
extern void PadSetMainMode(int port, int mode, int lock);
extern int VSync(int mode);
extern void *memset(void *s, int c, u32 n);

void InitPadControl(void)
{
    int i;

    MemCardInit(0);
    MemCardStart();
    memset(ComBuf[0], 0, sizeof(ComBuf));
    memset(PadPort, 0, sizeof(PadPort));
    PadInitDirect(ComBuf[0], ComBuf[1]);
    PadStartCom();
    if ((((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->analog_pad_present &
         1) != 0)
    {
        i = PAD_ANALOG_MODE_SWITCH_DELAY;
        do
        {
            VSync(0);
            i--;
        } while (i > 0);
        PadSetMainMode(PAD_PORT_1, PAD_MAIN_MODE_ANALOG,
                       PAD_MAIN_MODE_KEEP_LOCK);
    }
}
