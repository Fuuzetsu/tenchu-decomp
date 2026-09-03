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
        if (GameClock % 10 == 0)
        {
            s32 i;

            i = 0;
            view = &ViewInfo;
            p = misc;
        cull_loop:
            proc = p->proc;
            if (proc != 0)
            {
                if (__builtin_abs(view->vrx - p->x) < LEN &&
                    __builtin_abs(view->vry - p->y) < LEN &&
                    __builtin_abs(view->vrz - p->z) < LEN)
                {
                    if (p->pause != MISC_ACTIVE)
                    {
                        proc(p, MM_RESUME);
                        p->pause = MISC_ACTIVE;
                    }
                }
                else if (p->pause == MISC_ACTIVE)
                {
                    p->proc(p, MM_PAUSE);
                    p->pause = MISC_PAUSED;
                }
            }
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
