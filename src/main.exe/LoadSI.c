#include "common.h"
#include "main.exe.h"
#include "memcard.h"

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

extern void *valloc(u32 size);
extern void vfree(void *p);
extern void *memcpy(void *dst, void *src, u32 n);
extern void sprintf(char *s, char *fmt, ...);
extern void AdtMessageBox(char *fmt, ...);

void *LoadSI(enum save_storage storage, u8 *name)
{
    void *ret;
    char *msg;
    u8 fn[200];
    MemoryCardFileBlock block;
    s32 cmd;
    enum card_result result;

    if (storage == SAVE_STORAGE_DISK)
    {
        sprintf(fn, fmt_concat, ImagePath, name);
        ret = FileRead(fn);
        goto return_result;
    }
    msg = 0;
    /* Empty loop retained for code layout; its original source construct is unknown. */
    do
    {
    } while (0);
    ret = valloc(BLOCKSIZE);
    MemCardAccept(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    if (result != CARD_RESULT_SUCCESS && result != CARD_RESULT_NEW_CARD)
    {
        msg = msg_card_error;
        vfree(ret);
        ret = 0;
        goto done;
    }
    sprintf(fn, fmt_card_name, CID, StageID, name);
    MemCardReadFile(MEMCARD_CHANNEL_0, (char *)fn, &block, 0,
                    sizeof(block));
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    if (result != CARD_RESULT_SUCCESS)
    {
        msg = msg_file_read_error;
        vfree(ret);
        ret = 0;
        goto done;
    }
    memcpy(ret, block.payload, sizeof(block.payload));
done:
    if (msg != 0)
    {
        AdtMessageBox(msg, result);
    }
return_result:
    return ret;
}
