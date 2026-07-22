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

/*
 * InitPadControl (0x8001b364, 0xa0 bytes) — bring up the memory card and pad
 * libraries: MemCardInit/MemCardStart, zero the shared ComBuf[2][34] comm
 * scratch and the PadPort[2][4] state table, PadInitDirect over the two
 * 34-byte ComBuf rows, PadStartCom, then — only when the persistent-state
 * `analog_pad_present` bit (byte 0x47; see FUN_8001b2b8.c) is set — burn 15
 * VSyncs and force
 * PadSetMainMode(0,1,0) (digital mode) once.
 *
 * The raw `.s` disproves Ghidra's own `ComBuf[0x11]` second PadInitDirect
 * argument (an out-of-bounds outer-dimension index into `[2][34]`): the
 * actual encoding is `addiu $a1,$a0,0x22` off the SAME base just moved into
 * $a0 for the first argument — plain `ComBuf[1]` (offset 0x22 = 34, the row
 * size), not a flattened byte index.
 *
 * The one-time member read is a plain absolute scalar load (lui+lbu, no
 * reuse across branches), while FUN_8001b2b8.c retains a local TLinkInfo
 * pointer for its two-arm update.
 */
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
    memset(ComBuf[0], 0, 0x44);
    memset(PadPort, 0, 0x70);
    PadInitDirect(ComBuf[0], ComBuf[1]);
    PadStartCom();
    if ((((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)->analog_pad_present &
         1) != 0) {
        i = 0xf;
        do {
            VSync(0);
            i = i - 1;
        } while (i > 0);
        PadSetMainMode(0, 1, 0);
    }
}
