#include "common.h"
#include "main.exe.h"
#include "misc.h"
#include "tmdfast.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoMiscProc(void);
 *     MISC.C:713, 19 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       int i
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

/*
 * STATUS: MATCHING
 *
 * DoMiscProc (0x8004d350, 0x1C4 bytes) — the misc pool's per-frame driver
 * (main's game loop): bails with an error box if InitMisc hasn't run yet;
 * otherwise, every 10th GameClock tick, tests whether each live slot is
 * within LEN (15000) units on every camera axis (ViewInfo.vrx/vry/vrz),
 * dispatching MM_RESUME(3)+unpause when back in range and
 * MM_PAUSE(2)+pause when it drops out — then unconditionally runs every
 * still-unpaused slot's "draw" tick
 * MM_DO(4) after setting the renderer's TMD mode.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `GameClock == (GameClock / 10) * 10` reproduces the div-by-10
 *    magic-multiply automatically (same idiom as DoItemProc's identical
 *    tick gate, same TU-independent shape).
 *  - The first scan is a literal goto loop.  Without loop notes, loop.c does
 *    not strength-reduce `pause` into a second `p + 0x14` induction pointer;
 *    `p` can remain nonvolatile, so the zero store fills the resume jump's
 *    delay slot.  A genuine loop needed a volatile pointee to avoid that GIV,
 *    and the volatile store forced a duplicate counter increment.
 *  - Caching `ViewInfo` explicitly gives the target's s2 base. Initialising
 *    `i`, `view`, then `p` reproduces the counter/ViewInfo/misc preheader;
 *    direct `__builtin_abs(view->field - p->field)` expressions retain the
 *    target load order and subtraction roles without staging locals.
 *  - Cache `p->proc` for the in-range call, but dispatch the out-of-range call
 *    through the field.  That distinction matches the target call carriers.
 *  - `DrawTMDmode = TMD_BANK_FOG;` sits textually right after the cull loop in
 *    source, but its `li` is independent of the tick-gate branch, so cc1
 *    hoists it into that branch's own delay slot regardless of which side
 *    is taken — ordinary scheduling, no special spelling.
 */

extern char msg_misc_not_initialized[]; /* misc not initialized */

/* Misc visibility distance from MISC.C's anonymous enum. */
enum
{
    LEN = 15000
};

void DoMiscProc(void)
{
    TMisc *p;
    GsRVIEW2 *view;
    void (*proc)(TMisc *, TMiscMessage);

    if (Misc_fInitial == 0)
    {
        AdtMessageBox(msg_misc_not_initialized);
    }
    else
    {
        if (GameClock == (GameClock / 10) * 10)
        {
            s32 i;

            i = 0;
            view = &ViewInfo;
            p = misc;
        cull_loop:
            proc = p->proc;
            if (proc != 0)
            {
                if (__builtin_abs(view->vrx - p->x) < LEN)
                {
                    if (__builtin_abs(view->vry - p->y) < LEN)
                    {
                        if (__builtin_abs(view->vrz - p->z) < LEN)
                        {
                            if (p->pause != MISC_ACTIVE)
                            {
                                proc(p, MM_RESUME);
                                p->pause = MISC_ACTIVE;
                            }
                            goto next;
                        }
                    }
                }
                if (p->pause == MISC_ACTIVE)
                {
                    p->proc(p, MM_PAUSE);
                    p->pause = MISC_PAUSED;
                }
            }
        next:
            i++;
            p++;
            if (i < MaxMisc)
                goto cull_loop;
        }
        {
            s32 i;

            DrawTMDmode = TMD_BANK_FOG;
            for (i = 0; i < MaxMisc; i++)
            {
                if (misc[i].proc != 0 && misc[i].pause == MISC_ACTIVE)
                {
                    misc[i].proc(&misc[i], MM_DO);
                }
            }
        }
    }
}
