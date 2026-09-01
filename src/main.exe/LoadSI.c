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

/*
 * LoadSI (0x8005c2c4, 0x140 bytes total: entry + 2 override pieces) — the
 * debug-menu file loader: disk storage loads a whole-stage resource by name
 * from the CD/host filesystem (FileRead); card storage loads it from the
 * memory card (BISLPS_00000-prefixed save slot), staging through an 8KB
 * MemoryCardFileBlock and copying its payload member into the returned
 * buffer.
 *
 * The two error arms spell their cleanup independently. jump2 merges those
 * tails back together, while the earlier CSE pass consequently treats the
 * two MemCardSync stack-address pairs as separate expressions and
 * rematerializes them at each call. The one-shot loop around `msg = 0`
 * retains the target's shared $s1 zero/message lifetime and scheduling.
 *
 * Matching notes:
 *  - Register mapping in PSX.SYM ($s0=name, $s1=ret) is the DEMO build's —
 *    retail's asm caches `name` in $s2 (moved out of $a1 at function entry,
 *    before $a1 is reused for the first sprintf's format arg) and the
 *    valloc'd/returned buffer in $s0. Only the local COUNT/TYPES carry over;
 *    one `ret` variable serves BOTH return paths (FileRead's result on the
 *    disk path, valloc's buffer otherwise) — matches PSX.SYM's 6 named
 *    locals (ret/fn/msg/cmd/result/block) with no extra `puVar1`.
 *  - `block` is ONE 8192-byte MemoryCardFileBlock, not Ghidra's two
 *    overlapping locals (auStack_2018/auStack_1e18): MemCardReadFile fills
 *    the complete record and the payload copy starts at its named +0x200
 *    member — cookbook "Stack objects": the apparent locals are the header
 *    and payload of one file block.
 *  - Stack locals declared in ADDRESS order (fn@0x18, block@0xE0, cmd@0x20E0,
 *    result@0x20E4) to reproduce the 0x20F8 frame exactly.
 *  - `CID` is this TU's gp small (an `unsigned char *` POINTER variable,
 *    not an array — the asm `lw a2,%gp_rel(CID)($gp)` loads its
 *    VALUE); Build.hs maspsxGpExterns / tools/gpsyms.py confirm it's the
 *    only %gp_rel symbol in this function.
 *  - fmt_concat/fmt_card_name sit in the same INFOVIEW.C string-table run as
 *    SelectCameraOwnerOption's fmt_num_2 (fixed there as fmt_num_2);
 *    once that anchor was bound, splat's whole auto-name chain for this
 *    region resolved correctly (verified against the .map) — no separate
 *    fix needed for these two.
 */
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
    /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
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
