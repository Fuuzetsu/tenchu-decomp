#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "memcard.h"
#include "vmemory.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * LoadSI(int target, unsigned char *name);
 *     INFOVIEW.C:618, 53 src lines, frame 8440 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int target
 *     param $s0       unsigned char * name
 *     reg   $s1       void * ret
 *     stack sp+24     unsigned char [200] fn
 *     reg   $s2       unsigned char * msg
 *     stack sp+8416   long cmd
 *     stack sp+8420   long result
 *     stack sp+224    unsigned char [8192] block
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *ImagePath;
 *     extern unsigned char *CID;
 *     extern int StageID;
 * END PSX.SYM */

extern char fmt_concat[];          /* "%s%s" */
extern char fmt_card_name[];       /* "%s%d_%s" */
extern char msg_file_read_error[]; /* "file read error" */
extern char msg_card_error[];      /* "card error %d" */


void *LoadSI(enum save_storage storage, u8 *name)
{
    void *ret;
    char *msg;
    u8 fn[200];
    MemoryCardFileBlock block;
    s32 cmd;
    enum card_result result;
    enum memcard_channel channel;

    if (storage == SAVE_STORAGE_DISK)
    {
        sprintf(fn, fmt_concat, ImagePath, name);
        ret = FileRead(fn);
    }
    else
    {
        channel = MEMCARD_CHANNEL_0;
        msg = 0;
        ret = valloc(BLOCKSIZE);
        MemCardAccept(channel);
        MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
        if (result != CARD_RESULT_SUCCESS && result != CARD_RESULT_NEW_CARD)
        {
            msg = msg_card_error;
            vfree(ret);
            ret = 0;
        }
        else
        {
            sprintf(fn, fmt_card_name, CID, StageID, name);
            MemCardReadFile(channel, (char *)fn, &block, 0,
                            sizeof(block));
            MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
            if (result != CARD_RESULT_SUCCESS)
            {
                msg = msg_file_read_error;
                vfree(ret);
                ret = 0;
            }
            else
            {
                memcpy(ret, block.payload, sizeof(block.payload));
            }
        }
        if (msg != 0)
        {
            AdtMessageBox(msg, result);
        }
    }
    return ret;
}
