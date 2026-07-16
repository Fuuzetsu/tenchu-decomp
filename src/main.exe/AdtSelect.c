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
 * The residual is a COMBINE problem, not the reload problem this file used to
 * claim.  The earlier note said "a huge-offset-spilled pointer dereference can
 * NEVER self-tie".  Its reload mechanism is real (re-verified line-by-line
 * against the nix-pinned gcc-2.8.1 sources) but its FRAMING is wrong, and the
 * wrong framing is what kept the function parked:
 *
 *   reload.c ~4296  `reg_equiv_address` branch: recurses on the "sp+32972"
 *                   PLUS first (RELOAD_FOR_INPADDR_ADDRESS via ADDR_TYPE),
 *                   THEN push_reload's menu's value (RELOAD_FOR_INPUT_ADDRESS).
 *   reload1.c ~4567 RELOAD_FOR_INPUT_ADDRESS rejects any reg already in
 *                   reload_reg_used_in_inpaddr_addr[opnum] — the first
 *                   reload's own register.
 *   reload1.c ~4315 reload_reg_class_lower ties break on `r1 - r2` (push
 *                   order), so INPADDR_ADDRESS is always allocated first.
 *
 * All three are literally correct — but they are NOT a property of "huge
 * offsets" or of "spilled pointers".  They are a property of WHERE THE PSEUDO
 * APPEARS.  A pseudo INSIDE a MEM (`menu->choice_name`) goes through
 * find_reloads_address and is barred from self-tying.  The SAME pseudo as a
 * BARE OPERAND goes through find_reloads_toplev and becomes a single
 * RELOAD_FOR_INPUT, which only checks input_addr[i] for i > opnum — so it
 * self-ties freely.  Proof that this is the real discriminator: this very
 * function reloads this very `menu` pseudo from this very sp+0x80CC slot at
 * FOUR sites, and THREE of them already self-tie and byte-match —
 * `p = menu` (a bare copy), the print loop's `menu[i]` and the tail's
 * `menu[selection]` (bare operands of an addu).  Only the offset-0 deref,
 * where menu sits inside the MEM, is barred.  "Huge-offset-spilled" is not
 * the discriminator and must not be used as a park criterion.
 *
 * The self-tie is also not a "tie" between two reloads: at the bare-operand
 * sites it is ONE reload whose gen_reload sequence materialises the address
 * into its own destination (`ori a3; addu a3,a3,sp; lw a3,0(a3)`).
 *
 * So the target's site 1 is `X = menu; name = X->choice_name;` with the copy
 * surviving to reload (X in a3).  The shape IS reachable — declaring the
 * param `debug_menu_choice *volatile menu` breaks the mem-of-mem fold and
 * emits exactly the target's separate load-then-deref here — so "there is no
 * such respelling" was false.  It is not USABLE: volatile makes menu
 * memory-resident at all four sites, so the other three stop being
 * bare-operand reloads and global allocation shifts (s3/s4) — 9 -> 51 bytes.
 *
 * The real blocker, newly characterised, is a combine catch-22 measured this
 * round:
 *   - single-use copy (`q = menu; name = q->choice_name`): combine folds it
 *     back to (mem (reg 81)) within the block  ->  9 bytes  (unchanged)
 *   - multi-use copy (`p = menu; if (p->choice_name)...` reusing p as the
 *     loop cursor): the copy SURVIVES and site 1 self-ties correctly, but p
 *     then serves the loop too and the target's second, fresh `p = menu`
 *     read disappears  ->  764 bytes (12 short, i.e. 3 insns)
 *   - identical-arm fence around the copy: the arms are identical, so cse1
 *     merges them before combine ever sees the CODE_LABEL  ->  760 bytes
 * i.e. site 1 needs a copy that is SINGLE-USE (so read 2 survives) yet
 * survives combine (which folds exactly single-use copies inside a block).
 * A real basic-block boundary between the copy and its use would do it, but
 * the copy's load then lands on the far side of that boundary, and the
 * target's load is adjacent to its deref.  That is the open question; it is
 * not "no C respelling exists".
 *
 * Ruled out this round: autorules (23 candidates, no improving edit; both
 * do{}while(0) fences are load-bearing, unwrapping either costs +16), a
 * bounded decomp-permuter run (flat at 9, base best), and the demo build,
 * whose AdtSelect has a DIFFERENT frame (0x80E0/0x80E4) yet emits the same
 * `lw a3,0(a3)` — so the shape is a stable source property, not a
 * frame-size artefact.  All other allocation (9 callee-saved pseudos + 2
 * spilled params) matches.
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AdtSelect", AdtSelect);

#else /* NON_MATCHING */

extern s32 (*AdtPadRead)(s32);
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
    } while (AdtPadRead(0) != 0);

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
        pad = AdtPadRead(0);
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
