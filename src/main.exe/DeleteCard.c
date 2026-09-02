#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DeleteCard(unsigned char *name);
 *     MEMCARD.C:95, 10 src lines, frame 232 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * name
 *     stack sp+216    long cmd
 *     stack sp+220    long result
 *     stack sp+16     unsigned char [200] fn
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

extern char CardPathFormat[]; /* "%s%s" style path format */

extern int sprintf(char *buf, char *fmt, ...);

card_result DeleteCard(u8 *name)
{
    u8 fn[200];
    s32 cmd;
    enum card_result result;

    sprintf((char *)fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardDeleteFile(MEMCARD_CHANNEL_0, (char *)fn);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    return result;
}
