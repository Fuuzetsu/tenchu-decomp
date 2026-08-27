#include "common.h"
#include "main.exe.h"
#include "infoview.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SaveSI(int target, unsigned char *name, void *mem, long size);
 *     INFOVIEW.C:525, 89 src lines, frame 8488 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
    u8 bytes[0x10];
} SaveSIUnalignedChunk;

typedef struct
{
    u32 words[4];
} SaveSIAlignedChunk;

/* The icon loops keep distinct potentially-unaligned and aligned chunk
 * types so cc1 selects the retail load/store forms and register allocation.
 * A single fixed-size built-in copy changes that allocation in this larger
 * function; Clut's short copy does not need the distinction. */

extern char D_80097D90[];
extern char D_80097D98[];
extern char D_80014104[];
extern char D_80014114[];
extern char D_80014128[];
extern char D_80014134[];
extern char D_80014144[];
extern char D_80014158[];
extern char D_80014168[];
extern char D_80014178[];
extern char D_80014190[];

extern void *memcpy(void *dst, const void *src, u32 size);
extern int sprintf(char *buf, char *fmt, ...);
extern s32 PCcreat(char *name, s32 mode);
extern s32 PCwrite(s32 fd, void *data, s32 size);
extern s32 PCclose(s32 fd);
extern s32 MemCardAccept(s32 chan);
extern s32 MemCardCreateFile(s32 chan, char *name, s32 blocks);
extern s32 MemCardWriteFile(s32 chan, char *name, void *data, s32 offset,
                            s32 size);
extern s32 MemCardFormat(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);
extern s32 AdtSelect(char *title, TAdtSelect *choices, s32 mode);
extern void AdtMessageBox(char *fmt, ...);

void SaveSI(s32 target, u8 *name, void *mem, s32 size)
{
    s32 fd;
    char *msg;
    u8 fn[200];
    u8 block[BLOCKSIZE];
    TAdtSelect sel[3];
    s32 cmd;
    s32 result;
    s32 chan;
    TCardHeader *hd;
    void *data;

    if (target == 0)
    {
        sprintf(fn, D_80097D90, ImagePath, name);
        fd = PCcreat(fn, 0);
        if (fd == -1)
        {
            AdtMessageBox(D_80014104, fn);
            return;
        }
        PCwrite(fd, mem, size);
        PCclose(fd);
        return;
    }

    msg = 0;
    chan = 0;
    hd = (TCardHeader *)block;
    data = block + sizeof(TCardHeader);
    if ((u32)size > BLOCKSIZE - sizeof(TCardHeader))
    {
        AdtMessageBox(D_80014114);
        goto done;
    }
    {
        u8 *icon3;
        u8 *icon2;
        u8 *icon1;
        u8 *src;
        u8 *dst;
        s32 alignment;
        s32 end;
        s32 *cmdp;
        s32 *resultp;

        hd->Magic[0] = 0x53;
        hd->Magic[1] = 0x43;
        hd->Type = 0x13;
        hd->BlockEntry = 1;
        sprintf(hd->Title, D_80014128, StageID + 1, name);

        icon1 = (u8 *)GetArcData(0x16);
        icon2 = (u8 *)GetArcData(0x17);
        icon3 = (u8 *)GetArcData(0x18);
        __builtin_memcpy(hd->Clut, icon1 + 0x14, sizeof(hd->Clut));
        dst = hd->Icon[0];
        src = icon1 + 0x40;
        alignment = (u32)src & 3;
        if (alignment)
        {
            do
            {
                *(SaveSIUnalignedChunk *)dst =
                    *(SaveSIUnalignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while (src != icon1 + 0xc0);
        }
        else
        {
            do
            {
                *(SaveSIAlignedChunk *)dst = *(SaveSIAlignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while (src != icon1 + 0xc0);
        }

        dst = hd->Icon[1];
        src = icon2 + 0x40;
        alignment = (u32)src & 3;
        if (alignment)
        {
            do
            {
                *(SaveSIUnalignedChunk *)dst =
                    *(SaveSIUnalignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while (src != icon2 + 0xc0);
        }
        else
        {
            do
            {
                *(SaveSIAlignedChunk *)dst = *(SaveSIAlignedChunk *)src;
                src += 0x10;
                dst += 0x10;
            } while (src != icon2 + 0xc0);
        }

        dst = hd->Icon[2];
        src = icon3 + 0x40;
        end = (s32)icon3 + 0xc0;
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

        if (icon3 != 0)
        {
            src = (u8 *)chan;
        }
        else
        {
            src = (u8 *)chan;
        }
        MemCardAccept((s32)src);
        cmdp = &cmd;
        resultp = &result;
        MemCardSync(0, cmdp, resultp);
        if (result == 0 || result == 3)
        {
            goto create_file;
        }
        if (result == 4)
        {
            __builtin_memcpy(sel, D_800140A8, sizeof(sel));
            if (msg != 0)
            {
                do
                {
                    src = (u8 *)D_80014134;
                } while (0);
                dst = (u8 *)sel;
                end = AdtSelect((char *)src, (TAdtSelect *)dst, 1);
            }
            else
            {
                do
                {
                    src = (u8 *)D_80014134;
                } while (0);
                dst = (u8 *)sel;
                end = AdtSelect((char *)src, (TAdtSelect *)dst, 1);
            }
            if (end == 0)
            {
                msg = D_80014144;
            }
            else
            {
                MemCardFormat(chan);
                MemCardSync(0, cmdp, resultp);
                if (result == 0)
                {
                    goto create_file;
                }
                msg = D_80014158;
            }
        }
        else
        {
            msg = D_80014168;
        }
        goto done;

create_file:
        sprintf(fn, D_80097D98, CID, StageID, name);
        src = (u8 *)chan;
        if (msg != 0)
        {
            dst = fn;
        }
        else
        {
            dst = fn;
        }
        alignment = MemCardCreateFile((s32)src, (char *)dst, 1);
        result = alignment;
        if (alignment != 0 && alignment != 6)
        {
            msg = D_80014178;
            goto done;
        }
        memcpy(data, mem, size);
        MemCardWriteFile(chan, fn, block, 0, BLOCKSIZE);
        MemCardSync(0, &cmd, &result);
        if (result != 0)
        {
            msg = D_80014190;
        }
    }

done:
    if (msg != 0)
    {
        AdtMessageBox(msg, result);
    }
}
