#include "common.h"
#include "main.exe.h"

/*
 * ChkCard (0x8001b4bc region's memory-card TU) — kicks off a MemCardAccept on
 * slot 0 and blocks on MemCardSync until it reports a result, returning that
 * result truncated to a short. `cmd` is MemCardSync's command out-param, which
 * this caller ignores; `result` is seeded with MemCardAccept's return and then
 * overwritten in place by MemCardSync, which is why both share one stack slot
 * in source. FormatCard.c is the same function over MemCardFormat.
 */

extern s32 MemCardAccept(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s16 ChkCard(void)
{
    s32 cmd;
    s32 result;

    result = MemCardAccept(0);
    MemCardSync(0, &cmd, &result);
    return result;
}
