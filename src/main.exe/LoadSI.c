#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * LoadSI(int target, unsigned char *name);
 *     INFOVIEW.C:618, 53 src lines, frame 8440 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 * debug-menu file loader: target==0 loads a whole-stage resource by name
 * from the CD/host filesystem (FileRead); target!=0 loads it from the
 * memory card (BISLPS_00000-prefixed save slot), staging through an 8KB
 * scratch buffer and skipping the first 0x200-byte card-file header before
 * copying the real 0x1E00-byte payload into the returned buffer.
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
 *    target==0 path, valloc's buffer otherwise) — matches PSX.SYM's 6 named
 *    locals (ret/fn/msg/cmd/result/block) with no extra `puVar1`.
 *  - `block` is ONE 8192-byte buffer, not Ghidra's two overlapping locals
 *    (auStack_2018/auStack_1e18): MemCardReadFile fills all 0x2000 bytes,
 *    and the payload copy reads `block + 0x200` (skipping a 512-byte card
 *    header) — cookbook "Stack objects": overlapping Ghidra locals are one
 *    buffer plus a cast.
 *  - Stack locals declared in ADDRESS order (fn@0x18, block@0xE0, cmd@0x20E0,
 *    result@0x20E4) to reproduce the 0x20F8 frame exactly.
 *  - `CID` is this TU's gp small (a `char *` POINTER variable itself,
 *    not an array — the asm `lw a2,%gp_rel(CID)($gp)` loads its
 *    VALUE); Build.hs maspsxGpExterns / tools/gpsyms.py confirm it's the
 *    only %gp_rel symbol in this function.
 *  - D_80097D90/D_80097D98 sit in the same INFOVIEW.C string-table run as
 *    SelectCameraOwnerOption's D_80097D70 (fixed there as D_80097D70);
 *    once that anchor was bound, splat's whole auto-name chain for this
 *    region resolved correctly (verified against the .map) — no separate
 *    fix needed for these two.
 */
extern u8 *ImagePath;
extern s32 StageID;
extern char D_80097D90[];  /* "%s%s" */
extern char D_80097D98[];  /* "%s\%d\%s" */
extern char *CID;   /* "BISLPS_00000" */
extern char D_800141A4[];  /* "file read error" */
extern char D_80014168[];  /* "card error %d" */

extern u_long *FileRead(char *path);
extern void *valloc(u32 size);
extern void vfree(void *p);
extern s32 MemCardAccept(s32 chan);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);
extern s32 MemCardReadFile(s32 chan, char *name, void *buf, s32 mode, s32 size);
extern void *memcpy(void *dst, void *src, u32 n);
extern void sprintf(char *s, char *fmt, ...);
extern void AdtMessageBox(char *fmt, ...);

void *LoadSI(int target, u8 *name)
{
    void *ret;
    char *msg;
    u8 fn[200];
    u8 block[0x2000];
    s32 cmd;
    s32 result;

    if (target == 0)
    {
        sprintf(fn, D_80097D90, ImagePath, name);
        ret = FileRead(fn);
        goto return_result;
    }
    do
    {
        msg = 0;
    } while (0);
    ret = valloc(0x2000);
    MemCardAccept((s32)msg);
    MemCardSync((s32)msg, &cmd, &result);
    if (result != 0 && result != 3)
    {
        msg = D_80014168;
        vfree(ret);
        ret = 0;
        goto done;
    }
    sprintf(fn, D_80097D98, CID, StageID, name);
    MemCardReadFile(0, fn, block, 0, 0x2000);
    MemCardSync(0, &cmd, &result);
    if (result != 0)
    {
        msg = D_800141A4;
        vfree(ret);
        ret = 0;
        goto done;
    }
    memcpy(ret, block + 0x200, 0x1E00);
done:
    if (msg != 0)
    {
        AdtMessageBox(msg, result);
    }
return_result:
    return ret;
}
