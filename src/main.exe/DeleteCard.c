#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DeleteCard(unsigned char *name);
 *     MEMCARD.C:95, 10 src lines, frame 232 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * name
 *     stack sp+216    long cmd
 *     stack sp+220    long result
 *     stack sp+16     unsigned char [200] fn
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

/*
 * DeleteCard (0x80056ec4, 0x54 bytes) — builds the save file's full memory-card
 * path into a 200-byte stack buffer ("<region-prefix><name>", the prefix being
 * the "BISLPS-01901" style volume id held in the TENCHU_ID pointer) and asks
 * the card to delete it, blocking on MemCardSync as ChkCard.c/FormatCard.c do.
 *
 * `result` is address-taken (MemCardSync writes it back), so cc1 reloads it from
 * the stack for the return — hence the `lh` rather than a register truncation.
 * TENCHU_ID is %gp_rel (a small in the gp window); CardPathFormat, the format
 * string, is reached absolutely.
 *
 * Bound under fresh names (TENCHU_ID/CardPathFormat) in
 * config/symbols.main.exe.txt instead of the splat-auto D_80097D04/
 * D_80097D08: once check_card_file_ (the same TU, same "%s%s" idiom) stopped
 * being raw asm, splat's auto-symbol table lost its only remaining
 * raw-bytes anchor for these two addresses and started deriving them from
 * a drifted accumulation elsewhere (+4 bytes each) — the same "matching a
 * function can delete the very D_ symbol its C body needs" class in
 * docs/matching-cookbook.md. Bind fresh, non-colliding names at the
 * correct addresses instead of re-using the (now unreliable) auto name.
 */

extern char CardPathFormat[]; /* "%s%s" style path format */

extern int sprintf(char *buf, char *fmt, ...);
extern s32 MemCardDeleteFile(s32 chan, char *path);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

s16 DeleteCard(u8 *name)
{
    u8 fn[200];
    s32 cmd;
    s32 result;

    sprintf((char *)fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardDeleteFile(0, (char *)fn);
    MemCardSync(0, &cmd, &result);
    return result;
}
