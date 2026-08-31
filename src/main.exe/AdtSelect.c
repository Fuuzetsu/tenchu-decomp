#include "common.h"
#include "main.exe.h"
#include "adt.h"

/*
 * AdtSelect (0x8005fecc, 776 bytes) — modal debug-menu mode widget:
 * waits for pad release, saves the display state into a 0x8090-byte frame
 * buffer, then draws the choice list (18 per page) and moves the cursor on
 * edge-detected pad input until confirm (pad & 0x820 -> current entry) or
 * cancel (pad & 0x40 -> last entry); returns the entry's value.
 *
 * STATUS: MATCHING — ordinary human-shaped C under the reused ADT object's
 * pinned GCC 2.8.0 compiler profile.
 *
 * AUTHORITATIVE UPDATE (2026-07-19): the long round-by-round investigation
 * below is retained as history, not as a rulebook. Its "fences are required",
 * "all C levers are closed", and "parked/impossible" conclusions are
 * superseded by fresh source and compiler evidence:
 *
 * - All eleven contiguous C members of the ADT library object are exact under
 *   GCC 2.8.0. Linking their 2.8.0 objects produces zero differing bytes in
 *   main.exe; both canonical and Sony SN32 GCC 2.8.1 leave this function nine
 *   bytes off. In reload.c, 2.8.0 preserves INPADDR/OUTADDR as
 *   RELOAD_FOR_OPADDR_ADDR, while 2.8.1 retypes them unconditionally to
 *   RELOAD_FOR_OPERAND_ADDRESS. That version change alone decides whether the
 *   huge-frame address and value reload may share a3.
 * - Both synthetic do{}while(0) fences are gone. A normal list-display for
 *   loop plus the human D-pad `if / else if` chain reproduces every target
 *   callee-saved allocation. The earlier 37-byte regression came from spelling
 *   mutually exclusive directions as nested negated tests, not from removing
 *   a compiler fence.
 * - The entry count is the ordinary empty indexed loop
 *   `for (count = 0; menu[count].name != 0; count++) {}`. cc1's loop
 *   strength reduction creates the pointer cursor naturally. This clean source
 *   has the same nine-byte residual as the scaffolded draft.
 * - The demo's same-named body independently emits the TARGET self-tie at its
 *   larger frame offset (`lw a3,0(a3)` followed by the dereference), so the
 *   shape is demonstrably produced by the original human source family. It is
 *   evidence to keep searching upstream source identity, not evidence that the
 *   target is unreachable by this compiler.
 * - Fresh RTL localizes the remaining distinction: initial CSE turns the first
 *   test into a direct MEM through spilled menu, while loop strength reduction
 *   later creates a bare cursor pseudo. A fresh re-run corrected an earlier
 *   claim about volatile/address-taken spellings: NONE reproduced the target
 *   self-tie. A volatile parameter made 784 bytes, a volatile pointer local
 *   made 796 bytes, and loading through a volatile alias kept 776 bytes but
 *   caused a 51-byte allocation cascade. The only exact-length split-copy
 *   diagnostic was an empty asm constraint; it reduced the residual to two
 *   bytes but allocated the temporary/value in v0, not a3. It is lifetime
 *   evidence only, not acceptable source.
 * - The 776-byte body, including the a3 self-tie, is byte-identical in the
 *   shipped ENDING.EXE, MAIN.EXE, MENU.EXE, and TRIAL.EXE. This strongly points
 *   to the one reused ADT library object now represented by the build profile.
 *
 * The clean indexed loop below is exact. Any absolute "no natural C" or "do
 * not retry" language in the historical 2.8.1 log below is superseded.
 *
 * The 9 differing bytes are ONE reload decision, in the entry-count block:
 *
 *   target:  lw a3,0(a3); lw v0,0(a3);  ...  li t0,0x80CC; addu; lw v1,0(t0)
 *   ours:    lw t0,0(a3); lw v0,0(t0);  ...  li a3,0x80CC; addu; lw v1,0(a3)
 *
 * Both are the SAME 4-insn shape (`li`/`addu` materialise sp+0x80CC, load
 * menu, deref).  Only the register holding menu's value differs: the target
 * reuses the address register a3, we take t0.  Site 2's t0<->a3 is NOT an
 * independent difference — see the round-robin note below.
 *
 * The superseded round-by-round investigation log for this function lives
 * in docs/matching-archive.md.
 */

extern s32 VSync(s32 mode);

extern char str_select_item_2[]; /* select item */
extern char fmt_count_pair[];    /*  (%d/%d) */
extern char fmt_str[];           /* %s */
extern char str_blank_line[];    /* "\n\n" */
extern char str_arrow[];         /* -> */
extern char str_spaces[];        /*    */
extern char str_newline_4[];     /* "\n" */

s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode)
{
    TAdtDisp ad;
    s32 last;
    u32 trg;
    s32 count;
    s32 pages;
    s32 page;
    s32 first;
    u32 pad;
    short i;
    char *fmt;

    /* pad is read uninitialized on the first pass below (trg = pad before
     * the first AdtPadRead): retail's own. */
    do
    {
    } while (AdtPadRead(0) != 0);

    if (title == 0)
        title = str_select_item_2;

    for (count = 0; menu[count].name != 0; count++)
    {
    }

    pages = count / 18 + 1;
    AdtGetDisp(&ad);

    for (;;)
    {
        DrawPrim(&ad.bg);
        trg = pad;
        pad = AdtPadRead(0);
        trg = ~trg & pad;
        page = mode / 18;
        first = page * 18;
        last = first + 18;
        if (count < last)
            last = count;
        FntPrint(fmt_str, title);
        if (pages > 1)
            FntPrint(fmt_count_pair, page + 1, pages);
        FntPrint(str_blank_line);
        for (i = first; i < last; i++)
        {
            if (mode == i)
                fmt = str_arrow;
            else
                fmt = str_spaces;
            FntPrint(fmt);
            FntPrint(menu[i].name);
            FntPrint(str_newline_4);
        }
        FntFlush(-1);
        VSync(3);
        if (pad & (PADstart | PADRright))
            break;
        if (pad & PADRdown)
        {
            mode = count - 1;
            break;
        }
        /* i doubles as the cursor delta: byte-required (a separate local
         * loses the s-register identity; measured). */
        if (trg & PADLup)
            i = -1;
        else if (trg & PADLdown)
            i = 1;
        else if (trg & PADLleft)
            i = -18;
        else if (trg & PADLright)
            i = 18;
        else
            i = 0;
        mode += i;
        if (mode < 0)
            mode = 0;
        else if (count <= mode)
            mode = count - 1;
    }
    AdtReleaseDisp(&ad);
    return menu[mode].value;
}
