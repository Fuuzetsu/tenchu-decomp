#include "common.h"
#include "main.exe.h"

/*
 * FormatCard — MemCardFormat on slot 0, then block on MemCardSync and return
 * its result truncated to a short. Structurally identical to ChkCard.c (see
 * that file for the shared-stack-slot note); only the kick-off call differs.
 */

extern s32 MemCardFormat(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s16 FormatCard(void)
{
    s32 cmd;
    s32 result;

    result = MemCardFormat(0);
    MemCardSync(0, &cmd, &result);
    return result;
}
