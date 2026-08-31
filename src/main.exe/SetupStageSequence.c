#include "common.h"
#include "main.exe.h"
#include "stage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupStageSequence(void);
 *     STAGE.C:77, 12 src lines, frame 80 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     unsigned char [50] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern struct EventSeqType *StageEvent;
 *     extern struct Humanoid *StagePlayer;
 *     extern int StageID;
 * END PSX.SYM */

/*
 * SetupStageSequence (0x8004e8f4, 0x70 bytes) — reset the stage's event
 * sequence: point StagePlayer at the first live human (HumanGroup[0]), free
 * any previous StageEvent block, load
 * "K:\WORK\CDIMAGE\ANIM\STAGE<n>.ESD" (n = StageID+1) via FileRead into
 * StageEvent, and kick off StartStageSequence.
 *
 * splat/reverse.py see this as a 2-piece split only because
 * config/symbols.main.exe.txt carries a debug symbol
 * (update_events__override__prt_8004e938_407e704c) at the mid-function
 * address 0x8004e938 — there's no branch there (piece 1 falls straight
 * through into piece 2's `jal sprintf`), so this is one ordinary
 * straight-line function; both INCLUDE_ASM pieces are satisfied by the same
 * C body (same non-jump-table "__override__prt" split as FileOption's
 * debug_menu_file_option__override__prt_ piece — no _jtbl array needed).
 *
 */
extern void vfree(void *p);
extern void StartStageSequence(void);
extern void sprintf(char *s, char *fmt, ...);

extern char fmt_stage_esd[]; /* %sSTAGE%d.ESD */
extern char path_anim[];     /* K:\\WORK\\CDIMAGE\\ANIM\\ */

void SetupStageSequence(void)
{
    u8 name[50];

    StagePlayer = HumanGroup[0];
    if (StageEvent != 0)
    {
        vfree(StageEvent);
    }
    sprintf((char *)name, fmt_stage_esd, path_anim, StageID + 1);
    StageEvent = (EventSeqType *)FileRead(name);
    StartStageSequence();
}
