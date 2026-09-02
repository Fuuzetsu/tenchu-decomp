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
    sprintf((char *)name, fmt_stage_esd, path_anim,
            STAGE_NUMBER(StageID));
    StageEvent = (EventSeqType *)FileRead(name);
    StartStageSequence();
}
