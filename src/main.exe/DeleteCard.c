#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DeleteCard(unsigned char *name);
 *     MEMCARD.C:95, 10 src lines, frame 232 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned char * name
 *     stack sp+216    long cmd
 *     stack sp+220    long result
 *     stack sp+16     unsigned char [200] fn
 * END PSX.SYM */

/*
 * DeleteCard (0x80056ec4, 0x54 bytes) — builds the save file's full memory-card
 * path into a 200-byte stack buffer ("<region-prefix><name>", the prefix being
 * the "BISLPS-01901" style volume id held in the D_80097D04 pointer) and asks
 * the card to delete it, blocking on MemCardSync as ChkCard.c/FormatCard.c do.
 *
 * `result` is address-taken (MemCardSync writes it back), so cc1 reloads it from
 * the stack for the return — hence the `lh` rather than a register truncation.
 * D_80097D04 is %gp_rel (a small in the gp window); D_80097D08, the format
 * string, is reached absolutely.
 *
 * Bound under fresh names (CardVolumeIdPtr/CardPathFormat) in
 * config/symbols.main.exe.txt instead of the splat-auto D_80097D04/
 * D_80097D08: once FUN_80056e30 (the same TU, same "%s%s" idiom) stopped
 * being raw asm, splat's auto-symbol table lost its only remaining
 * raw-bytes anchor for these two addresses and started deriving them from
 * a drifted accumulation elsewhere (+4 bytes each) — the same "matching a
 * function can delete the very D_ symbol its C body needs" class in
 * docs/matching-cookbook.md. Bind fresh, non-colliding names at the
 * correct addresses instead of re-using the (now unreliable) auto name.
 */

extern char *CardVolumeIdPtr;  /* -> the volume-id prefix string */
extern char CardPathFormat[]; /* "%s%s" style path format */

extern int sprintf(char *buf, char *fmt, ...);
extern s32 MemCardDeleteFile(s32 chan, char *path);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s16 DeleteCard(char *name)
{
    char path[200];
    s32 cmd;
    s32 result;

    sprintf(path, CardPathFormat, CardVolumeIdPtr, name);
    result = MemCardDeleteFile(0, path);
    MemCardSync(0, &cmd, &result);
    return result;
}
