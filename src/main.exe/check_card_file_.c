#include "common.h"
#include "main.exe.h"
#include "memcard.h"

extern char CardPathFormat[]; /* "%s%s" style path format */

extern int sprintf(char *buf, char *fmt, ...);

card_result check_card_file_(char *name)
{
    char path[200];
    s32 cmd;
    enum card_result result;
    s32 acceptCmd;
    enum card_result acceptResult;

    result = MemCardAccept(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    sprintf(path, CardPathFormat, TENCHU_ID, name);
    acceptResult = MemCardOpen(MEMCARD_CHANNEL_0, path,
                               MEMCARD_OPEN_READ_ONLY);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &acceptCmd, &acceptResult);
    if (acceptResult == CARD_RESULT_SUCCESS)
    {
        MemCardClose();
    }
    return acceptResult;
}
