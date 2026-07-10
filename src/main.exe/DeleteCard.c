#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DeleteCard(unsigned char *name);
 *     MEMCARD.C:95, 10 src lines, frame 232 bytes, saved-reg mask 0x80000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
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
 */

extern char *D_80097D04;  /* -> the volume-id prefix string */
extern char D_80097D08[]; /* "%s%s" style path format */

extern int sprintf(char *buf, char *fmt, ...);
extern s32 MemCardDeleteFile(s32 chan, char *path);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s16 DeleteCard(char *name)
{
    char path[200];
    s32 cmd;
    s32 result;

    sprintf(path, D_80097D08, D_80097D04, name);
    result = MemCardDeleteFile(0, path);
    MemCardSync(0, &cmd, &result);
    return result;
}
