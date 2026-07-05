#include "common.h"
#include "main.exe.h"

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
 * StagePlayer/StageEvent/HumanGroup kept as bare `void *`/`void *[]` here —
 * nothing dereferences a field, only assigns/frees the whole pointer
 * (freeing `StageEvent` itself is identical to Ghidra's `&StageEvent->id`:
 * EventSeqType's `id` sits at offset 0, reference/ghidra_types.h).
 * ReturnNormal.c independently proves StagePlayer's real type is
 * `Humanoid *`; kept untyped here per the "declare only what's touched"
 * convention.
 */
extern void vfree(void *p);
extern u_long *FileRead(char *path);
extern void StartStageSequence(void);
extern void sprintf(char *s, char *fmt, ...);

extern void *StagePlayer;
extern void *StageEvent;
extern void *HumanGroup[];
extern s32 StageID;
extern char D_80012808[]; /* "%sSTAGE%d.ESD" */
extern char D_80012818[]; /* "K:\\WORK\\CDIMAGE\\ANIM\\" */

void SetupStageSequence(void)
{
    char buf[56];

    StagePlayer = HumanGroup[0];
    if (StageEvent != 0) {
        vfree(StageEvent);
    }
    sprintf(buf, D_80012808, D_80012818, StageID + 1);
    StageEvent = FileRead(buf);
    StartStageSequence();
}
