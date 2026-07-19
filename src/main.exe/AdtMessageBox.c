#include "common.h"
#include "main.exe.h"
#include "adt.h"

/*
 * AdtMessageBox (0x8005f958, 624 bytes) — the "Adt" debug console's modal
 * message box: printf-style formats `fmt`+varargs into a stack buffer via
 * AdtVsprintf, then draws it full-screen with FntPrint and (unless the
 * global quiet flag is set) blocks on a pad button before restoring the
 * display state. Highest in-degree unmatched function in the game (87
 * call sites); every already-matched caller declares it
 * `extern void AdtMessageBox(char *fmt, ...);` and passes a POOLED STRING
 * SYMBOL (`extern char D_XXXXXXXX[];`) as fmt, never a literal.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Variadic prologue: taking `&fmt` (the ONLY named parameter) forces
 *    cc1 to spill all four incoming argument registers (fmt + 3 varargs)
 *    to their ABI-reserved stack slots at function entry, the same idiom
 *    already verified in this file's own sibling FUN_8005fe38.c
 *    (`s32 *ap = (s32 *)((char *)&arg0 + sizeof(arg0));`). AdtVsprintf's
 *    own signature (`int AdtVsprintf(s32 *args, char *dst, u32 n, char
 *    *fmt)`, from FUN_8005fe38.c) takes that raw spilled-register pointer,
 *    not a `va_list` abstraction — there is no stdarg.h in this codebase.
 *  - `fmt` doubles as a MUTABLE local (its own address is taken below, so
 *    cc1 keeps it memory-resident for its whole lifetime — reusing a
 *    second name here would just get its own register/copy instead): a
 *    leading "%#"/"%$" prefix is stripped (advancing the pointer by 2) and
 *    drives a 0/1/2 `mode` selecting how modal the box is (2 = "%#": no
 *    counter line, no wait; 1 = "%$": show once, no wait; 0 = default:
 *    full modal "press start" wait). Two shape levers here, both needed
 *    together: (1) Ghidra's own `goto` for the "neither # nor $" case
 *    (must skip the '#'/'$' handling entirely, unreachable via a plain
 *    if/else-if); (2) the '#' test's polarity is the OPPOSITE of Ghidra's
 *    literal rendering — the target reaches the '#' body (mode=2) via a
 *    forward branch with the "$"-test as the fallthrough, so the source
 *    tests `fmt[1] != '#'` first (cc1 always makes the `then` arm the
 *    fallthrough); and (3) `fmt += 2` must be DUPLICATED in both arms, not
 *    shared after the if/else — a single shared statement forces a fresh
 *    stack reload of fmt at the merge point instead of reusing the
 *    register each arm already has it in (cc1 still cross-jump-merges the
 *    resulting identical tails at the RTL level, so this costs nothing).
 *  - The two guard clauses (`AdtFnt.quiet == 1` and the button-held check)
 *    are plain early `return;`s with no `else` — Ghidra's own rendering of
 *    the second one (`if (A==0 || B==0) { <long body> }`, no else) is the
 *    De-Morgan-inverted form of the natural guard clause; the SECOND
 *    `AdtPadRead(0)` call is only ever made when the first already
 *    tested true, so the natural source is the short-circuit `&&` guard
 *    (`if (A && B) return;`) rather than Ghidra's literal `||`.
 *  - D_8008F1B8 (same font-adapter global as AdtFntLoad.c/AdtFntOpen.c/
 *    AdtQuiet.c) is reached at every field this TU touches: x/y/w/h/
 *    isbg/n@0x0-0x14 (FntOpen), tx/ty@0x18/0x1c (FntLoad), and quiet@0x20
 *    (the early guard) — this file's own "declare only what you touch"
 *    view therefore covers all nine fields.
 *  - The DRAWENV/DISPENV/RECT/backup-buffer/primitive locals must be ONE
 *    combined struct local (TAdtDisp, see AdtReleaseDisp.c, extended here
 *    with a trailing POLY_F4 field this TU also touches), NOT
 *    five separate top-level array/struct locals — even though Ghidra
 *    renders them as separate acStack_8298/DStack_80a0/DStack_8044/
 *    RStack_8030/auStack_8028/auStack_28 locals. cc1's stack-frame
 *    allocator pads EACH separately-declared aggregate local up to a
 *    multiple of 8 bytes (DRAWENV 0x5c->0x60, DISPENV 0x14->0x18),
 *    inflating the frame by 8 bytes total and shifting every later
 *    offset — whereas FIELD offsets inside one combined struct follow
 *    plain member alignment with no such rounding, matching the target's
 *    tight layout (rect at draw+0x70, backup at draw+0x78, primitive at
 *    draw+0x8078) exactly.
 *  - The two `DrawPrim` calls pass the SAME 24-byte stack primitive
 *    (the combined struct's trailing field), never written by this
 *    function itself — matches Ghidra's `auStack_28` exactly; whatever it
 *    draws is whatever was left on the stack, not this function's concern.
 */
typedef struct
{
    s32 x, y, w, h, isbg, n; /* +0x00-0x14, FntOpen args */
    s32 tx, ty;              /* +0x18, +0x1c, FntLoad args */
    s32 quiet;               /* +0x20, AdtQuiet() flag */
} AdtFntState;

extern AdtFntState D_8008F1B8;
extern s32 (*AdtPadRead)(s32 port);
extern s32 AdtDmyPadRead(s32 port);
extern s32 AdtMessageBoxCount; /* AdtMessageBox call counter */
extern char D_80014AAC[]; /* "*** AdtInit not called ***" */
extern char D_80014AC8[]; /* "AdtMessageBox #%d\n\n" */
extern char D_80014ADC[]; /* "\n\nPress start to continue..." */

extern s32 AdtVsprintf(s32 *args, char *dst, u32 n, char *fmt);
extern void FntPrint(char *fmt, ...);
extern s32 FntFlush(s32 id);
extern s32 VSync(s32 mode);
extern void FntLoad(int tx, int ty);
extern int FntOpen(int x, int y, int w, int h, int isbg, int n);

void AdtMessageBox(char *fmt, ...)
{
    s32 mode;
    s32 count;
    char buf[504];
    TAdtDisp ad;

    mode = 0;
    if (AdtPadRead == AdtDmyPadRead)
        fmt = D_80014AAC;
    if (D_8008F1B8.quiet == 1)
        return;
    if (*fmt == '%')
    {
        if (fmt[1] != '#')
        {
            if (fmt[1] != '$')
                goto skip;
            mode = 1;
            fmt += 2;
        }
        else
        {
            mode = 2;
            fmt += 2;
        }
    }
skip:
    if ((AdtPadRead(0) & 0x100) && (AdtPadRead(0) & 0x800))
        return;

    AdtVsprintf((s32 *)((char *)&fmt + sizeof(fmt)), buf, 0x1F4, fmt);
    AdtGetDisp(&ad);
    if (mode < 2)
    {
        DrawPrim(&ad.bg);
        count = AdtMessageBoxCount + 1;
        AdtMessageBoxCount = count;
        FntPrint(D_80014AC8, count);
    }
    FntPrint(buf);
    if (mode == 0)
        FntPrint(D_80014ADC);
    FntFlush(-1);
    DrawSync(0);
    VSync(2);
    if (mode == 0)
    {
        while (AdtPadRead(0) & 0x800)
            VSync(0);
        while (!(AdtPadRead(0) & 0x800))
            VSync(0);
        DrawPrim(&ad.bg);
        DrawSync(0);
    }
    FntLoad(D_8008F1B8.tx, D_8008F1B8.ty);
    FntOpen(D_8008F1B8.x, D_8008F1B8.y, D_8008F1B8.w, D_8008F1B8.h,
            D_8008F1B8.isbg, D_8008F1B8.n);
    LoadImage(&ad.rect, ad.backup);
    DrawSync(0);
    PutDrawEnv(&ad.draw);
    PutDispEnv(&ad.disp);
}
