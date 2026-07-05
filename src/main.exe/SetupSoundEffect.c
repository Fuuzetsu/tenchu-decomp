#include "common.h"
#include "main.exe.h"

/*
 * SetupSoundEffect (0x8004fe70, 0xA0 bytes) — (re)load the stage's ambient
 * sound bank: dispose any previous STAGE_SOUNDS_POINTER, then (if `stage`
 * is a real stage index, i.e. >= 0) build
 * "<language-prefix>STAGE<stage><'A'|'R'>.VAB" and hand it to
 * FileRead + SetupSE.
 *
 * splat/reverse.py see this as a 2-piece split only because
 * config/symbols.main.exe.txt carries a debug symbol
 * (initialise_stage_sounds__override__prt_8004fee0_a9b16ba2) at the
 * mid-function address 0x8004fee0 — there's no branch there (piece 1 falls
 * straight through into piece 2's `jal sprintf`); the ONE real internal
 * branch (`bltz $a3, .L8004FEFC`, the `stage < 0` early-exit) targets the
 * shared epilogue INSIDE piece 2, not the piece boundary. So this is one
 * ordinary straight-line function with no `_jtbl` array — same
 * non-jump-table "__override__prt" split as SetupStageSequence.c /
 * FileOption's buried instance.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `stage`'s sign-extension for the `stage >= 0` guard test is computed
 *    ONCE into $a3 and stays live (no intervening call) all the way to the
 *    sprintf call, where it's reused directly as the `%d` argument — cse
 *    folds the second reference, so no separate temp is needed; just refer
 *    to the `stage` parameter both times.
 *  - The 'A'/'R' selector is a full 4-byte `int` (`sw`, not `sb`) — it's a
 *    variadic sprintf argument (default char->int promotion), not `char`.
 */
extern void DisposeSE(void *se);
extern u_long *FileRead(char *path);
extern void *SetupSE(u8 *vab);
extern void sprintf(char *s, char *fmt, ...);

extern void *STAGE_SOUNDS_POINTER;
extern u8 CHOSEN_LANGUAGE;
extern char *STAGE_SOUND_PREFICES[];
extern char D_8001359C[]; /* "%sSTAGE%d%c.VAB" */

void SetupSoundEffect(short mode, short stage)
{
    char buf[104];

    if (STAGE_SOUNDS_POINTER != 0)
    {
        DisposeSE(STAGE_SOUNDS_POINTER);
    }
    STAGE_SOUNDS_POINTER = 0;
    if (stage >= 0)
    {
        sprintf(buf, D_8001359C, STAGE_SOUND_PREFICES[CHOSEN_LANGUAGE], stage,
                mode == 0 ? 0x52 : 0x41);
        STAGE_SOUNDS_POINTER = SetupSE((u8 *)FileRead(buf));
    }
}
