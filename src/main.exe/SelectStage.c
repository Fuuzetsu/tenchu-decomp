#include "common.h"
#include "main.exe.h"
#include "infoview.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SelectStage(void);
 *     INFOVIEW.C:675, 15 src lines, frame 1016 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct TAdtSelect [10] StageSelect
 *     stack sp+96     unsigned char [9][100] name
 *     reg   $s0       int i
 * END PSX.SYM */

/*
 * SelectStage (0x8005c404) — builds language, character, and stage debug
 * menus, then writes the selected values into the caller's persistent-state
 * record. A selection outside StageConfig is the menu's retry entry, so all
 * three prompts repeat until a real stage is chosen.
 *
 * Matching notes:
 *  - Retail takes `TLinkInfo *ps`, despite the demo symbol's stale
 *    `void SelectStage(void)` prototype. Both retail callers pass the state
 *    pointer in a0, and this body stores its three results at +0x5e/+4/+5.
 *  - The local declarations reproduce the full 0x500-byte working window:
 *    language[N_LANGUAGES + 1] at sp+0x10, player[3] at sp+0x38,
 *    StageSelect[14] at sp+0x50,
 *    and name[N_STAGE_CONFIGS][100] at sp+0xc0. Their padding plus the
 *    saved-register area gives the target's 0x528 frame.
 *  - Capturing `StageConfig[i].uid` once keeps it live across `sprintf` in
 *    s0 and lets both stage-entry stores reuse one computed address. Reading
 *    the field separately at each use was three instructions too long.
 *  - The stage scan is deliberately a `while (1)` with an explicit top
 *    break. A natural `for` is folded into a bottom-tested loop and is two
 *    instructions short; retail keeps the top test and unconditional back
 *    jump, with `i++` in its delay slot.
 *  - `debug_menu_select_stage__override__prt_8005c4d8_8cf8befb` is an
 *    interior call-site prototype marker, not a second function. Its asm
 *    piece falls straight into `sprintf` and branches back into the first
 *    piece; this single C body matches the complete 420-byte carve.
 */

extern char fmt_index_name[];      /* %2d  %s */
extern u8 str_back[];              /* back */
extern char str_language_select[]; /* language select */
extern char str_player_select[];   /* player select */
extern char str_stage_select[];    /* stage select */

extern s32 AdtSelect(char *title, TAdtSelect *menu, s32 mode);
extern int sprintf(char *buf, char *fmt, ...);

void SelectStage(TLinkInfo *ps)
{
    TAdtSelect language[N_LANGUAGES + 1];
    TAdtSelect player[3];
    TAdtSelect StageSelect[14];
    u8 name[N_STAGE_CONFIGS][100];
    s32 i;
    s32 uid;

    __builtin_memcpy(language, DEBUG_MENU_LANGUAGE_CHOICES, sizeof(language));
    __builtin_memcpy(player, sel_player, sizeof(player));
    i = 0;
    while (1)
    {
        if (i >= N_STAGE_CONFIGS)
        {
            break;
        }
        uid = StageConfig[i].uid;
        sprintf((char *)name[i], fmt_index_name, uid, StageConfig[i].name);
        StageSelect[uid].name = name[i];
        StageSelect[uid].value = i;
        i++;
    }
    StageSelect[i].name = str_back;
    StageSelect[i].value = N_STAGE_CONFIGS;
    StageSelect[i + 1].name = NULL;

    do
    {
        ps->language = AdtSelect(str_language_select, language, 0);
        ps->CharType = AdtSelect(str_player_select, player, 0);
        ps->StageNo = AdtSelect(str_stage_select, StageSelect, 0);
    } while (ps->StageNo >= N_STAGE_CONFIGS);
}
