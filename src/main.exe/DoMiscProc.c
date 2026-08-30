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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 *  - Caching `ViewInfo` explicitly gives the target's s2 base.  Initialising
 *    `i`, `view`, then `p` reproduces the counter/ViewInfo/misc preheader.
 *    The named `coord` temp plus `d = view->field; coord = p->field; d -=
 *    coord;` fixes both the target load order and its v0/v1 subtraction roles.
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
    s32 i;
    s32 d;
    s32 coord;
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
            i = 0;
            view = &ViewInfo;
            p = misc;
        cull_loop:
            proc = p->proc;
            if (proc != 0)
            {
                d = view->vrx;
                coord = p->x;
                d -= coord;
                if (d < 0)
                    d = -d;
                if (d < LEN)
                {
                    d = view->vry;
                    coord = p->y;
                    d -= coord;
                    if (d < 0)
                        d = -d;
                    if (d < LEN)
                    {
                        d = view->vrz;
                        coord = p->z;
                        d -= coord;
                        if (d < 0)
                            d = -d;
                        if (d < LEN)
                        {
                            if (p->pause != 0)
                            {
                                proc(p, MM_RESUME);
                                p->pause = 0;
                            }
                            goto next;
                        }
                    }
                }
                if (p->pause == 0)
                {
                    p->proc(p, MM_PAUSE);
                    p->pause = 1;
                }
            }
        next:
            i++;
            p++;
            if (i < MaxMisc)
                goto cull_loop;
        }
        DrawTMDmode = TMD_BANK_FOG;
        for (i = 0; i < MaxMisc; i++)
        {
            if (misc[i].proc != 0 && misc[i].pause == 0)
            {
                misc[i].proc(&misc[i], MM_DO);
            }
        }
    }
}
