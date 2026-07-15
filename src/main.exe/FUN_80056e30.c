#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

/*
 * FUN_80056e30 (0x80056e30, 0x94 bytes) — MEMCARD.C family: primes the card
 * system with the standard ChkCard.c/LoadCard.c/DeleteCard.c boilerplate
 * (MemCardAccept(0) then MemCardSync to block for the result), builds a
 * memory-card path exactly like DeleteCard.c ("%s%s" of the volume-id
 * prefix D_80097D04 and the caller's `name`), opens it via MemCardOpen in
 * mode 1, blocks on MemCardSync again, and — unlike DeleteCard/LoadCard,
 * which go on to read/delete the file — immediately closes it again with
 * no read/write in between if the open failed (`result == 0`). That shape
 * (open, sync, close-on-failure, return the raw open result truncated to a
 * short, no data transfer at all) reads as a plain "does this save file
 * exist on the card" probe. Called only by (still-asm) FUN_8005a7a4.
 *
 * The Ghidra `__override__prt_80056e6c_aee7b64a` split (cookbook's
 * "Toolchain gotchas") is a call-site prototype marker for the `jal
 * sprintf` inside, not a jump table — one C function produces both pieces.
 *
 * D_80097D04/D_80097D08 and their types are proven by DeleteCard.c (the
 * same TU, same "%s%s" sprintf idiom); MemCardOpen/MemCardClose have no
 * prototype elsewhere in the game yet, so declared minimally from the raw
 * a0-a3/`jal` call sites (MemCardOpen(chan, path, mode); MemCardClose()
 * takes no visible args and its result is unused).
 *
 * TENCHU_ID/CardPathFormat (0x80097D04/0x80097D08) are bound under
 * fresh names in config/symbols.main.exe.txt, not the splat-auto
 * D_80097D04/D_80097D08: converting this function away from raw asm
 * removed the last raw-bytes anchor for those two addresses, so splat's
 * auto-symbol table started deriving them from a drifted accumulation
 * elsewhere (+4 bytes each) — see DeleteCard.c, the other consumer, fixed
 * the same way.
 */

extern char *TENCHU_ID;  /* -> the volume-id prefix string */
extern char CardPathFormat[]; /* "%s%s" style path format */

extern int sprintf(char *buf, char *fmt, ...);
extern s32 MemCardAccept(s32 chan);
extern s32 MemCardOpen(s32 chan, char *path, s32 mode);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);
extern void MemCardClose(void);

s16 FUN_80056e30(char *name)
{
    char path[200];
    s32 cmd;
    s32 result;
    s32 cmd2;
    s32 result2;

    result = MemCardAccept(0);
    MemCardSync(0, &cmd, &result);
    sprintf(path, CardPathFormat, TENCHU_ID, name);
    result2 = MemCardOpen(0, path, 1);
    MemCardSync(0, &cmd2, &result2);
    if (result2 == 0) {
        MemCardClose();
    }
    return result2;
}
