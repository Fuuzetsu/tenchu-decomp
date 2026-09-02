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
