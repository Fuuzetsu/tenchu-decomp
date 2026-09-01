#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/*
 * check_card_file_ (0x80056e30, 0x94 bytes) — MEMCARD.C family: primes the card
 * system with the standard ChkCard.c/LoadCard.c/DeleteCard.c boilerplate
 * (MemCardAccept(0) then MemCardSync to block for the result), builds a
 * memory-card path exactly like DeleteCard.c ("%s%s" of the volume-id
 * prefix TENCHU_ID and the caller's `name`), opens it via MemCardOpen in
 * mode 1, blocks on MemCardSync again, and — unlike DeleteCard/LoadCard,
 * which go on to read/delete the file — immediately closes it again with
 * no read/write in between when the sync succeeds
 * (`acceptResult == CARD_RESULT_SUCCESS`). That shape
 * (open, sync, close-on-failure, return the sync's out-value truncated
 * to a short — the open's return is only a seed MemCardSync overwrites —
 * no data transfer at all) reads as a plain "does this save file
 * exist on the card" probe. Called only by update_card_screen_.
 *
 * The Ghidra `__override__prt_80056e6c_aee7b64a` split (cookbook's
 * "Toolchain gotchas") is a call-site prototype marker for the `jal
 * sprintf` inside, not a jump table — one C function produces both pieces.
 *
 * TENCHU_ID/CardPathFormat and their types are proven by DeleteCard.c (the
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

extern char CardPathFormat[]; /* "%s%s" style path format */

extern int sprintf(char *buf, char *fmt, ...);
extern s32 MemCardAccept(s32 chan);
extern s32 MemCardOpen(s32 chan, char *path, s32 mode);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);
extern void MemCardClose(void);

card_result check_card_file_(char *name)
{
    char path[200];
    s32 cmd;
    s32 result;
    /* A second sync pair, unlike every sibling's single reused cmd/result:
     * collapsing them onto one pair mismatches (separate stack slots are
     * retail's own). */
    s32 acceptCmd;
    s32 acceptResult;

    result = MemCardAccept(0);
    MemCardSync(0, &cmd, &result);
    sprintf(path, CardPathFormat, TENCHU_ID, name);
    acceptResult = MemCardOpen(0, path, 1);
    MemCardSync(0, &acceptCmd, &acceptResult);
    if (acceptResult == CARD_RESULT_SUCCESS)
    {
        MemCardClose();
    }
    return acceptResult;
}
