#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include "images.h"
#include "infoview.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SaveSI(int target, unsigned char *name, void *mem, long size);
 *     INFOVIEW.C:525, 89 src lines, frame 8488 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int target
 *     param $s5       unsigned char * name
 *     param $fp       void * mem
 *     param $s6       long size
 *     stack sp+24     unsigned char [200] fn
 *     reg   $s0       int fd
 *     reg   $s3       unsigned char * msg
 *     stack sp+8416   long cmd
 *     stack sp+8420   long result
 *     reg   $s4       long chan
 *     stack sp+224    unsigned char [8192] block
 *     reg   $s2       struct TCardHeader * hd
 *     reg   $s7       void * data
 *     reg   $s2       struct TCardHeader * hd
 *     reg   $t1       unsigned char * icon3
 *     reg   $s1       unsigned char * icon2
 *     reg   $s0       unsigned char * icon1
 *     stack sp+8424   struct TAdtSelect [3] sel
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *ImagePath;
 *     extern int StageID;
 *     extern unsigned char *CID;
 * END PSX.SYM */

typedef struct
{
    u32 word0;
    u32 word1;
    u32 word2;
    u32 word3;
} SaveSIAlignedChunk;

typedef struct
{
    u32 word0;
    u32 word1;
    u32 word2;
    u32 word3;
} __attribute__((packed)) SaveSIUnalignedChunk;

/* The two branches distinguish aligned and potentially unaligned sources. */

extern char fmt_concat[];         /* "%s%s" */
extern char fmt_card_name[];      /* "%s%d_%s" */
extern char msg_open_error[];     /* "open error %s" */
extern char msg_size_too_large[]; /* "file size too large" */
extern char fmt_save_title[];     /* "STAGE %d %s" */
extern char msg_format_card[];    /* "format card?" */
extern char msg_not_formatted[];  /* "card not formated" (sic) */
extern char msg_card_damaged[];   /* "card damaged" */
extern char msg_card_error[];     /* "card error %d" */
extern char msg_create_error[];   /* "file create error %d" */
extern char msg_write_error[];    /* "file write error %d" */

void SaveSI(enum save_storage storage, u8 *name, void *mem, s32 size)
{
    s32 fd;
    char *msg;
    u8 fn[200];
    MemoryCardFileBlock block;
    TAdtSelect sel[3];
    s32 cmd;
    enum card_result result;
    enum memcard_channel chan;
    TCardHeader *hd;
    void *data;

    if (storage == SAVE_STORAGE_DISK)
    {
        sprintf(fn, fmt_concat, ImagePath, name);
        fd = PCcreat(fn, 0);
        if (fd == -1)
        {
            AdtMessageBox(msg_open_error, fn);
            return;
        }
        PCwrite(fd, mem, size);
        PCclose(fd);
        return;
    }

    msg = 0;
    chan = MEMCARD_CHANNEL_0;
    hd = &block.header;
    data = block.payload;
    if ((u32)size > sizeof(block.payload))
    {
        AdtMessageBox(msg_size_too_large);
    }
    else
    {
        u8 *icon3;
        u8 *icon2;
        u8 *icon1;
        u8 *src;
        u8 *dst;
        s32 alignment;
        enum card_result create_result;
        s32 end;

        /* The block header a PSX memory card expects. */
        hd->Magic[0] = 'S';
        hd->Magic[1] = 'C';
        hd->Type = SAVE_ICON_3_FRAMES;
        hd->BlockEntry = CARD_FILE_BLOCKS;
        sprintf(hd->Title, fmt_save_title, STAGE_NUMBER(StageID), name);

        icon1 = GetArcData(ICON_CARD1);
        icon2 = GetArcData(ICON_CARD2);
        icon3 = GetArcData(ICON_CARD3);
        __builtin_memcpy(hd->Clut, CARD_ICON_TIM_CLUT(icon1),
                         sizeof(hd->Clut));
        dst = hd->Icon[0];
        src = CARD_ICON_TIM_PIXELS(icon1);
        alignment = (u32)src & 3;
        if (alignment)
        {
            do
            {
                *(SaveSIUnalignedChunk *)dst =
                    *(SaveSIUnalignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while (src != CARD_ICON_TIM_END(icon1));
        }
        else
        {
            do
            {
                *(SaveSIAlignedChunk *)dst = *(SaveSIAlignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while (src != CARD_ICON_TIM_END(icon1));
        }

        dst = hd->Icon[1];
        src = CARD_ICON_TIM_PIXELS(icon2);
        alignment = (u32)src & 3;
        if (alignment)
        {
            do
            {
                *(SaveSIUnalignedChunk *)dst =
                    *(SaveSIUnalignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while (src != CARD_ICON_TIM_END(icon2));
        }
        else
        {
            do
            {
                *(SaveSIAlignedChunk *)dst = *(SaveSIAlignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while (src != CARD_ICON_TIM_END(icon2));
        }

        dst = hd->Icon[2];
        src = CARD_ICON_TIM_PIXELS(icon3);
        end = (s32)CARD_ICON_TIM_END(icon3);
        alignment = (u32)src & 3;
        if (alignment)
        {
            do
            {
                *(SaveSIUnalignedChunk *)dst =
                    *(SaveSIUnalignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while ((s32)src != end);
        }
        else
        {
            do
            {
                *(SaveSIAlignedChunk *)dst = *(SaveSIAlignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while ((s32)src != end);
        }

        MemCardAccept(chan);
        MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
        if (result != CARD_RESULT_SUCCESS && result != CARD_RESULT_NEW_CARD)
        {
            if (result == CARD_RESULT_UNFORMATTED)
            {
                __builtin_memcpy(sel, sel_okcancel, sizeof(sel));
                end = AdtSelect(msg_format_card, sel, 1);
                if (end == 0)
                {
                    msg = msg_not_formatted;
                }
                else
                {
                    MemCardFormat(chan);
                    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
                    if (result == CARD_RESULT_SUCCESS)
                    {
                        goto create_file;
                    }
                    msg = msg_card_damaged;
                }
            }
            else
            {
                msg = msg_card_error;
            }
            goto done;
        }

    create_file:
        sprintf(fn, fmt_card_name, CID, StageID, name);
        src = (u8 *)chan;
        if (msg != 0)
        {
            dst = fn;
        }
        else
        {
            dst = fn;
        }
        create_result = MemCardCreateFile((enum memcard_channel)src,
                                          (char *)dst, CARD_FILE_BLOCKS);
        result = create_result;
        if (create_result != CARD_RESULT_SUCCESS &&
            create_result != CARD_RESULT_FILE_EXISTS)
        {
            msg = msg_create_error;
        }
        else
        {
            memcpy(data, mem, size);
            MemCardWriteFile(chan, (char *)fn, &block, 0, sizeof(block));
            MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
            if (result != CARD_RESULT_SUCCESS)
            {
                msg = msg_write_error;
            }
        }
    }

done:
    if (msg != 0)
    {
        AdtMessageBox(msg, result);
    }
}
