#include "common.h"
#include "main.exe.h"

/*
 * AdtSelect (0x8005fecc, 776 bytes) — modal debug-menu selection widget:
 * waits for pad release, saves the display state into a 0x8090-byte frame
 * buffer, then draws the choice list (18 per page) and moves the cursor on
 * edge-detected pad input until confirm (pad & 0x820 -> current entry) or
 * cancel (pad & 0x40 -> last entry); returns the entry's choice_number.
 *
 * STATUS: NON_MATCHING — 9 of 776 bytes differ.  Build the draft with
 * `NON_MATCHING=AdtSelect ./Build` (or tools/matchdiff.py, which sets it
 * automatically); the default build keeps the INCLUDE_ASM stub below.
 * The 9 differing
 * bytes are ONE reload-register swap (a3<->t0) in the entry-count block:
 *
 *   target:  lw a3,0(a3); lw v0,0(a3);  ...  li t0,0x80CC; addu; lw v1,0(t0)
 *   ours:    lw t0,0(a3); lw v0,0(t0);  ...  li a3,0x80CC; addu; lw v1,0(a3)
 *
 * PROVEN impossible to fix by C respelling (this session escalated past the
 * earlier "round-robin" guess to the exact reload1.c mechanism — see
 * docs/matching-cookbook.md's RTL-dump section for the method).  The frame
 * is 0x80C8 bytes, so the spilled params title/menu live at sp+0x80C8 /
 * sp+0x80CC — offsets > 32767 that no lw can encode, so every raw `menu`
 * read goes through `find_reloads_address`'s `reg_equiv_address[regno]`
 * branch (reload.c ~4304): it recurses on the "sp+32972" PLUS first (giving
 * a reload of type `RELOAD_FOR_INPADDR_ADDRESS`, since `ADDR_TYPE` maps
 * RELOAD_FOR_INPUT_ADDRESS -> RELOAD_FOR_INPADDR_ADDRESS, reload.c ~302),
 * *then* pushes a second reload — for menu's own value, used as the address
 * of the `->choice_name` mem — typed `RELOAD_FOR_INPUT_ADDRESS` (the type is
 * passed through unchanged).  `reload_reg_free_p`'s RELOAD_FOR_INPUT_ADDRESS
 * case (reload1.c ~4567) explicitly rejects any hard reg already marked in
 * `reload_reg_used_in_inpaddr_addr[opnum]` — i.e. the FIRST reload's own
 * register — so the second reload can NEVER reuse the first's register.
 * And this isn't a coin flip: `reload_reg_class_lower`, the qsort comparator
 * that orders same-class reloads for allocation (reload1.c ~4315), breaks
 * ties with `return r1 - r2` (reload NUMBER, i.e. push order) — and the
 * PLUS-reload is *always* pushed before the value-reload inside this one
 * `reg_equiv_address` branch, unconditionally, as a fact of the branch's own
 * code order, not of anything the C source can shape.  So this exact
 * "dereference a huge-offset-spilled pointer in one combined expression"
 * shape can PROVABLY never self-tie in this compiler — the earlier guess
 * that a source respelling might force the input-reload shape was wrong;
 * there is no such respelling.  (The title test `if (title == 0)` and the
 * return path `menu[selection].choice_number` DO self-tie, byte-matched,
 * because there the spilled value is the WHOLE read operand, not the
 * address of a further mem — that's plain RELOAD_FOR_INPUT, which has no
 * such reject rule.)
 * Confirmed twice more this session that introducing an intermediate
 * pointer local for the first access (`q = menu; name = q->choice_name;`,
 * and separately dropping `name` for `if (menu->choice_name != 0)`) changes
 * NOTHING — byte-identical 9-diff output both times — because `combine.c`
 * refolds any single-def/single-use temp back into the same combined
 * mem-of-mem RTL before reload ever runs (it doesn't yet know `menu` will
 * end up reg_equiv_address'd).  For target's a3-self-tie to exist, its
 * original source must avoid the reg_equiv_address recursion for this
 * access entirely — e.g. a pointer local that combine can't fold away
 * because it has a second, later use or crosses a boundary combine won't
 * cross (cf. "cse1 stops at NOTE_INSN_LOOP_END" in the cookbook) — but no
 * such construct was found that doesn't also change the (already-matching)
 * instruction count elsewhere.  menu[count] with unknown count emits an
 * extra sll, K&R params change nothing, and an 80k-iteration
 * decomp-permuter run never beat the score-10 base (expected: this is a
 * pre-AST, reload-pass decision, permuter-immune by construction).  All
 * other register allocation (9 callee-saved pseudos + 2 spilled params)
 * matches.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AdtSelect", AdtSelect);

#else /* NON_MATCHING */

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

#endif /* NON_MATCHING */
