#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupSoundEffect(short mode, short stage);
 *     SEMNG.C:26, 12 src lines, frame 144 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short mode
 *     param $a1       short stage
 *     stack sp+24     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct SoundEffect *StageSE;
 * END PSX.SYM */

/*
 * SetupSoundEffect (0x8004fe70, 0xA0 bytes) — (re)load the selected
 * character's stage sound bank: dispose any previous StageSE, then (if `stage`
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
extern void DisposeSE(SoundEffect *se);
extern SoundEffect *SetupSE(u8 *vab);
extern void sprintf(char *s, char *fmt, ...);

extern u8 CHOSEN_LANGUAGE;
extern char *STAGE_SOUND_PREFICES[N_LANGUAGES];
extern char fmt_stage_vab[]; /* %sSTAGE%d%c.VAB */

void SetupSoundEffect(character_kind character, short stage)
{
    u8 name[100];

    if (StageSE != 0)
    {
        DisposeSE(StageSE);
    }
    StageSE = 0;
    if (stage >= 0)
    {
        sprintf((char *)name, fmt_stage_vab,
                STAGE_SOUND_PREFICES[CHOSEN_LANGUAGE], stage,
                character == RIKIMARU_0 ? 'R' : 'A');
        StageSE = SetupSE((u8 *)FileRead(name));
    }
}
