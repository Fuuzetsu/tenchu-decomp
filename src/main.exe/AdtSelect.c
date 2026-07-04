#include "common.h"
#include "main.exe.h"

/*
 * AdtSelect (0x8005fecc, 776 bytes) — modal debug-menu selection widget:
 * waits for pad release, saves the display state into a 0x8090-byte frame
 * buffer, then draws the choice list (18 per page) and moves the cursor on
 * edge-detected pad input until confirm (pad & 0x820 -> current entry) or
 * cancel (pad & 0x40 -> last entry); returns the entry's choice_number.
 *
 * CURRENT(9): best attempt below matches 767/776 bytes.  The 9 differing
 * bytes are ONE reload-register swap (a3<->t0) in the entry-count block:
 *
 *   target:  lw a3,0(a3); lw v0,0(a3);  ...  li t0,0x80CC; addu; lw v1,0(t0)
 *   ours:    lw t0,0(a3); lw v0,0(t0);  ...  li a3,0x80CC; addu; lw v1,0(a3)
 *
 * Cause (verified against gcc-2.8.1 reload.c/reload1.c sources): the frame
 * is 0x80C8 bytes, so the spilled params title/menu live at sp+0x80C8 /
 * sp+0x80CC — offsets > 32767 that no lw can encode.  reg_equiv_address
 * kicks in and every `menu` read needs reload-materialized addresses.  For
 * `menu->choice_name` (a mem whose ADDRESS contains the spilled reg) our
 * cc1 deterministically pushes TWO conflicting RELOAD_FOR_OPERAND_ADDRESS
 * reloads (the plus and the pointer) -> two spill regs a3+t0 and the
 * round-robin `last_spill_reg` rotation then hands BB5's p=menu reload a3.
 * The original's bytes need the pointer reload to SHARE a3 in-place
 * (allowed only for a whole-operand RELOAD_FOR_INPUT, cf. the title test
 * `if (title == 0)` and the return path `menu[selection].choice_number`,
 * both of which read the POINTER VALUE and do share, byte-matched).  No C
 * spelling found that keeps the deref yet forces the input-reload shape:
 * name-temps / pointer-temps cse-fold back to the identical RTL,
 * menu[count] with unknown count emits an extra sll, K&R params and an
 * 80k-iteration decomp-permuter run change nothing.  All other register
 * allocation (9 callee-saved pseudos + 2 spilled params) matches.
 */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AdtSelect", AdtSelect);

#if 0 /* CURRENT(9) best attempt — activate by removing INCLUDE_ASM above */

extern s32 (*AdtReadPadFunc)(s32);
extern void AdtGetDisp(u8 *buf);
extern void AdtReleaseDisp(u8 *buf);
extern void DrawPrim(u8 *prim);
extern void FntPrint(char *fmt, ...);
extern s32 FntFlush(s32 id);
extern s32 VSync(s32 mode);

extern char D_80014AFC[]; /* "select item" */
extern char D_80014B08[]; /* " (%d/%d)" */
extern char D_80097E9C[]; /* "%s" */
extern char D_80097EA0[]; /* "\n\n" */
extern char D_80097EA4[]; /* "->" */
extern char D_80097EA8[]; /* "  " */
extern char D_80097EAC[]; /* "\n" */

s32 AdtSelect(char *title, debug_menu_choice *menu, s32 selection)
{
    u8 buf[0x8090];
    s32 last;
    u32 trg;
    debug_menu_choice *p;
    s32 count;
    s32 pages;
    s32 page;
    s32 first;
    u32 pad;
    short i;
    char *fmt;
    char *name;

    do
    {
    } while (AdtReadPadFunc(0) != 0);

    if (title == 0)
        title = D_80014AFC;

    count = 0;
    name = menu->choice_name;
    if (name != 0)
    {
        p = menu;
        do
        {
            p++;
            count++;
        } while (p->choice_name != 0);
    }

    pages = count / 0x12 + 1;
    AdtGetDisp(buf);

    for (;;)
    {
        DrawPrim(buf + 0x8078);
        trg = pad;
        pad = AdtReadPadFunc(0);
        trg = ~trg & pad;
        page = selection / 0x12;
        first = page * 0x12;
        last = first + 0x12;
        if (count < last)
            last = count;
        FntPrint(D_80097E9C, title);
        if (pages > 1)
            FntPrint(D_80014B08, page + 1, pages);
        FntPrint(D_80097EA0);
        i = first;
        do
        {
            /* do{}while(0) wrappers: flow.c loop-depth ref weighting;
               see the matching notes above and docs/matching-cookbook.md */
            for (; i < last; i++)
            {
                if (selection == i)
                    fmt = D_80097EA4;
                else
                    fmt = D_80097EA8;
                FntPrint(fmt);
                FntPrint(menu[i].choice_name);
                FntPrint(D_80097EAC);
            }
        } while (0);
        FntFlush(-1);
        VSync(3);
        if (pad & 0x820)
            break;
        if (pad & 0x40)
        {
            selection = count - 1;
            break;
        }
        i = -1;
        if (!(trg & 0x1000))
        {
            i = 1;
            if (!(trg & 0x4000))
            {
                i = -0x12;
                if (!(trg & 0x8000))
                {
                    i = 0;
                    do
                    {
                        if (trg & 0x2000)
                            i = 0x12;
                    } while (0);
                }
            }
        }
        selection += i;
        if (selection < 0)
            selection = 0;
        else if (count <= selection)
            selection = count - 1;
    }
    AdtReleaseDisp(buf);
    return menu[selection].choice_number;
}

#endif /* CURRENT(9) */
