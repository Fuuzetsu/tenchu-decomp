#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short FormatCard(void);
 *     MEMCARD.C:108, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     long cmd
 *     stack sp+20     long result
 * END PSX.SYM */

card_result FormatCard(void)
{
    s32 cmd;
    enum card_result result;

    result = MemCardFormat(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    return result;
}
